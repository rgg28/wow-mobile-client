#include <jni.h>
#include <string>
#include <android/log.h>

#define LOG_TAG "WoWAndroidCore"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Función auxiliar para leer e interpretar el realmlist.wtf en el futuro
void procesarRealmlist(const std::string& rutaData) {
    LOGI("Buscando realmlist.wtf en la ruta: %s", rutaData.c_str());
    
    // Aquí implementaremos la apertura del archivo de texto realmlist.wtf
    // Ejemplo lógico: buscar "set realmlist ://servidor.com" para extraer la IP
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wow_client_MainActivity_iniciarMotorWoW(JNIEnv *env, jobject thiz, jstring ruta_data) {
    // Convertir el string de Java a string nativo de C++
    const char *rutaNative = env->GetStringUTFChars(ruta_data, nullptr);
    std::string rutaDataStr(rutaNative);
    
    LOGI("Motor C++ iniciado correctamente de forma nativa.");
    LOGI("Accediendo al directorio de datos: %s", rutaDataStr.c_str());

    // Ejecutar la lectura del realmlist
    procesarRealmlist(rutaDataStr);

    // Aquí se inicializará el bucle principal de renderizado (SDL2/OpenGL),
    // la inyección de los joysticks virtuales y la conexión TCP al servidor

    // Liberar la memoria del string de Java
    env->ReleaseStringUTFChars(ruta_data, rutaNative);
}
