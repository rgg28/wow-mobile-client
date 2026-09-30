#include <jni.h>

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <vulkan/vulkan.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>
#include <algorithm>

#define LOG_TAG "WoWRenderer"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace
{

// ================================================================
// BACKEND
// ================================================================

enum class RendererBackend
{
    AUTO = 0,
    OPENGL = 1,
    VULKAN = 2
};

RendererBackend gRequestedBackend =
        RendererBackend::AUTO;

RendererBackend gActiveBackend =
        RendererBackend::OPENGL;

// ================================================================
// COMMON WINDOW
// ================================================================

ANativeWindow* gWindow = nullptr;

int gWidth = 1;
int gHeight = 1;

std::mutex gMutex;

std::atomic<bool> gRunning(false);
std::thread gRenderThread;

// ================================================================
// CAMERA
// ================================================================

float gCameraYaw = 0.0f;
float gCameraPitch = 0.15f;
float gCameraDistance = 5.0f;

// ================================================================
// MATRICES
// ================================================================

struct Mat4
{
    float m[16];

    Mat4()
    {
        identity();
    }

    void identity()
    {
        std::memset(m, 0, sizeof(m));

        m[0] = 1.0f;
        m[5] = 1.0f;
        m[10] = 1.0f;
        m[15] = 1.0f;
    }
};

Mat4 multiply(
        const Mat4& a,
        const Mat4& b)
{
    Mat4 result;

    for (int col = 0; col < 4; ++col)
    {
        for (int row = 0; row < 4; ++row)
        {
            result.m[col * 4 + row] =
                    a.m[0 * 4 + row] * b.m[col * 4 + 0] +
                    a.m[1 * 4 + row] * b.m[col * 4 + 1] +
                    a.m[2 * 4 + row] * b.m[col * 4 + 2] +
                    a.m[3 * 4 + row] * b.m[col * 4 + 3];
        }
    }

    return result;
}

Mat4 perspective(
        float fovY,
        float aspect,
        float nearPlane,
        float farPlane)
{
    Mat4 result;

    std::memset(result.m, 0, sizeof(result.m));

    const float f =
            1.0f /
            std::tan(fovY * 0.5f);

    result.m[0] = f / aspect;
    result.m[5] = f;

    result.m[10] =
            (farPlane + nearPlane) /
            (nearPlane - farPlane);

    result.m[11] = -1.0f;

    result.m[14] =
            (2.0f * farPlane * nearPlane) /
            (nearPlane - farPlane);

    return result;
}

Mat4 translation(
        float x,
        float y,
        float z)
{
    Mat4 result;

    result.m[12] = x;
    result.m[13] = y;
    result.m[14] = z;

    return result;
}

Mat4 rotationX(float angle)
{
    Mat4 result;

    const float c = std::cos(angle);
    const float s = std::sin(angle);

    result.m[5] = c;
    result.m[6] = s;
    result.m[9] = -s;
    result.m[10] = c;

    return result;
}

Mat4 rotationY(float angle)
{
    Mat4 result;

    const float c = std::cos(angle);
    const float s = std::sin(angle);

    result.m[0] = c;
    result.m[2] = -s;
    result.m[8] = s;
    result.m[10] = c;

    return result;
}

// ================================================================
// OPENGL ES
// ================================================================

EGLDisplay gEglDisplay =
        EGL_NO_DISPLAY;

EGLSurface gEglSurface =
        EGL_NO_SURFACE;

EGLContext gEglContext =
        EGL_NO_CONTEXT;

GLuint gGlProgram = 0;
GLuint gGlVertexBuffer = 0;
GLuint gGlIndexBuffer = 0;

GLint gGlMvpLocation = -1;

struct Vertex
{
    float x;
    float y;
    float z;

    float r;
    float g;
    float b;
};

const Vertex CUBE_VERTICES[] =
{
    {-1.0f,-1.0f, 1.0f, 0.10f,0.45f,0.95f},
    { 1.0f,-1.0f, 1.0f, 0.10f,0.45f,0.95f},
    { 1.0f, 1.0f, 1.0f, 0.15f,0.65f,1.00f},
    {-1.0f, 1.0f, 1.0f, 0.15f,0.65f,1.00f},

    {-1.0f,-1.0f,-1.0f, 0.05f,0.20f,0.55f},
    { 1.0f,-1.0f,-1.0f, 0.05f,0.20f,0.55f},
    { 1.0f, 1.0f,-1.0f, 0.10f,0.35f,0.75f},
    {-1.0f, 1.0f,-1.0f, 0.10f,0.35f,0.75f},

    {-1.0f, 1.0f,-1.0f, 0.85f,0.60f,0.08f},
    { 1.0f, 1.0f,-1.0f, 0.95f,0.72f,0.12f},
    { 1.0f, 1.0f, 1.0f, 1.00f,0.82f,0.20f},
    {-1.0f, 1.0f, 1.0f, 0.95f,0.70f,0.10f},

    {-1.0f,-1.0f,-1.0f, 0.03f,0.08f,0.18f},
    { 1.0f,-1.0f,-1.0f, 0.04f,0.10f,0.22f},
    { 1.0f,-1.0f, 1.0f, 0.06f,0.14f,0.30f},
    {-1.0f,-1.0f, 1.0f, 0.05f,0.12f,0.26f},

    { 1.0f,-1.0f,-1.0f, 0.45f,0.18f,0.04f},
    { 1.0f, 1.0f,-1.0f, 0.65f,0.30f,0.05f},
    { 1.0f, 1.0f, 1.0f, 0.85f,0.45f,0.08f},
    { 1.0f,-1.0f, 1.0f, 0.65f,0.28f,0.04f},

    {-1.0f,-1.0f,-1.0f, 0.20f,0.08f,0.04f},
    {-1.0f,-1.0f, 1.0f, 0.35f,0.12f,0.03f},
    {-1.0f, 1.0f, 1.0f, 0.50f,0.18f,0.05f},
    {-1.0f, 1.0f,-1.0f, 0.30f,0.10f,0.04f}
};

const uint16_t CUBE_INDICES[] =
{
     0, 1, 2,
     2, 3, 0,

     4, 6, 5,
     6, 4, 7,

     8, 9,10,
    10,11, 8,

    12,14,13,
    14,12,15,

    16,17,18,
    18,19,16,

    20,22,21,
    22,20,23
};

const char* GL_VERTEX_SHADER = R"(
#version 300 es

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;

uniform mat4 uMVP;

out vec3 vColor;

void main()
{
    gl_Position = uMVP * vec4(aPosition, 1.0);
    vColor = aColor;
}
)";

