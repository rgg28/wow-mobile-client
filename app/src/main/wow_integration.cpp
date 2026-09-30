#include <jni.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <memory>
#include <string>

// --- INCLUSIONES DE CABECERAS GLOBALES DEL MOTOR ---
#include "core/application.hpp"
#include "network/world_socket.hpp"
#include "auth/auth_handler.hpp"

#define LOG_TAG "WoWIntegration"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Instancias globales directas sin envoltorios de namespaces
std::unique_ptr<Application> g_WoWApplication = nullptr;
std::unique_ptr<WorldSocket> g_WoWWorldSocket = nullptr;

static std::string integrationJstringToString(JNIEnv* env, jstring value) {
    if (env == nullptr || value == nullptr) return {};
    const char* chars = env->GetStringUTFChars(value, nullptr);
    if (chars == nullptr) return {};
    std::string result(chars);
    env->ReleaseStringUTFChars(value, chars);
    return result;
}

extern "C" {

// Inicialización de la pantalla gráfica (Vulkan + ImGui nativo)
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_initWoWEngine(JNIEnv* env, jobject thiz, jobject surface) {
    if (surface == nullptr) {
        LOGE("La Surface de Android es nula. No se puede iniciar Vulkan.");
        return;
    }

    ANativeWindow* nativeWindow = ANativeWindow_fromSurface(env, surface);
    if (nativeWindow == nullptr) {
        LOGE("No se pudo obtener ANativeWindow desde la Surface.");
        return;
    }

    LOGI("Levantando el motor gráfico nativo sobre Vulkan...");
    g_WoWApplication = std::make_unique<Application>();
    g_WoWApplication->Initialize(nativeWindow);
}

// Bucle de renderizado asíncrono
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_renderFrame(JNIEnv* env, jobject thiz) {
    if (g_WoWApplication) {
        g_WoWApplication->RunFrame();
    }
}

// Socket de red TCP binario y Handshake de autenticación SRP6
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_connectToServer(JNIEnv* env, jobject thiz, jstring host, jint port, jstring user, jstring pass) {
    std::string c_host = integrationJstringToString(env, host);
    std::string c_user = integrationJstringToString(env, user);
    std::string c_pass = integrationJstringToString(env, pass);

    LOGI("Abriendo conexión asíncrona por socket hacia %s:%d", c_host.c_str(), port);
    g_WoWWorldSocket = std::make_unique<WorldSocket>();

    if (!g_WoWWorldSocket->Connect(c_host, port)) {
        LOGE("Error de red: El servidor rechazó la conexión TCP.");
        return;
    }

    LOGI("Conectado con éxito. Iniciando AuthHandler con criptografía OpenSSL...");
    AuthHandler auth(g_WoWWorldSocket.get());
    auth.StartAuthentication(c_user, c_pass);
}

} // extern "C"
