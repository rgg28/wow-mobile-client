#include <jni.h>
#include <android/log.h>
#include <android/native_window_jni.h>

#include <memory>
#include <string>

#include "core/application.hpp"
#include "network/world_socket.hpp"
#include "auth/auth_handler.hpp"

#define LOG_TAG "WoWIntegration"

#define LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


// ============================================================
// INSTANCIAS GLOBALES
// ============================================================

static std::unique_ptr<wowee::core::Application> g_WoWApplication;
static std::unique_ptr<wowee::network::WorldSocket> g_WoWWorldSocket;


// ============================================================
// JNI STRING -> std::string
// ============================================================

static std::string integrationJstringToString(
    JNIEnv* env,
    jstring value)
{
    if (env == nullptr || value == nullptr) {
        return {};
    }

    const char* chars =
        env->GetStringUTFChars(value, nullptr);

    if (chars == nullptr) {
        return {};
    }

    std::string result(chars);

    env->ReleaseStringUTFChars(value, chars);

    return result;
}


// ============================================================
// JNI
// ============================================================

extern "C" {


// ============================================================
// 1. INICIALIZAR MOTOR WOW
// ============================================================

JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_initWoWEngine(
    JNIEnv* env,
    jobject thiz,
    jobject surface)
{
    (void)thiz;

    if (env == nullptr) {
        LOGE("JNIEnv es nulo.");
        return;
    }

    if (surface == nullptr) {
        LOGE("La Surface de Android es nula.");
        return;
    }

    ANativeWindow* nativeWindow =
        ANativeWindow_fromSurface(env, surface);

    if (nativeWindow == nullptr) {
        LOGE("No se pudo obtener ANativeWindow.");
        return;
    }

    LOGI("Inicializando motor nativo WoWee...");

    try {

        if (!g_WoWApplication) {

            g_WoWApplication =
                std::make_unique<wowee::core::Application>();

            LOGI("Application creada correctamente.");
        }

        /*
         * El motor WoWee controla su inicialización
         * mediante Application::run().
         */
        g_WoWApplication->run();

        LOGI("Application::run() ejecutado correctamente.");

    }
    catch (...) {

        LOGE(
            "Excepción durante la inicialización de WoWee."
        );
    }

    ANativeWindow_release(nativeWindow);
}


// ============================================================
// 2. RENDER FRAME
// ============================================================

JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_renderFrame(
    JNIEnv* env,
    jobject thiz)
{
    (void)env;
    (void)thiz;

    if (!g_WoWApplication) {
        return;
    }

    /*
     * Actualmente Application::run() controla
     * el ciclo principal del motor.
     *
     * No llamamos a una función de tick/render
     * que no esté definida por WoWee.
     */
}


// ============================================================
// 3. CONECTAR AL WORLD SERVER
// ============================================================

JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_connectToServer(
    JNIEnv* env,
    jobject thiz,
    jstring host,
    jint port,
    jstring user,
    jstring pass)
{
    (void)thiz;

    if (env == nullptr) {
        LOGE("JNIEnv es nulo.");
        return;
    }

    const std::string c_host =
        integrationJstringToString(env, host);

    const std::string c_user =
        integrationJstringToString(env, user);

    const std::string c_pass =
        integrationJstringToString(env, pass);

    if (c_host.empty()) {
        LOGE("Host vacío.");
        return;
    }

    if (c_user.empty()) {
        LOGE("Usuario vacío.");
        return;
    }

    if (c_pass.empty()) {
        LOGE("Contraseña vacía.");
        return;
    }

    if (port <= 0 || port > 65535) {
        LOGE(
            "Puerto inválido: %d",
            static_cast<int>(port)
        );
        return;
    }

    LOGI(
        "Conectando a %s:%d...",
        c_host.c_str(),
        static_cast<int>(port)
    );

    try {

        // ----------------------------------------------------
        // Crear WorldSocket
        // ----------------------------------------------------

        g_WoWWorldSocket =
            std::make_unique<wowee::network::WorldSocket>();

        LOGI("WorldSocket creado.");

        // ----------------------------------------------------
        // Conectar
        // ----------------------------------------------------

        if (!g_WoWWorldSocket->connect(
                c_host,
                static_cast<int>(port)))
        {
            LOGE(
                "WorldSocket::connect() falló."
            );

            g_WoWWorldSocket.reset();

            return;
        }

        LOGI(
            "Conexión TCP establecida con %s:%d.",
            c_host.c_str(),
            static_cast<int>(port)
        );

        // ----------------------------------------------------
        // Autenticación
        // ----------------------------------------------------

        /*
         * Utilizamos exactamente la API que existe
         * en el wow_integration.cpp del proyecto:
         *
         *     AuthHandler auth;
         *     auth.logon(user, password);
         *
         * NO hacemos sustituciones automáticas aquí.
         */

        wowee::auth::AuthHandler auth;

        LOGI("Iniciando autenticación...");

        auth.logon(
            c_user,
            c_pass
        );

        LOGI(
            "Solicitud de autenticación enviada."
        );

    }
    catch (...) {

        LOGE(
            "Excepción durante conexión/autenticación."
        );

        g_WoWWorldSocket.reset();
    }
}


// ============================================================
// 4. APAGAR MOTOR
// ============================================================

JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_shutdownWoW(
    JNIEnv* env,
    jobject thiz)
{
    (void)env;
    (void)thiz;

    LOGI("Cerrando WoW Mobile...");

    g_WoWWorldSocket.reset();

    g_WoWApplication.reset();

    LOGI("WoW Mobile cerrado.");
}

} // extern "C"