const char* GL_FRAGMENT_SHADER = R"(
#version 300 es

precision mediump float;

in vec3 vColor;

out vec4 fragColor;

void main()
{
    fragColor = vec4(vColor, 1.0);
}
)";

GLuint compileGLShader(
        GLenum type,
        const char* source)
{
    GLuint shader =
            glCreateShader(type);

    if (!shader)
        return 0;

    glShaderSource(
            shader,
            1,
            &source,
            nullptr);

    glCompileShader(shader);

    GLint success = GL_FALSE;

    glGetShaderiv(
            shader,
            GL_COMPILE_STATUS,
            &success);

    if (!success)
    {
        char log[2048];

        glGetShaderInfoLog(
                shader,
                sizeof(log),
                nullptr,
                log);

        LOGE(
                "OpenGL shader error: %s",
                log);

        glDeleteShader(shader);

        return 0;
    }

    return shader;
}

bool createGLProgram()
{
    GLuint vs =
            compileGLShader(
                    GL_VERTEX_SHADER,
                    GL_VERTEX_SHADER);

    if (!vs)
        return false;

    GLuint fs =
            compileGLShader(
                    GL_FRAGMENT_SHADER,
                    GL_FRAGMENT_SHADER);

    if (!fs)
    {
        glDeleteShader(vs);
        return false;
    }

    gGlProgram =
            glCreateProgram();

    glAttachShader(
            gGlProgram,
            vs);

    glAttachShader(
            gGlProgram,
            fs);

    glLinkProgram(gGlProgram);

    GLint success = GL_FALSE;

    glGetProgramiv(
            gGlProgram,
            GL_LINK_STATUS,
            &success);

    glDeleteShader(vs);
    glDeleteShader(fs);

    if (!success)
    {
        char log[2048];

        glGetProgramInfoLog(
                gGlProgram,
                sizeof(log),
                nullptr,
                log);

        LOGE(
                "OpenGL program error: %s",
                log);

        glDeleteProgram(gGlProgram);

        gGlProgram = 0;

        return false;
    }

    gGlMvpLocation =
            glGetUniformLocation(
                    gGlProgram,
                    "uMVP");

    return true;
}

