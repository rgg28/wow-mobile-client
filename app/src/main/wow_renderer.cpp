#include <jni.h>
#include <android/native_window_jni.h>
#include <android/log.h>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <atomic>
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

namespace {

struct Vertex {
    float x;
    float y;
    float z;

    float nx;
    float ny;
    float nz;
};

struct Mat4 {
    float m[16]{};
};

struct WorldVertex {
    float x;
    float y;
    float z;
};

std::mutex gMutex;

ANativeWindow* gWindow = nullptr;

EGLDisplay gDisplay = EGL_NO_DISPLAY;
EGLSurface gSurface = EGL_NO_SURFACE;
EGLContext gContext = EGL_NO_CONTEXT;

std::thread gRenderThread;

std::atomic<bool> gRunning(false);
std::atomic<bool> gSurfaceReady(false);

int gWidth = 1;
int gHeight = 1;

float gCameraX = 0.0f;
float gCameraY = 0.0f;
float gZoom = 1.0f;

float gLastTouchX = 0.0f;
float gLastTouchY = 0.0f;

bool gTouchInitialized = false;

std::vector<Vertex> gCharacterVertices;
std::vector<uint16_t> gCharacterIndices;

std::vector<Vertex> gWorldVertices;
std::vector<uint16_t> gWorldIndices;

bool gCharacterDirty = false;
bool gWorldDirty = false;

enum SceneType {
    SCENE_NONE = 0,
    SCENE_CHARACTER = 1,
    SCENE_WORLD = 2
};

SceneType gScene = SCENE_NONE;

GLuint gProgram = 0;

GLuint gVbo = 0;
GLuint gEbo = 0;

GLsizei gIndexCount = 0;

GLuint gWorldVbo = 0;
GLuint gWorldEbo = 0;

GLsizei gWorldIndexCount = 0;

float gWorldCenterX = 0.0f;
float gWorldCenterY = 0.0f;
float gWorldCenterZ = 0.0f;

float gWorldRadius = 100.0f;

// ------------------------------------------------------------
// MATRIX
// ------------------------------------------------------------

Mat4 identity() {

    Mat4 r{};

    r.m[0] = 1.0f;
    r.m[5] = 1.0f;
    r.m[10] = 1.0f;
    r.m[15] = 1.0f;

    return r;
}

Mat4 multiply(
        const Mat4& a,
        const Mat4& b
) {

    Mat4 r{};

    for (int c = 0; c < 4; ++c) {

        for (int row = 0; row < 4; ++row) {

            r.m[c * 4 + row] =
                    a.m[0 * 4 + row] * b.m[c * 4 + 0] +
                    a.m[1 * 4 + row] * b.m[c * 4 + 1] +
                    a.m[2 * 4 + row] * b.m[c * 4 + 2] +
                    a.m[3 * 4 + row] * b.m[c * 4 + 3];
        }
    }

    return r;
}

Mat4 perspective(
        float fov,
        float aspect,
        float nearPlane,
        float farPlane
) {

    Mat4 r{};

    float f =
            1.0f /
            std::tan(fov * 0.5f);

    r.m[0] = f / aspect;
    r.m[5] = f;

    r.m[10] =
            (farPlane + nearPlane) /
            (nearPlane - farPlane);

    r.m[11] = -1.0f;

    r.m[14] =
            (2.0f * farPlane * nearPlane) /
            (nearPlane - farPlane);

    return r;
}

Mat4 translation(
        float x,
        float y,
        float z
) {

    Mat4 r =
            identity();

    r.m[12] = x;
    r.m[13] = y;
    r.m[14] = z;

    return r;
}

Mat4 rotationY(float angle) {

    Mat4 r =
            identity();

    float c =
            std::cos(angle);

    float s =
            std::sin(angle);

    r.m[0] = c;
    r.m[2] = -s;

    r.m[8] = s;
    r.m[10] = c;

    return r;
}

Mat4 rotationX(float angle) {

    Mat4 r =
            identity();

    float c =
            std::cos(angle);

    float s =
            std::sin(angle);

    r.m[5] = c;
    r.m[6] = s;

    r.m[9] = -s;
    r.m[10] = c;

    return r;
}

// ------------------------------------------------------------
// SHADER
// ------------------------------------------------------------

GLuint compileShader(
        GLenum type,
        const char* source
) {

    GLuint shader =
            glCreateShader(type);

    glShaderSource(
            shader,
            1,
            &source,
            nullptr
    );

    glCompileShader(shader);

    GLint ok = 0;

    glGetShaderiv(
            shader,
            GL_COMPILE_STATUS,
            &ok
    );

    if (!ok) {

        char log[2048]{};

        glGetShaderInfoLog(
                shader,
                sizeof(log),
                nullptr,
                log
        );

        LOGE(
                "Shader error: %s",
                log
        );

        glDeleteShader(shader);

        return 0;
    }

    return shader;
}

bool createProgram() {

    const char* vertexSource = R"(
        #version 300 es

        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec3 aNormal;

        uniform mat4 uMVP;
        uniform mat4 uModel;

        out vec3 vNormal;

        void main() {

            gl_Position =
                uMVP *
                vec4(aPosition, 1.0);

            vNormal =
                mat3(uModel) *
                aNormal;
        }
    )";

