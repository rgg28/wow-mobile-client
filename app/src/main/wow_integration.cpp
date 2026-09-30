#include <jni.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <memory>
#include <string>

// --- INCLUSIONES DEL NÚCLEO EXTRAÍDO DE WOWEE ---
#include "core/application.hpp"
#include "network/world_socket.hpp"
#include "auth/auth_handler.hpp"

#define LOG_TAG "WoWIntegration"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Instancias globales aisladas para el renderizado y la red de WoWee
std::unique_ptr<WoWee::Application> g_WoWApplication = nullptr;
std::unique_ptr<WoWee::WorldSocket>   g_WoWWorldSocket = nullptr;

// Helpers locales para conversión de cadenas JNI
static std::string integrationJstringToString(JNIEnv* env, jstring value) {
    if (env == nullptr || value == nullptr) return {};
    const char* chars = env->GetStringUTFChars(value, nullptr);
    if (chars == nullptr) return {};
    std::string result(chars);
    env->ReleaseStringUTFChars(value, chars);
    return result;
}

extern "C" {

// ============================================================
// 1. CONTROL GRÁFICO (Inicialización de Vulkan + ImGui)
// ============================================================
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

    LOGI("Levantando el motor gráfico nativo de WoWee sobre Vulkan...");
    g_WoWApplication = std::make_unique<WoWee::Application>();
    g_WoWApplication->Initialize(nativeWindow);
    LOGI("Motor de renderizado Vulkan/ImGui listo.");
}

// ============================================================
// 2. REFRESCO DE FRAME (Llamado a 60 FPS desde Java/Kotlin)
// ============================================================
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_renderFrame(JNIEnv* env, jobject thiz) {
    if (g_WoWApplication) {
        g_WoWApplication->RunFrame();
    }
}

// ============================================================
// 3. SISTEMA DE RED (Conexión asíncrona y handshake SRP6)
// ============================================================
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_connectToServer(JNIEnv* env, jobject thiz, jstring host, jint port, jstring user, jstring pass) {
    std::string c_host = integrationJstringToString(env, host);
    std::string c_user = integrationJstringToString(env, user);
    std::string c_pass = integrationJstringToString(env, pass);

    LOGI("Conectando de forma directa al reino: %s:%d", c_host.c_str(), port);
    g_WoWWorldSocket = std::make_unique<WoWee::WorldSocket>();

    if (!g_WoWWorldSocket->Connect(c_host, port)) {
        LOGE("Error de red: No se pudo abrir el socket TCP.");
        return;
    }

    LOGI("Conexión establecida. Iniciando protocolo de autenticación con el servidor...");
    WoWee::AuthHandler auth(g_WoWWorldSocket.get());
    auth.StartAuthentication(c_user, c_pass);
}

} // extern "C"