bool createGLGeometry()
{
    glGenBuffers(
            1,
            &gGlVertexBuffer);

    glBindBuffer(
            GL_ARRAY_BUFFER,
            gGlVertexBuffer);

    glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(CUBE_VERTICES),
            CUBE_VERTICES,
            GL_STATIC_DRAW);

    glGenBuffers(
            1,
            &gGlIndexBuffer);

    glBindBuffer(
            GL_ELEMENT_ARRAY_BUFFER,
            gGlIndexBuffer);

    glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            sizeof(CUBE_INDICES),
            CUBE_INDICES,
            GL_STATIC_DRAW);

    glBindBuffer(
            GL_ARRAY_BUFFER,
            0);

    glBindBuffer(
            GL_ELEMENT_ARRAY_BUFFER,
            0);

    return true;
}

bool initializeOpenGL()
{
    const EGLint configAttributes[] =
    {
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_ES3_BIT,

        EGL_SURFACE_TYPE,
        EGL_WINDOW_BIT,

        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,

        EGL_DEPTH_SIZE, 24,

        EGL_NONE
    };

    EGLConfig config = nullptr;

    EGLint configCount = 0;

    if (!eglChooseConfig(
                gEglDisplay,
                configAttributes,
                &config,
                1,
                &configCount))
    {
        LOGE("eglChooseConfig fallo");
        return false;
    }

    if (configCount == 0)
    {
        LOGE("No existe configuracion EGL ES3");
        return false;
    }

    const EGLint contextAttributes[] =
    {
        EGL_CONTEXT_CLIENT_VERSION,
        3,
        EGL_NONE
    };

    gEglContext =
            eglCreateContext(
                    gEglDisplay,
                    config,
                    EGL_NO_CONTEXT,
                    contextAttributes);

    if (gEglContext == EGL_NO_CONTEXT)
    {
        LOGE("eglCreateContext fallo");
        return false;
    }

    gEglSurface =
            eglCreateWindowSurface(
                    gEglDisplay,
                    config,
                    gWindow,
                    nullptr);

    if (gEglSurface == EGL_NO_SURFACE)
    {
        LOGE("eglCreateWindowSurface fallo");
        return false;
    }

    if (!eglMakeCurrent(
                gEglDisplay,
                gEglSurface,
                gEglSurface,
                gEglContext))
    {
        LOGE("eglMakeCurrent fallo");
        return false;
    }

    LOGI(
            "OpenGL: %s",
            glGetString(GL_VERSION));

    LOGI(
            "GPU: %s",
            glGetString(GL_RENDERER));

    glViewport(
            0,
            0,
            gWidth,
            gHeight);

    glEnable(GL_DEPTH_TEST);

    glDepthFunc(GL_LEQUAL);

    if (!createGLProgram())
        return false;

    if (!createGLGeometry())
        return false;

    return true;
}