    const char* fragmentSource = R"(
        #version 300 es

        precision mediump float;

        in vec3 vNormal;

        out vec4 fragColor;

        void main() {

            vec3 n =
                normalize(vNormal);

            vec3 light =
                normalize(
                    vec3(
                        -0.4,
                        0.8,
                        0.5
                    )
                );

            float diffuse =
                max(
                    dot(n, light),
                    0.0
                );

            float value =
                0.25 +
                diffuse * 0.75;

            fragColor =
                vec4(
                    value * 0.72,
                    value * 0.78,
                    value * 0.88,
                    1.0
                );
        }
    )";

    GLuint vs =
            compileShader(
                    GL_VERTEX_SHADER,
                    vertexSource
            );

    GLuint fs =
            compileShader(
                    GL_FRAGMENT_SHADER,
                    fragmentSource
            );

    if (!vs || !fs) {
        return false;
    }

    gProgram =
            glCreateProgram();

    glAttachShader(
            gProgram,
            vs
    );

    glAttachShader(
            gProgram,
            fs
    );

    glLinkProgram(
            gProgram
    );

    GLint linked = 0;

    glGetProgramiv(
            gProgram,
            GL_LINK_STATUS,
            &linked
    );

    glDeleteShader(vs);
    glDeleteShader(fs);

    if (!linked) {

        char log[2048]{};

        glGetProgramInfoLog(
                gProgram,
                sizeof(log),
                nullptr,
                log
        );

        LOGE(
                "Program link error: %s",
                log
        );

        glDeleteProgram(gProgram);
        gProgram = 0;

        return false;
    }

    return true;
}

// ------------------------------------------------------------
// BINARY HELPERS
// ------------------------------------------------------------

uint32_t readU32(
        const std::vector<uint8_t>& data,
        size_t p
) {

    if (p + 4 > data.size()) {
        return 0;
    }

    return
            static_cast<uint32_t>(data[p]) |
            (static_cast<uint32_t>(data[p + 1]) << 8) |
            (static_cast<uint32_t>(data[p + 2]) << 16) |
            (static_cast<uint32_t>(data[p + 3]) << 24);
}

uint16_t readU16(
        const std::vector<uint8_t>& data,
        size_t p
) {

    if (p + 2 > data.size()) {
        return 0;
    }

    return
            static_cast<uint16_t>(data[p]) |
            (static_cast<uint16_t>(data[p + 1]) << 8);
}

float readF32(
        const std::vector<uint8_t>& data,
        size_t p
) {

    if (p + 4 > data.size()) {
        return 0.0f;
    }

    uint32_t v =
            readU32(data, p);

    float f;

    std::memcpy(
            &f,
            &v,
            sizeof(float)
    );

    return f;
}

bool tagAt(
        const std::vector<uint8_t>& data,
        size_t p,
        const char* tag
) {

    if (p + 4 > data.size()) {
        return false;
    }

    return
            data[p] == tag[0] &&
            data[p + 1] == tag[1] &&
            data[p + 2] == tag[2] &&
            data[p + 3] == tag[3];
}

// ------------------------------------------------------------
// M2
// ------------------------------------------------------------

