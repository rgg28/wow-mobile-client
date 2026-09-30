#include <jni.h>
#include <string>
#include <android/native_window_jni.h>
#include "core/application.hpp"
#include "network/world_socket.hpp"
#include "auth/auth_handler.hpp"

// Instancias globales del núcleo que extraemos de WoWee
std::unique_ptr<WoWee::Application> g_Application = nullptr;
std::unique_ptr<WoWee::WorldSocket> g_WorldSocket = nullptr;

extern "C" JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_initWoWEngine(JNIEnv* env, jobject thiz, jobject surface) {
    // 1. Obtener la ventana nativa de Android a partir de la Surface de Java/Kotlin
    ANativeWindow* nativeWindow = ANativeWindow_fromSurface(env, surface);
    
    // 2. Inicializar el motor gráfico y la ventana pasando la ventana nativa
    g_Application = std::make_unique<WoWee::Application>();
    // Aquí WoWee inicializará Vulkan e ImGui sobre esta Surface
    g_Application->Initialize(nativeWindow); 
}

extern "C" JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_connectToServer(JNIEnv* env, jobject thiz, jstring host, jint port, jstring user, jstring pass) {
    std::string c_host = env->GetStringUTFChars(host, nullptr);
    std::string c_user = env->GetStringUTFChars(user, nullptr);
    std::string c_pass = env->GetStringUTFChars(pass, nullptr);

    // Inicializar el socket de red extraído de WoWee
    g_WorldSocket = std::make_unique<WoWee::WorldSocket>();
    
    // Lanzar el handshake de autenticación (SRP6 + Criptografía de OpenSSL)
    if (g_WorldSocket->Connect(c_host, port)) {
        WoWee::AuthHandler auth(g_WorldSocket.get());
        auth.StartAuthentication(c_user, c_pass);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_renderFrame(JNIEnv* env, jobject thiz) {
    if (g_Application) {
        // Ciclo de renderizado nativo: actualiza ImGui y procesa los comandos de Vulkan
        g_Application->RunFrame(); 
    }
}