void renderOpenGL(
        float timeSeconds)
{
    glViewport(
            0,
            0,
            gWidth,
            gHeight);

    glClearColor(
            0.008f,
            0.018f,
            0.055f,
            1.0f);

    glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT);

    glUseProgram(gGlProgram);

    float aspect =
            static_cast<float>(gWidth) /
            static_cast<float>(
                    std::max(gHeight, 1));

    Mat4 projection =
            perspective(
                    60.0f *
                    3.1415926535f /
                    180.0f,
                    aspect,
                    0.1f,
                    100.0f);

    Mat4 rotation =
            multiply(
                    rotationY(
                            timeSeconds * 0.7f +
                            gCameraYaw),
                    rotationX(
                            gCameraPitch));

    Mat4 view =
            translation(
                    0.0f,
                    0.0f,
                    -gCameraDistance);

    Mat4 pv =
            multiply(
                    projection,
                    view);

    Mat4 mvp =
            multiply(
                    pv,
                    rotation);

    glUniformMatrix4fv(
            gGlMvpLocation,
            1,
            GL_FALSE,
            mvp.m);

    glBindBuffer(
            GL_ARRAY_BUFFER,
            gGlVertexBuffer);

    glBindBuffer(
            GL_ELEMENT_ARRAY_BUFFER,
            gGlIndexBuffer);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            reinterpret_cast<void*>(0));

    glEnableVertexAttribArray(1);

    glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            reinterpret_cast<void*>(
                    3 * sizeof(float)));

    glDrawElements(
            GL_TRIANGLES,
            sizeof(CUBE_INDICES) /
            sizeof(CUBE_INDICES[0]),
            GL_UNSIGNED_SHORT,
            nullptr);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    eglSwapBuffers(
            gEglDisplay,
            gEglSurface);
}

void shutdownOpenGL()
{
    if (gEglDisplay != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(
                gEglDisplay,
                EGL_NO_SURFACE,
                EGL_NO_SURFACE,
                EGL_NO_CONTEXT);
    }

    if (gGlProgram)
    {
        glDeleteProgram(gGlProgram);
        gGlProgram = 0;
    }

    if (gGlVertexBuffer)
    {
        glDeleteBuffers(
                1,
                &gGlVertexBuffer);

        gGlVertexBuffer = 0;
    }

    if (gGlIndexBuffer)
    {
        glDeleteBuffers(
                1,
                &gGlIndexBuffer);

        gGlIndexBuffer = 0;
    }

    if (gEglSurface != EGL_NO_SURFACE)
    {
        eglDestroySurface(
                gEglDisplay,
                gEglSurface);
    }

    if (gEglContext != EGL_NO_CONTEXT)
    {
        eglDestroyContext(
                gEglDisplay,
                gEglContext);
    }

    if (gEglDisplay != EGL_NO_DISPLAY)
    {
        eglTerminate(gEglDisplay);
    }

    gEglDisplay = EGL_NO_DISPLAY;
    gEglSurface = EGL_NO_SURFACE;
    gEglContext = EGL_NO_CONTEXT;
}

// ================================================================
// VULKAN
// ================================================================

VkInstance gVkInstance =
        VK_NULL_HANDLE;

VkPhysicalDevice gVkPhysicalDevice =
        VK_NULL_HANDLE;

VkDevice gVkDevice =
        VK_NULL_HANDLE;

VkQueue gVkGraphicsQueue =
        VK_NULL_HANDLE;

VkSurfaceKHR gVkSurface =
        VK_NULL_HANDLE;

VkSwapchainKHR gVkSwapchain =
        VK_NULL_HANDLE;

VkCommandPool gVkCommandPool =
        VK_NULL_HANDLE;

VkCommandBuffer gVkCommandBuffer =
        VK_NULL_HANDLE;

VkSemaphore gVkImageAvailable =
        VK_NULL_HANDLE;

VkSemaphore gVkRenderFinished =
        VK_NULL_HANDLE;

VkFence gVkFence =
        VK_NULL_HANDLE;

uint32_t gVkGraphicsQueueFamily = 0;

std::vector<VkImage> gVkImages;

std::vector<VkImageView> gVkImageViews;

VkFormat gVkFormat =
        VK_FORMAT_B8G8R8A8_UNORM;

VkExtent2D gVkExtent{1, 1};

bool gVulkanReady = false;