bool parseM2(
        const std::vector<uint8_t>& data
) {

    gCharacterVertices.clear();
    gCharacterIndices.clear();

    if (data.size() < 0x48) {
        return false;
    }

    if (!tagAt(data, 0, "MD20")) {
        LOGE("M2 no comienza con MD20");
        return false;
    }

    uint32_t nVertices =
            readU32(data, 0x40);

    uint32_t ofsVertices =
            readU32(data, 0x44);

    if (nVertices == 0 ||
            nVertices > 1000000) {

        return false;
    }

    if (ofsVertices >= data.size()) {
        return false;
    }

    const size_t vertexSize = 48;

    if (
            static_cast<uint64_t>(ofsVertices) +
            static_cast<uint64_t>(nVertices) *
            vertexSize >
            data.size()
    ) {

        LOGE("Vertices M2 fuera de rango");
        return false;
    }

    gCharacterVertices.reserve(
            nVertices
    );

    for (uint32_t i = 0;
         i < nVertices;
         ++i) {

        size_t p =
                ofsVertices +
                static_cast<size_t>(i) *
                vertexSize;

        Vertex v{};

        v.x =
                readF32(
                        data,
                        p + 0
                );

        v.y =
                readF32(
                        data,
                        p + 4
                );

        v.z =
                readF32(
                        data,
                        p + 8
                );

        v.nx =
                readF32(
                        data,
                        p + 20
                );

        v.ny =
                readF32(
                        data,
                        p + 24
                );

        v.nz =
                readF32(
                        data,
                        p + 28
                );

        gCharacterVertices.push_back(v);
    }

    return true;
}

// ------------------------------------------------------------
// SKIN
// ------------------------------------------------------------

bool parseSkin(
        const std::vector<uint8_t>& data
) {

    gCharacterIndices.clear();

    if (data.size() < 0x20) {
        return false;
    }

    if (!tagAt(data, 0, "SKIN")) {
        return false;
    }

    uint32_t nIndices =
            readU32(data, 0x04);

    uint32_t ofsIndices =
            readU32(data, 0x08);

    uint32_t nTriangles =
            readU32(data, 0x0C);

    uint32_t ofsTriangles =
            readU32(data, 0x10);

    if (nIndices == 0 ||
            nTriangles == 0) {

        return false;
    }

    if (
            static_cast<uint64_t>(ofsIndices) +
            static_cast<uint64_t>(nIndices) * 2 >
            data.size()
    ) {

        return false;
    }

    if (
            static_cast<uint64_t>(ofsTriangles) +
            static_cast<uint64_t>(nTriangles) * 2 >
            data.size()
    ) {

        return false;
    }

    std::vector<uint16_t> indices;

    indices.reserve(nIndices);

    for (uint32_t i = 0;
         i < nIndices;
         ++i) {

        indices.push_back(
                readU16(
                        data,
                        ofsIndices +
                        i * 2
                )
        );
    }

    gCharacterIndices.reserve(
            nTriangles
    );

    for (uint32_t i = 0;
         i < nTriangles;
         ++i) {

        uint16_t local =
                readU16(
                        data,
                        ofsTriangles +
                        i * 2
                );

        if (local >= indices.size()) {
            continue;
        }

        uint16_t real =
                indices[local];

        if (real >=
                gCharacterVertices.size()) {

            continue;
        }

        gCharacterIndices.push_back(
                real
        );
    }

    return
            gCharacterIndices.size() >= 3;
}

// ------------------------------------------------------------
// ADT / MCNK
// ------------------------------------------------------------

struct MCVT {
    float h[145]{};
};

bool findSubChunk(
        const std::vector<uint8_t>& data,
        size_t start,
        size_t end,
        const char* wanted,
        size_t& payload,
        size_t& size
) {

    if (end > data.size()) {
        end = data.size();
    }

    size_t p = start;

    while (p + 8 <= end) {

        uint32_t chunkSize =
                readU32(data, p + 4);

        size_t chunkEnd =
                p + 8 +
                static_cast<size_t>(chunkSize);

        if (chunkEnd > end ||
                chunkEnd > data.size()) {

            break;
        }

        if (tagAt(
                data,
                p,
                wanted
        )) {

            payload = p + 8;
            size = chunkSize;

            return true;
        }

        p = chunkEnd;
    }

    return false;
}

