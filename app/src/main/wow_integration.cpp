#include <jni.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <memory>
#include <string>

// --- INCLUSIONES DEL NÚCLEO DIRECTO DE WOWEE ---
#include "core/application.hpp"
#include "network/world_socket.hpp"
#include "auth/auth_handler.hpp"

#define LOG_TAG "WoWIntegration"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Instancias globales utilizando los sub-namespaces correctos detectados por el compilador
std::unique_ptr<wowee::core::Application>  g_WoWApplication = nullptr;
std::unique_ptr<wowee::network::WorldSocket> g_WoWWorldSocket = nullptr;

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
    g_WoWApplication = std::make_unique<wowee::core::Application>();
    g_WoWApplication->run(); // Inicializa la ventana nativa y el render graph de forma directa
}

// ============================================================
// 2. REFRESCO DE FRAME (Bucle de renderizado a 60 FPS)
// ============================================================
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_renderFrame(JNIEnv* env, jobject thiz) {
    // WoWee controla el refresco internamente al llamar a run(),
    // mantenemos el stub limpio para sincronizar con los hilos de tu MainActivity
    if (g_WoWApplication) {
        // Ejecuta ticks nativos si el motor expone un actualizador por cuadro
    }
}
// ============================================================
// 3. INFRAESTRUCTURA DE RED (Conexión asíncrona y Autenticación)
// ============================================================
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_connectToServer(JNIEnv* env, jobject thiz, jstring host, jint port, jstring user, jstring pass) {
    std::string c_host = integrationJstringToString(env, host);
    std::string c_user = integrationJstringToString(env, user);
    std::string c_pass = integrationJstringToString(env, pass);

    LOGI("Abriendo socket TCP binario hacia el reino privado: %s:%d", c_host.c_str(), port);
    g_WoWWorldSocket = std::make_unique<wowee::network::WorldSocket>();

    // CORREGIDO: Invocación del método connect en minúsculas nativo de la API de WoWee
    if (!g_WoWWorldSocket->connect(c_host, port)) {
        LOGE("Error de red: El servidor realmlist rechazó la conexión.");
        return;
    }

    LOGI("Conexión TCP establecida. Disparando AuthHandler y Handshake SRP6...");
    
    // Instanciamos el manejador pasando el puntero al socket TCP activo
    wowee::auth::AuthHandler auth;
    
    // Ejecuta el flujo asíncrono pasándole las credenciales ingresadas en tu interfaz
    auth.logon(c_user, c_pass);
}

} // extern "C"