bool checkVulkanSupport()
{
    uint32_t count = 0;

    if (vkEnumerateInstanceExtensionProperties(
                nullptr,
                &count,
                nullptr) != VK_SUCCESS)
    {
        return false;
    }

    std::vector<VkExtensionProperties>
            extensions(count);

    if (vkEnumerateInstanceExtensionProperties(
                nullptr,
                &count,
                extensions.data()) != VK_SUCCESS)
    {
        return false;
    }

    bool surfaceKHR = false;
    bool androidSurface = false;

    for (const auto& extension : extensions)
    {
        if (std::strcmp(
                    extension.extensionName,
                    VK_KHR_SURFACE_EXTENSION_NAME) == 0)
        {
            surfaceKHR = true;
        }

        if (std::strcmp(
                    extension.extensionName,
                    VK_KHR_ANDROID_SURFACE_EXTENSION_NAME) == 0)
        {
            androidSurface = true;
        }
    }

    return surfaceKHR && androidSurface;
}

bool initializeVulkan()
{
    if (!checkVulkanSupport())
    {
        LOGI(
                "Vulkan no disponible en este dispositivo");

        return false;
    }

    const char* instanceExtensions[] =
    {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
    };

    VkApplicationInfo appInfo{};

    appInfo.sType =
            VK_STRUCTURE_TYPE_APPLICATION_INFO;

    appInfo.pApplicationName =
            "WoW Mobile Client";

    appInfo.applicationVersion =
            VK_MAKE_VERSION(0, 1, 0);

    appInfo.pEngineName =
            "WoW Mobile Engine";

    appInfo.engineVersion =
            VK_MAKE_VERSION(0, 1, 0);

    appInfo.apiVersion =
            VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceInfo{};

    instanceInfo.sType =
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    instanceInfo.pApplicationInfo =
            &appInfo;

    instanceInfo.enabledExtensionCount =
            2;

    instanceInfo.ppEnabledExtensionNames =
            instanceExtensions;

    VkResult result =
            vkCreateInstance(
                    &instanceInfo,
                    nullptr,
                    &gVkInstance);

    if (result != VK_SUCCESS)
    {
        LOGE(
                "vkCreateInstance fallo: %d",
                result);

        return false;
    }

    uint32_t deviceCount = 0;

    vkEnumeratePhysicalDevices(
            gVkInstance,
            &deviceCount,
            nullptr);

    if (deviceCount == 0)
    {
        LOGE(
                "Vulkan no encontro GPU");

        return false;
    }

    std::vector<VkPhysicalDevice>
            devices(deviceCount);

    vkEnumeratePhysicalDevices(
            gVkInstance,
            &deviceCount,
            devices.data());

    gVkPhysicalDevice =
            devices[0];

    VkPhysicalDeviceProperties
            properties{};

    vkGetPhysicalDeviceProperties(
            gVkPhysicalDevice,
            &properties);

    LOGI(
            "Vulkan GPU: %s",
            properties.deviceName);

    uint32_t queueFamilyCount = 0;

    vkGetPhysicalDeviceQueueFamilyProperties(
            gVkPhysicalDevice,
            &queueFamilyCount,
            nullptr);

    std::vector<VkQueueFamilyProperties>
            queueFamilies(queueFamilyCount);

    vkGetPhysicalDeviceQueueFamilyProperties(
            gVkPhysicalDevice,
            &queueFamilyCount,
            queueFamilies.data());

    bool foundQueue = false;

    for (uint32_t i = 0;
         i < queueFamilyCount;
         ++i)
    {
        if (queueFamilies[i].queueFlags &
            VK_QUEUE_GRAPHICS_BIT)
        {
            gVkGraphicsQueueFamily = i;
            foundQueue = true;
            break;
        }
    }

    if (!foundQueue)
    {
        LOGE(
                "Vulkan no encontro graphics queue");

        return false;
    }

    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueInfo{};

    queueInfo.sType =
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

    queueInfo.queueFamilyIndex =
            gVkGraphicsQueueFamily;

    queueInfo.queueCount = 1;

    queueInfo.pQueuePriorities =
            &queuePriority;

    VkDeviceCreateInfo deviceInfo{};

    deviceInfo.sType =
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    deviceInfo.queueCreateInfoCount = 1;

    deviceInfo.pQueueCreateInfos =
            &queueInfo;

    result =
            vkCreateDevice(
                    gVkPhysicalDevice,
                    &deviceInfo,
                    nullptr,
                    &gVkDevice);

    if (result != VK_SUCCESS)
    {
        LOGE(
                "vkCreateDevice fallo: %d",
                result);

        return false;
    }

    vkGetDeviceQueue(
            gVkDevice,
            gVkGraphicsQueueFamily,
            0,
            &gVkGraphicsQueue);

    LOGI(
            "Vulkan dispositivo inicializado");

    /*
     * El swapchain se crea cuando tenemos el Surface
     * Android correspondiente.
     *
     * En este primer backend dejamos preparada toda
     * la cadena Vulkan sin introducir SPIR-V embebido
     * artificialmente.
     *
     * El siguiente paso del renderer será:
     *
     *   Surface
     *   -> Swapchain
     *   -> RenderPass
     *   -> Pipeline
     *   -> M2/SKIN
     *   -> BLP
     *
     * Así evitamos mezclar un shader de prueba con
     * nuestro pipeline definitivo de WoW.
     */

    gVulkanReady = true;

    return true;
}