bool parseMCNK(
        const std::vector<uint8_t>& data,
        size_t mcnk,
        size_t mcnkEnd,
        int chunkX,
        int chunkY
) {

    /*
     * WotLK MCNK header is followed by:
     *
     * MCVT
     * MCNR
     * MCLY
     * ...
     *
     * MCVT contains 145 floats:
     *
     * 9x9 outer vertices
     * 8x8 inner vertices
     */

    size_t mcvtPayload = 0;
    size_t mcvtSize = 0;

    if (!findSubChunk(
            data,
            mcnk + 8,
            mcnkEnd,
            "MCVT",
            mcvtPayload,
            mcvtSize
    )) {

        return false;
    }

    if (mcvtSize < 145 * 4) {
        return false;
    }

    float baseZ = 0.0f;

    /*
     * MCNK header position field is around +0x68.
     * For the WotLK terrain format this is the base
     * height used together with MCVT.
     */

    if (mcnk + 0x70 <= data.size()) {

        baseZ =
                readF32(
                        data,
                        mcnk + 0x68
                );
    }

    MCVT heights{};

    for (int i = 0; i < 145; ++i) {

        heights.h[i] =
                readF32(
                        data,
                        mcvtPayload +
                        static_cast<size_t>(i) * 4
                );
    }

    /*
     * 9x9 V9 grid.
     *
     * MCVT is interleaved:
     *
     * row 0: 9 outer
     * row 1: 8 inner
     * row 2: 9 outer
     * row 3: 8 inner
     *
     * ...
     */

    const float chunkSize = 33.333333f;
    const float step = chunkSize / 8.0f;

    std::vector<WorldVertex> outer(
            81
    );

    std::vector<WorldVertex> inner(
            64
    );

    int index = 0;

    for (int y = 0; y < 9; ++y) {

        for (int x = 0; x < 9; ++x) {

            float wx =
                    (
                            chunkX * chunkSize
                            +
                            x * step
                    );

            float wz =
                    (
                            chunkY * chunkSize
                            +
                            y * step
                    );

            float wy =
                    baseZ +
                    heights.h[index++];

            outer[
                    y * 9 + x
            ] = {
                    wx,
                    wy,
                    wz
            };
        }

        if (y < 8) {

            for (int x = 0; x < 8; ++x) {

                float wx =
                        chunkX * chunkSize +
                        (x + 0.5f) * step;

                float wz =
                        chunkY * chunkSize +
                        (y + 0.5f) * step;

                float wy =
                        baseZ +
                        heights.h[index++];

                inner[
                        y * 8 + x
                ] = {
                        wx,
                        wy,
                        wz
                };
            }
        }
    }

    /*
     * Build only the V9 grid for this first world
     * renderer. The inner 8x8 data remains parsed,
     * while V9 gives us a stable terrain mesh.
     */

    uint16_t baseIndex =
            static_cast<uint16_t>(
                    gWorldVertices.size()
            );

    for (const auto& v : outer) {

        Vertex out{};

        out.x = v.x;
        out.y = v.y;
        out.z = v.z;

        /*
         * Temporary upward normal.
         * Proper MCNR normals can be added next.
         */

        out.nx = 0.0f;
        out.ny = 1.0f;
        out.nz = 0.0f;

        gWorldVertices.push_back(out);
    }

    for (int y = 0; y < 8; ++y) {

        for (int x = 0; x < 8; ++x) {

            uint16_t a =
                    baseIndex +
                    y * 9 +
                    x;

            uint16_t b =
                    baseIndex +
                    y * 9 +
                    x + 1;

            uint16_t c =
                    baseIndex +
                    (y + 1) * 9 +
                    x;

            uint16_t d =
                    baseIndex +
                    (y + 1) * 9 +
                    x + 1;

            gWorldIndices.push_back(a);
            gWorldIndices.push_back(c);
            gWorldIndices.push_back(b);

            gWorldIndices.push_back(b);
            gWorldIndices.push_back(c);
            gWorldIndices.push_back(d);
        }
    }

    return true;
}

bool parseADT(
        const std::vector<uint8_t>& data
) {

    gWorldVertices.clear();
    gWorldIndices.clear();

    if (data.size() < 8) {
        return false;
    }

    size_t p = 0;

    int chunkCounter = 0;

    while (p + 8 <= data.size()) {

        uint32_t size =
                readU32(
                        data,
                        p + 4
                );

        size_t end =
                p + 8 +
                static_cast<size_t>(size);

        if (end > data.size()) {
            break;
        }

        if (tagAt(
                data,
                p,
                "MCNK"
        )) {

            int chunkX =
                    chunkCounter % 16;

            int chunkY =
                    chunkCounter / 16;

            if (parseMCNK(
                    data,
                    p,
                    end,
                    chunkX,
                    chunkY
            )) {

                ++chunkCounter;
            }
        }

        p = end;
    }

    if (gWorldVertices.empty() ||
            gWorldIndices.empty()) {

        return false;
    }

    float minX = 1e30f;
    float minY = 1e30f;
    float minZ = 1e30f;

    float maxX = -1e30f;
    float maxY = -1e30f;
    float maxZ = -1e30f;

    for (const Vertex& v :
         gWorldVertices) {

        minX =
                std::min(minX, v.x);

        minY =
                std::min(minY, v.y);

        minZ =
                std::min(minZ, v.z);

        maxX =
                std::max(maxX, v.x);

        maxY =
                std::max(maxY, v.y);

        maxZ =
                std::max(maxZ, v.z);
    }

    gWorldCenterX =
            (minX + maxX) * 0.5f;

    gWorldCenterY =
            (minY + maxY) * 0.5f;

    gWorldCenterZ =
            (minZ + maxZ) * 0.5f;

    float dx =
            maxX - minX;

    float dy =
            maxY - minY;

    float dz =
            maxZ - minZ;

    gWorldRadius =
            std::max(
                    10.0f,
                    std::sqrt(
                            dx * dx +
                            dy * dy +
                            dz * dz
                    ) * 0.5f
            );

    LOGI(
            "ADT: chunks=%d vertices=%zu indices=%zu radius=%f",
            chunkCounter,
            gWorldVertices.size(),
            gWorldIndices.size(),
            gWorldRadius
    );

    return true;
}

// ------------------------------------------------------------
// GPU UPLOAD
// ------------------------------------------------------------

void uploadCharacter() {

    if (!gCharacterDirty) {
        return;
    }

    gCharacterDirty = false;

    if (gVbo) {
        glDeleteBuffers(
                1,
                &gVbo
        );
    }

    if (gEbo) {
        glDeleteBuffers(
                1,
                &gEbo
        );
    }

    glGenBuffers(
            1,
            &gVbo
    );

    glGenBuffers(
            1,
            &gEbo
    );

    glBindBuffer(
            GL_ARRAY_BUFFER,
            gVbo
    );

    glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                    gCharacterVertices.size() *
                    sizeof(Vertex)
            ),
            gCharacterVertices.data(),
            GL_STATIC_DRAW
    );

    glBindBuffer(
            GL_ELEMENT_ARRAY_BUFFER,
            gEbo
    );

    glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                    gCharacterIndices.size() *
                    sizeof(uint16_t)
            ),
            gCharacterIndices.data(),
            GL_STATIC_DRAW
    );

    gIndexCount =
            static_cast<GLsizei>(
                    gCharacterIndices.size()
            );
}

void uploadWorld() {

    if (!gWorldDirty) {
        return;
    }

    gWorldDirty = false;

    if (gWorldVbo) {
        glDeleteBuffers(
                1,
                &gWorldVbo
        );
    }

    if (gWorldEbo) {
        glDeleteBuffers(
                1,
                &gWorldEbo
        );
    }

    glGenBuffers(
            1,
            &gWorldVbo
    );

    glGenBuffers(
            1,
            &gWorldEbo
    );

    glBindBuffer(
            GL_ARRAY_BUFFER,
            gWorldVbo
    );

    glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                    gWorldVertices.size() *
                    sizeof(Vertex)
            ),
            gWorldVertices.data(),
            GL_STATIC_DRAW
    );

    glBindBuffer(
            GL_ELEMENT_ARRAY_BUFFER,
            gWorldEbo
    );

    glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                    gWorldIndices.size() *
                    sizeof(uint16_t)
            ),
            gWorldIndices.data(),
            GL_STATIC_DRAW
    );

    gWorldIndexCount =
            static_cast<GLsizei>(
                    gWorldIndices.size()
            );
}

// ------------------------------------------------------------
// EGL
// ------------------------------------------------------------