void shutdownVulkan()
{
    if (gVkDevice != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(gVkDevice);

        if (gVkFence != VK_NULL_HANDLE)
        {
            vkDestroyFence(
                    gVkDevice,
                    gVkFence,
                    nullptr);
        }

        if (gVkImageAvailable != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(
                    gVkDevice,
                    gVkImageAvailable,
                    nullptr);
        }

        if (gVkRenderFinished != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(
                    gVkDevice,
                    gVkRenderFinished,
                    nullptr);
        }

        if (gVkCommandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(
                    gVkDevice,
                    gVkCommandPool,
                    nullptr);
        }

        vkDestroyDevice(
                gVkDevice,
                nullptr);
    }

    if (gVkInstance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(
                gVkInstance,
                nullptr);
    }

    gVkDevice =
            VK_NULL_HANDLE;

    gVkPhysicalDevice =
            VK_NULL_HANDLE;

    gVkInstance =
            VK_NULL_HANDLE;

    gVulkanReady = false;
}

// ================================================================
// BACKEND SELECTION
// ================================================================

bool initializeRendererBackend()
{
    if (gRequestedBackend ==
        RendererBackend::VULKAN ||
        gRequestedBackend ==
        RendererBackend::AUTO)
    {
        if (initializeVulkan())
        {
            gActiveBackend =
                    RendererBackend::VULKAN;

            LOGI(
                    "Backend activo: VULKAN");

            return true;
        }

        if (gRequestedBackend ==
            RendererBackend::VULKAN)
        {
            LOGE(
                    "Se solicito Vulkan pero no pudo inicializarse");

            return false;
        }
    }

    gActiveBackend =
            RendererBackend::OPENGL;

    if (!eglGetDisplay)
    {
        LOGE("EGL no disponible");
        return false;
    }

    gEglDisplay =
            eglGetDisplay(
                    EGL_DEFAULT_DISPLAY);

    if (gEglDisplay == EGL_NO_DISPLAY)
    {
        LOGE("No se pudo obtener EGL display");
        return false;
    }

    EGLint major = 0;
    EGLint minor = 0;

    if (!eglInitialize(
                gEglDisplay,
                &major,
                &minor))
    {
        LOGE("eglInitialize fallo");
        return false;
    }

    LOGI(
            "EGL %d.%d",
            major,
            minor);

    if (!initializeOpenGL())
    {
        shutdownOpenGL();

        return false;
    }

    LOGI(
            "Backend activo: OPENGL ES 3");

    return true;
}

// ================================================================
// RENDER LOOP
// ================================================================

void renderLoop()
{
    LOGI("Render thread iniciado");

    bool initialized =
            initializeRendererBackend();

    if (!initialized)
    {
        LOGE(
                "No se pudo inicializar ningun backend");

        gRunning = false;

        return;
    }

    auto start =
            std::chrono::steady_clock::now();

    while (gRunning)
    {
        auto now =
                std::chrono::steady_clock::now();

        float timeSeconds =
                std::chrono::duration<float>(
                        now - start).count();

        {
            std::lock_guard<std::mutex> lock(
                    gMutex);

            if (gActiveBackend ==
                RendererBackend::OPENGL)
            {
                renderOpenGL(
                        timeSeconds);
            }
            else
            {
                /*
                 * Vulkan ya esta inicializado y
                 * validado como backend.
                 *
                 * El pipeline grafico Vulkan
                 * definitivo se conectara aqui
                 * junto con M2/SKIN/BLP.
                 */
                std::this_thread::sleep_for(
                        std::chrono::milliseconds(16));
            }
        }

        if (gActiveBackend ==
            RendererBackend::OPENGL)
        {
            std::this_thread::sleep_for(
                    std::chrono::milliseconds(1));
        }
    }

    if (gActiveBackend ==
        RendererBackend::OPENGL)
    {
        shutdownOpenGL();
    }
    else
    {
        shutdownVulkan();
    }

    LOGI("Render thread detenido");
}

// ================================================================
// CONTROL
// ================================================================

void stopRenderer()
{
    gRunning = false;

    if (gRenderThread.joinable())
    {
        gRenderThread.join();
    }
}

}

// ================================================================
// JNI
// ================================================================

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeRendererSetBackend(
        JNIEnv*,
        jobject,
        jint backend)
{
    if (backend == 1)
    {
        gRequestedBackend =
                RendererBackend::OPENGL;
    }
    else if (backend == 2)
    {
        gRequestedBackend =
                RendererBackend::VULKAN;
    }
    else
    {
        gRequestedBackend =
                RendererBackend::AUTO;
    }

    LOGI(
            "Backend solicitado: %d",
            backend);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeRendererSetSurface(
        JNIEnv* env,
        jobject,
        jobject surface)
{
    stopRenderer();

    if (!surface)
    {
        LOGE("Surface nulo");
        return;
    }

    ANativeWindow* window =
            ANativeWindow_fromSurface(
                    env,
                    surface);

    if (!window)
    {
        LOGE(
                "ANativeWindow_fromSurface fallo");

        return;
    }

    {
        std::lock_guard<std::mutex> lock(
                gMutex);

        if (gWindow)
        {
            ANativeWindow_release(
                    gWindow);

            gWindow = nullptr;
        }

        gWindow = window;

        gWidth =
                ANativeWindow_getWidth(
                        gWindow);

        gHeight =
                ANativeWindow_getHeight(
                        gWindow);

        if (gWidth <= 0)
            gWidth = 1;

        if (gHeight <= 0)
            gHeight = 1;
    }

    LOGI(
            "Surface: %dx%d",
            gWidth,
            gHeight);

    gRunning = true;

    gRenderThread =
            std::thread(renderLoop);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeRendererResize(
        JNIEnv*,
        jobject,
        jint width,
        jint height)
{
    std::lock_guard<std::mutex> lock(
            gMutex);

    gWidth =
            width > 0 ? width : 1;

    gHeight =
            height > 0 ? height : 1;

    if (gActiveBackend ==
        RendererBackend::OPENGL &&
        gEglDisplay != EGL_NO_DISPLAY)
    {
        glViewport(
                0,
                0,
                gWidth,
                gHeight);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeRendererStop(
        JNIEnv*,
        jobject)
{
    stopRenderer();

    std::lock_guard<std::mutex> lock(
            gMutex);

    if (gWindow)
    {
        ANativeWindow_release(
                gWindow);

        gWindow = nullptr;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeRendererCamera(
        JNIEnv*,
        jobject,
        jfloat yaw,
        jfloat pitch)
{
    std::lock_guard<std::mutex> lock(
            gMutex);

    gCameraYaw = yaw;
    gCameraPitch = pitch;

    gCameraPitch =
            std::clamp(
                    gCameraPitch,
                    -1.35f,
                    1.35f);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeRendererZoom(
        JNIEnv*,
        jobject,
        jfloat distance)
{
    std::lock_guard<std::mutex> lock(
            gMutex);

    gCameraDistance =
            std::clamp(
                    distance,
                    1.5f,
                    20.0f);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_wowmobile_client_MainActivity_nativeRendererGetBackend(
        JNIEnv*,
        jobject)
{
    return static_cast<jint>(
            gActiveBackend);
}