bool initEGL() {

    gDisplay =
            eglGetDisplay(
                    EGL_DEFAULT_DISPLAY
            );

    if (gDisplay == EGL_NO_DISPLAY) {
        LOGE("No EGL display");
        return false;
    }

    if (!eglInitialize(
            gDisplay,
            nullptr,
            nullptr
    )) {

        LOGE("eglInitialize failed");
        return false;
    }

    const EGLint configAttributes[] = {

            EGL_RENDERABLE_TYPE,
            EGL_OPENGL_ES3_BIT,

            EGL_SURFACE_TYPE,
            EGL_WINDOW_BIT,

            EGL_RED_SIZE,
            8,

            EGL_GREEN_SIZE,
            8,

            EGL_BLUE_SIZE,
            8,

            EGL_ALPHA_SIZE,
            8,

            EGL_DEPTH_SIZE,
            24,

            EGL_NONE
    };

    EGLConfig config;

    EGLint numConfigs = 0;

    if (!eglChooseConfig(
            gDisplay,
            configAttributes,
            &config,
            1,
            &numConfigs
    ) || numConfigs == 0) {

        LOGE("eglChooseConfig failed");
        return false;
    }

    const EGLint contextAttributes[] = {

            EGL_CONTEXT_CLIENT_VERSION,
            3,

            EGL_NONE
    };

    gContext =
            eglCreateContext(
                    gDisplay,
                    config,
                    EGL_NO_CONTEXT,
                    contextAttributes
            );

    if (gContext == EGL_NO_CONTEXT) {

        LOGE("eglCreateContext failed");

        return false;
    }

    gSurface =
            eglCreateWindowSurface(
                    gDisplay,
                    config,
                    gWindow,
                    nullptr
            );

    if (gSurface == EGL_NO_SURFACE) {

        LOGE(
                "eglCreateWindowSurface failed"
        );

        return false;
    }

    if (!eglMakeCurrent(
            gDisplay,
            gSurface,
            gSurface,
            gContext
    )) {

        LOGE(
                "eglMakeCurrent failed"
        );

        return false;
    }

    eglSwapInterval(
            gDisplay,
            1
    );

    if (!createProgram()) {
        return false;
    }

    glEnable(
            GL_DEPTH_TEST
    );

    glEnable(
            GL_CULL_FACE
    );

    glCullFace(
            GL_BACK
    );

    glClearColor(
            0.015f,
            0.025f,
            0.055f,
            1.0f
    );

    return true;
}

void shutdownEGL() {

    if (gDisplay != EGL_NO_DISPLAY) {

        eglMakeCurrent(
                gDisplay,
                EGL_NO_SURFACE,
                EGL_NO_SURFACE,
                EGL_NO_CONTEXT
        );

        if (gSurface != EGL_NO_SURFACE) {

            eglDestroySurface(
                    gDisplay,
                    gSurface
            );
        }

        if (gContext != EGL_NO_CONTEXT) {

            eglDestroyContext(
                    gDisplay,
                    gContext
            );
        }

        eglTerminate(
                gDisplay
        );
    }

    gDisplay = EGL_NO_DISPLAY;
    gSurface = EGL_NO_SURFACE;
    gContext = EGL_NO_CONTEXT;

    if (gProgram) {

        glDeleteProgram(
                gProgram
        );

        gProgram = 0;
    }
}

// ------------------------------------------------------------
// FRAME
// ------------------------------------------------------------

void drawScene() {

    glViewport(
            0,
            0,
            gWidth,
            gHeight
    );

    glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT
    );

    if (!gProgram) {
        return;
    }

    uploadCharacter();
    uploadWorld();

    glUseProgram(
            gProgram
    );

    float aspect =
            static_cast<float>(gWidth) /
            static_cast<float>(
                    std::max(1, gHeight)
            );

    Mat4 projection =
            perspective(
                    1.0f,
                    aspect,
                    0.1f,
                    100000.0f
            );

    Mat4 model =
            identity();

    float cameraDistance;

    if (gScene == SCENE_WORLD) {

        cameraDistance =
                gWorldRadius *
                (1.4f + gZoom * 0.35f);

        model =
                translation(
                        -gWorldCenterX,
                        -gWorldCenterY,
                        -gWorldCenterZ
                );

    } else {

        cameraDistance =
                4.0f *
                (1.0f + gZoom * 0.25f);
    }

    Mat4 camera =
            translation(
                    0.0f,
                    -cameraDistance * 0.05f,
                    -cameraDistance
            );

    Mat4 rotY =
            rotationY(
                    gCameraX
            );

    Mat4 rotX =
            rotationX(
                    gCameraY
            );

    Mat4 view =
            multiply(
                    camera,
                    multiply(
                            rotX,
                            rotY
                    )
            );

    Mat4 mvp =
            multiply(
                    projection,
                    multiply(
                            view,
                            model
                    )
            );

    GLint mvpLocation =
            glGetUniformLocation(
                    gProgram,
                    "uMVP"
            );

    GLint modelLocation =
            glGetUniformLocation(
                    gProgram,
                    "uModel"
            );

    glUniformMatrix4fv(
            mvpLocation,
            1,
            GL_FALSE,
            mvp.m
    );

    glUniformMatrix4fv(
            modelLocation,
            1,
            GL_FALSE,
            model.m
    );

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    if (gScene == SCENE_CHARACTER &&
            gVbo &&
            gEbo &&
            gIndexCount > 0) {

        glBindBuffer(
                GL_ARRAY_BUFFER,
                gVbo
        );

        glBindBuffer(
                GL_ELEMENT_ARRAY_BUFFER,
                gEbo
        );

        glVertexAttribPointer(
                0,
                3,
                GL_FLOAT,
                GL_FALSE,
                sizeof(Vertex),
                reinterpret_cast<void*>(0)
        );

        glVertexAttribPointer(
                1,
                3,
                GL_FLOAT,
                GL_FALSE,
                sizeof(Vertex),
                reinterpret_cast<void*>(
                        3 * sizeof(float)
                )
        );

        glDrawElements(
                GL_TRIANGLES,
                gIndexCount,
                GL_UNSIGNED_SHORT,
                nullptr
        );
    }

    if (gScene == SCENE_WORLD &&
            gWorldVbo &&
            gWorldEbo &&
            gWorldIndexCount > 0) {

        glBindBuffer(
                GL_ARRAY_BUFFER,
                gWorldVbo
        );

        glBindBuffer(
                GL_ELEMENT_ARRAY_BUFFER,
                gWorldEbo
        );

        glVertexAttribPointer(
                0,
                3,
                GL_FLOAT,
                GL_FALSE,
                sizeof(Vertex),
                reinterpret_cast<void*>(0)
        );

        glVertexAttribPointer(
                1,
                3,
                GL_FLOAT,
                GL_FALSE,
                sizeof(Vertex),
                reinterpret_cast<void*>(
                        3 * sizeof(float)
                )
        );

        glDrawElements(
                GL_TRIANGLES,
                gWorldIndexCount,
                GL_UNSIGNED_SHORT,
                nullptr
        );
    }

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    eglSwapBuffers(
            gDisplay,
            gSurface
    );
}

// ------------------------------------------------------------
// RENDER THREAD
// ------------------------------------------------------------

void renderLoop() {

    LOGI("Render thread iniciado");

    if (!initEGL()) {

        LOGE(
                "No se pudo iniciar EGL"
        );

        gRunning = false;

        return;
    }

    gSurfaceReady = true;

    while (gRunning) {

        {
            std::lock_guard<std::mutex> lock(
                    gMutex
            );

            if (gWidth < 1) {
                gWidth = 1;
            }

            if (gHeight < 1) {
                gHeight = 1;
            }

            drawScene();
        }

        std::this_thread::sleep_for(
                std::chrono::milliseconds(16)
        );
    }

    gSurfaceReady = false;

    shutdownEGL();

    LOGI("Render thread detenido");
}

} // namespace

// ============================================================
// JNI
// ============================================================

extern "C"
JNIEXPORT jint JNICALL
Java_com_wowmobile_client_MainActivity_nativeRendererGetBackend(
        JNIEnv*,
        jobject
) {

    return 1;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeRendererSetBackend(
        JNIEnv*,
        jobject,
        jint backend
) {

    LOGI(
            "Backend seleccionado: %d",
            backend
    );
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeRendererSetSurface(
        JNIEnv* env,
        jobject,
        jobject surface
) {

    std::lock_guard<std::mutex> lock(
            gMutex
    );

    if (gWindow) {

        ANativeWindow_release(
                gWindow
        );

        gWindow = nullptr;
    }

    gWindow =
            ANativeWindow_fromSurface(
                    env,
                    surface
            );

    if (!gWindow) {

        LOGE(
                "No se pudo obtener ANativeWindow"
        );

        return;
    }

    if (!gRunning) {

        gRunning = true;

        gRenderThread =
                std::thread(
                        renderLoop
                );
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeRendererResize(
        JNIEnv*,
        jobject,
        jint width,
        jint height
) {

    std::lock_guard<std::mutex> lock(
            gMutex
    );

    gWidth =
            std::max(
                    1,
                    static_cast<int>(width)
            );

    gHeight =
            std::max(
                    1,
                    static_cast<int>(height)
            );
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeRendererStop(
        JNIEnv*,
        jobject
) {

    if (!gRunning) {
        return;
    }

    gRunning = false;

    if (gRenderThread.joinable()) {

        gRenderThread.join();
    }

    std::lock_guard<std::mutex> lock(
            gMutex
    );

    if (gWindow) {

        ANativeWindow_release(
                gWindow
        );

        gWindow = nullptr;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeRendererCamera(
        JNIEnv*,
        jobject,
        jfloat x,
        jfloat y
) {

    std::lock_guard<std::mutex> lock(
            gMutex
    );

    if (!gTouchInitialized) {

        gLastTouchX = x;
        gLastTouchY = y;

        gTouchInitialized = true;

        return;
    }

    float dx =
            x - gLastTouchX;

    float dy =
            y - gLastTouchY;

    gLastTouchX = x;
    gLastTouchY = y;

    gCameraX +=
            dx * 0.008f;

    gCameraY +=
            dy * 0.005f;

    gCameraY =
            std::clamp(
                    gCameraY,
                    -1.3f,
                    1.3f
            );
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeRendererZoom(
        JNIEnv*,
        jobject,
        jfloat delta
) {

    std::lock_guard<std::mutex> lock(
            gMutex
    );

    gZoom +=
            delta * 0.08f;

    gZoom =
            std::clamp(
                    gZoom,
                    0.1f,
                    8.0f
            );
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_wowmobile_client_MainActivity_nativeRendererLoadCharacter(
        JNIEnv* env,
        jobject,
        jbyteArray m2Array,
        jbyteArray skinArray
) {

    if (!m2Array) {
        return JNI_FALSE;
    }

    jsize m2Size =
            env->GetArrayLength(
                    m2Array
            );

    std::vector<uint8_t> m2(
            static_cast<size_t>(m2Size)
    );

    env->GetByteArrayRegion(
            m2Array,
            0,
            m2Size,
            reinterpret_cast<jbyte*>(
                    m2.data()
            )
    );

    std::vector<uint8_t> skin;

    if (skinArray) {

        jsize skinSize =
                env->GetArrayLength(
                        skinArray
                );

        skin.resize(
                static_cast<size_t>(skinSize)
        );

        env->GetByteArrayRegion(
                skinArray,
                0,
                skinSize,
                reinterpret_cast<jbyte*>(
                        skin.data()
                )
        );
    }

    std::lock_guard<std::mutex> lock(
            gMutex
    );

    if (!parseM2(m2)) {

        LOGE(
                "No se pudo interpretar M2"
        );

        return JNI_FALSE;
    }

    if (!skin.empty()) {

        if (!parseSkin(skin)) {

            LOGE(
                    "No se pudo interpretar SKIN"
            );

            return JNI_FALSE;
        }

    } else {

        /*
         * Fallback: triángulos consecutivos.
         * Solamente para comprobar modelos que no
         * tengan SKIN disponible.
         */

        gCharacterIndices.clear();

        for (size_t i = 0;
             i + 2 < gCharacterVertices.size();
             i += 3) {

            gCharacterIndices.push_back(
                    static_cast<uint16_t>(i)
            );

            gCharacterIndices.push_back(
                    static_cast<uint16_t>(i + 1)
            );

            gCharacterIndices.push_back(
                    static_cast<uint16_t>(i + 2)
            );
        }
    }

    if (gCharacterIndices.empty()) {

        return JNI_FALSE;
    }

    gScene =
            SCENE_CHARACTER;

    gCharacterDirty =
            true;

    gZoom = 1.0f;

    gCameraX = 0.0f;
    gCameraY = 0.0f;

    gTouchInitialized = false;

    LOGI(
            "Personaje cargado: vertices=%zu indices=%zu",
            gCharacterVertices.size(),
            gCharacterIndices.size()
    );

    return JNI_TRUE;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_wowmobile_client_MainActivity_nativeRendererLoadWorld(
        JNIEnv* env,
        jobject,
        jbyteArray adtArray
) {

    if (!adtArray) {
        return JNI_FALSE;
    }

    jsize size =
            env->GetArrayLength(
                    adtArray
            );

    if (size <= 0 ||
            size > 64 * 1024 * 1024) {

        return JNI_FALSE;
    }

    std::vector<uint8_t> data(
            static_cast<size_t>(size)
    );

    env->GetByteArrayRegion(
            adtArray,
            0,
            size,
            reinterpret_cast<jbyte*>(
                    data.data()
            )
    );

    std::lock_guard<std::mutex> lock(
            gMutex
    );

    if (!parseADT(data)) {

        LOGE(
                "No se pudo interpretar ADT"
        );

        return JNI_FALSE;
    }

    gScene =
            SCENE_WORLD;

    gWorldDirty =
            true;

    gCameraX = 0.0f;
    gCameraY = -0.55f;

    gZoom = 1.0f;

    gTouchInitialized = false;

    LOGI(
            "Mundo ADT preparado"
    );

    return JNI_TRUE;
}
