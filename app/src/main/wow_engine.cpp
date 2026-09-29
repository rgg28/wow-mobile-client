#include <jni.h>
#include <android/log.h>

#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#define LOG_TAG "WoWMobile"

#define LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


// ============================================================
// ESTADO GLOBAL DEL MOTOR
// ============================================================

static JavaVM* g_vm = nullptr;

static jobject g_activity = nullptr;

static std::mutex g_mutex;

static std::string g_wowFolderUri;

static std::string g_username;

static std::atomic<bool> g_running(false);

static std::atomic<bool> g_connected(false);

static std::string g_selectedCharacter;

static float g_joystickX = 0.0f;
static float g_joystickY = 0.0f;

static bool g_modL1Activo = false;
static bool g_modR1Activo = false;


// ============================================================
// JNI / ENV
// ============================================================

static JNIEnv* getJNIEnv(
        bool& attached
) {
    attached = false;

    if (g_vm == nullptr) {
        return nullptr;
    }

    JNIEnv* env = nullptr;

    jint result =
            g_vm->GetEnv(
                    reinterpret_cast<void**>(&env),
                    JNI_VERSION_1_6
            );

    if (result == JNI_OK) {
        return env;
    }

    if (result != JNI_EDETACHED) {
        return nullptr;
    }

    if (g_vm->AttachCurrentThread(
            &env,
            nullptr
    ) != JNI_OK) {
        return nullptr;
    }

    attached = true;

    return env;
}

static void releaseJNIEnv(
        bool attached
) {
    if (attached && g_vm != nullptr) {
        g_vm->DetachCurrentThread();
    }
}


// ============================================================
// OBTENER CLASE DE LA ACTIVIDAD
// ============================================================

static jclass getActivityClass(
        JNIEnv* env
) {
    if (env == nullptr ||
        g_activity == nullptr) {
        return nullptr;
    }

    return env->GetObjectClass(
            g_activity
    );
}


// ============================================================
// LLAMADA JAVA: vfsExists()
// ============================================================

static bool vfsExists(
        const std::string& path
) {
    bool attached = false;

    JNIEnv* env =
            getJNIEnv(attached);

    if (env == nullptr) {
        return false;
    }

    jclass clazz =
            getActivityClass(env);

    if (clazz == nullptr) {
        releaseJNIEnv(attached);
        return false;
    }

    jmethodID method =
            env->GetMethodID(
                    clazz,
                    "vfsExists",
                    "(Ljava/lang/String;)Z"
            );

    if (method == nullptr) {
        env->DeleteLocalRef(clazz);
        releaseJNIEnv(attached);
        return false;
    }

    jstring jpath =
            env->NewStringUTF(
                    path.c_str()
            );

    jboolean result =
            env->CallBooleanMethod(
                    g_activity,
                    method,
                    jpath
            );

    env->DeleteLocalRef(jpath);
    env->DeleteLocalRef(clazz);

    releaseJNIEnv(attached);

    return result == JNI_TRUE;
}


// ============================================================
// LLAMADA JAVA: vfsIsDirectory()
// ============================================================

static bool vfsIsDirectory(
        const std::string& path
) {
    bool attached = false;

    JNIEnv* env =
            getJNIEnv(attached);

    if (env == nullptr) {
        return false;
    }

    jclass clazz =
            getActivityClass(env);

    if (clazz == nullptr) {
        releaseJNIEnv(attached);
        return false;
    }

    jmethodID method =
            env->GetMethodID(
                    clazz,
                    "vfsIsDirectory",
                    "(Ljava/lang/String;)Z"
            );

    if (method == nullptr) {
        env->DeleteLocalRef(clazz);
        releaseJNIEnv(attached);
        return false;
    }

    jstring jpath =
            env->NewStringUTF(
                    path.c_str()
            );

    jboolean result =
            env->CallBooleanMethod(
                    g_activity,
                    method,
                    jpath
            );

    env->DeleteLocalRef(jpath);
    env->DeleteLocalRef(clazz);

    releaseJNIEnv(attached);

    return result == JNI_TRUE;
}


// ============================================================
// LLAMADA JAVA: vfsList()
// ============================================================

static std::vector<std::string> vfsList(
        const std::string& path
) {
    std::vector<std::string> result;

    bool attached = false;

    JNIEnv* env =
            getJNIEnv(attached);

    if (env == nullptr) {
        return result;
    }

    jclass clazz =
            getActivityClass(env);

    if (clazz == nullptr) {
        releaseJNIEnv(attached);
        return result;
    }

    jmethodID method =
            env->GetMethodID(
                    clazz,
                    "vfsList",
                    "(Ljava/lang/String;)Ljava/util/List;"
            );

    if (method == nullptr) {
        env->DeleteLocalRef(clazz);
        releaseJNIEnv(attached);
        return result;
    }

    jstring jpath =
            env->NewStringUTF(
                    path.c_str()
            );

    jobject listObject =
            env->CallObjectMethod(
                    g_activity,
                    method,
                    jpath
            );

    env->DeleteLocalRef(jpath);

    if (listObject == nullptr) {
        env->DeleteLocalRef(clazz);
        releaseJNIEnv(attached);
        return result;
    }

    jclass listClass =
            env->FindClass(
                    "java/util/List"
            );

    if (listClass == nullptr) {
        env->DeleteLocalRef(listObject);
        env->DeleteLocalRef(clazz);
        releaseJNIEnv(attached);
        return result;
    }

    jmethodID sizeMethod =
            env->GetMethodID(
                    listClass,
                    "size",
                    "()I"
            );

    jmethodID getMethod =
            env->GetMethodID(
                    listClass,
                    "get",
                    "(I)Ljava/lang/Object;"
            );

    if (sizeMethod == nullptr ||
        getMethod == nullptr) {

        env->DeleteLocalRef(listClass);
        env->DeleteLocalRef(listObject);
        env->DeleteLocalRef(clazz);

        releaseJNIEnv(attached);

        return result;
    }

    jint size =
            env->CallIntMethod(
                    listObject,
                    sizeMethod
            );

    for (jint i = 0; i < size; ++i) {

        jobject item =
                env->CallObjectMethod(
                        listObject,
                        getMethod,
                        i
                );

        if (item == nullptr) {
            continue;
        }

        jstring stringItem =
                static_cast<jstring>(item);

        const char* chars =
                env->GetStringUTFChars(
                        stringItem,
                        nullptr
                );

        if (chars != nullptr) {
            result.emplace_back(chars);

            env->ReleaseStringUTFChars(
                    stringItem,
                    chars
            );
        }

        env->DeleteLocalRef(item);
    }

    env->DeleteLocalRef(listClass);
    env->DeleteLocalRef(listObject);
    env->DeleteLocalRef(clazz);

    releaseJNIEnv(attached);

    return result;
}


// ============================================================
// LLAMADA JAVA: vfsReadFile()
// ============================================================

static std::vector<uint8_t> vfsReadFile(
        const std::string& path
) {
    std::vector<uint8_t> result;

    bool attached = false;

    JNIEnv* env =
            getJNIEnv(attached);

    if (env == nullptr) {
        return result;
    }

    jclass clazz =
            getActivityClass(env);

    if (clazz == nullptr) {
        releaseJNIEnv(attached);
        return result;
    }

    jmethodID method =
            env->GetMethodID(
                    clazz,
                    "vfsReadFile",
                    "(Ljava/lang/String;)[B"
            );

    if (method == nullptr) {
        env->DeleteLocalRef(clazz);
        releaseJNIEnv(attached);
        return result;
    }

    jstring jpath =
            env->NewStringUTF(
                    path.c_str()
            );

    jbyteArray data =
            static_cast<jbyteArray>(
                    env->CallObjectMethod(
                            g_activity,
                            method,
                            jpath
                    )
            );

    env->DeleteLocalRef(jpath);

    if (data == nullptr) {
        env->DeleteLocalRef(clazz);
        releaseJNIEnv(attached);
        return result;
    }

    jsize length =
            env->GetArrayLength(data);

    if (length > 0) {
        result.resize(
                static_cast<size_t>(length)
        );

        env->GetByteArrayRegion(
                data,
                0,
                length,
                reinterpret_cast<jbyte*>(
                        result.data()
                )
        );
    }

    env->DeleteLocalRef(data);
    env->DeleteLocalRef(clazz);

    releaseJNIEnv(attached);

    return result;
}


// ============================================================
// MOTOR: INICIALIZACIÓN
// ============================================================

static void engineInit() {
    std::lock_guard<std::mutex> lock(
            g_mutex
    );

    g_running = true;

    LOGI(
            "WoW Mobile Engine inicializado"
    );
}


// ============================================================
// MOTOR: CARPETA WOW
// ============================================================

static void engineSetWowFolder(
        const std::string& uri
) {
    std::lock_guard<std::mutex> lock(
            g_mutex
    );

    g_wowFolderUri = uri;

    LOGI(
            "Carpeta WoW configurada: %s",
            g_wowFolderUri.c_str()
    );
}


// ============================================================
// CONVERTIR BYTES A HEX
// ============================================================

static std::string bytesToHex(
        const std::vector<uint8_t>& data,
        size_t maxBytes
) {
    std::ostringstream stream;

    size_t count =
            data.size();

    if (count > maxBytes) {
        count = maxBytes;
    }

    for (size_t i = 0; i < count; ++i) {

        char buffer[4];

        std::snprintf(
                buffer,
                sizeof(buffer),
                "%02X",
                data[i]
        );

        if (i > 0) {
            stream << ' ';
        }

        stream << buffer;
    }

    return stream.str();
}


// ============================================================
// PRUEBA COMPLETA DEL VFS
// ============================================================

static std::string engineTestVfs() {

    std::ostringstream out;

    out << "=== WOW MOBILE CLIENT / VFS TEST ===\n";
    out << "\n";

    if (g_wowFolderUri.empty()) {
        out << "VFS_ERROR\n";
        out << "[ERROR] No hay carpeta WoW configurada.\n";
        return out.str();
    }

    out << "[INFO] URI SAF configurada.\n";
    out << "\n";


    // --------------------------------------------------------
    // DIRECTORIOS ESPERADOS
    // --------------------------------------------------------

    const char* directories[] = {
            "cameras",
            "character",
            "creatures",
            "Data",
            "Interface",
            "WTF"
    };

    int directoriesFound = 0;

    out << "--- DIRECTORIOS ---\n";

    for (const char* directory :
            directories) {

        bool exists =
                vfsIsDirectory(directory);

        if (exists) {
            ++directoriesFound;

            out << "[OK] Directorio: "
                << directory
                << "\n";
        } else {
            out << "[INFO] No encontrado: "
                << directory
                << "\n";
        }
    }

    out << "\n";


    // --------------------------------------------------------
    // LISTADO DE RAÍZ
    // --------------------------------------------------------

    out << "--- RAÍZ DEL CLIENTE ---\n";

    std::vector<std::string> rootFiles =
            vfsList("");

    if (rootFiles.empty()) {

        out << "[ERROR] El VFS no pudo listar la raíz "
               "o la carpeta está vacía.\n";

    } else {

        out << "[OK] Elementos encontrados: "
            << rootFiles.size()
            << "\n";

        const size_t maxDisplay = 80;

        for (size_t i = 0;
             i < rootFiles.size() &&
             i < maxDisplay;
             ++i) {

            out << "  "
                << rootFiles[i]
                << "\n";
        }

        if (rootFiles.size() > maxDisplay) {
            out << "  ...\n";
        }
    }

    out << "\n";


    // --------------------------------------------------------
    // ARCHIVOS PEQUEÑOS / IMPORTANTES
    // --------------------------------------------------------

    const char* testFiles[] = {
            "realmlist.wtf",
            "Data/common.MPQ",
            "Data/expansion.MPQ",
            "Data/World.MPQ",
            "Data/lichking.MPQ"
    };

    int filesFound = 0;
    int filesRead = 0;

    out << "--- PRUEBA DE ARCHIVOS ---\n";

    for (const char* file :
            testFiles) {

        bool exists =
                vfsExists(file);

        if (!exists) {

            out << "[INFO] No encontrado: "
                << file
                << "\n";

            continue;
        }

        ++filesFound;

        out << "[OK] Encontrado: "
            << file
            << "\n";

        std::vector<uint8_t> data =
                vfsReadFile(file);

        if (data.empty()) {

            out << "[INFO] Existe pero no se "
                   "pudo leer mediante byte[].\n";

            continue;
        }

        ++filesRead;

        out << "[OK] Leído: "
            << file
            << " ("
            << data.size()
            << " bytes)\n";

        out << "[INFO] Primeros bytes: "
            << bytesToHex(data, 32)
            << "\n";
    }

    out << "\n";


    // --------------------------------------------------------
    // CONCLUSIÓN
    // --------------------------------------------------------

    out << "--- RESULTADO ---\n";

    bool rootOk =
            !rootFiles.empty();

    bool directoryOk =
            directoriesFound > 0;

    bool fileAccessOk =
            filesFound == 0 ||
            filesRead > 0;

    if (rootOk &&
        directoryOk &&
        fileAccessOk) {

        out << "VFS_OK\n";
        out << "[OK] El motor nativo puede acceder "
               "al cliente mediante el VFS Android.\n";

    } else {

        out << "VFS_ERROR\n";

        if (!rootOk) {
            out << "[ERROR] No se pudo listar la raíz.\n";
        }

        if (!directoryOk) {
            out << "[ERROR] No se encontraron "
                   "directorios del cliente.\n";
        }

        if (!fileAccessOk) {
            out << "[ERROR] Se encontraron archivos "
                   "pero no pudieron leerse.\n";
        }
    }

    out << "\n";

    out << "Directorios detectados: "
        << directoriesFound
        << "\n";

    out << "Archivos de prueba encontrados: "
        << filesFound
        << "\n";

    out << "Archivos de prueba leídos: "
        << filesRead
        << "\n";

    return out.str();
}


// ============================================================
// LOGIN - PROVISIONAL
// ============================================================

static void engineLogin(
        const std::string& username,
        const std::string& password
) {
    {
        std::lock_guard<std::mutex> lock(
                g_mutex
        );

        g_username = username;
        g_connected = false;
    }

    LOGI(
            "Solicitud de login para usuario: %s",
            username.c_str()
    );

    /*
     * IMPORTANTE:
     *
     * Este punto todavía NO implementa el protocolo
     * real de autenticación de WoW 3.3.5a.
     *
     * No se debe fingir que el servidor aceptó
     * usuario/contraseña.
     *
     * La implementación real deberá realizar:
     *
     *   AUTH_LOGON_CHALLENGE
     *   AUTH_LOGON_PROOF
     *   REALM_LIST
     *   CMSG_AUTH_SESSION
     *   SMSG_AUTH_RESPONSE
     *   CMSG_CHAR_ENUM
     *   SMSG_CHAR_ENUM
     *
     * etc.
     */

    LOGI(
            "Protocolo de autenticacion WoW 3.3.5a "
            "todavia no implementado."
    );

    (void)password;
}


// ============================================================
// INPUT
// ============================================================

static void engineTouchDown(
        float x,
        float y
) {
    LOGI(
            "Touch DOWN: %.1f %.1f",
            x,
            y
    );
}

static void engineTouchMove(
        float x,
        float y
) {
    LOGI(
            "Touch MOVE: %.1f %.1f",
            x,
            y
    );
}

static void engineTouchUp(
        float x,
        float y
) {
    LOGI(
            "Touch UP: %.1f %.1f",
            x,
            y
    );
}


// ============================================================
// JNI_OnLoad
// ============================================================

JNIEXPORT jint JNICALL
JNI_OnLoad(
        JavaVM* vm,
        void*
) {
    g_vm = vm;

    LOGI(
            "JNI_OnLoad - WoW Mobile"
    );

    return JNI_VERSION_1_6;
}


// ============================================================
// nativeInit()
// ============================================================

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeInit(
        JNIEnv* env,
        jobject activity
) {
    if (g_activity != nullptr) {
        env->DeleteGlobalRef(
                g_activity
        );

        g_activity = nullptr;
    }

    g_activity =
            env->NewGlobalRef(
                    activity
            );

    engineInit();
}


// ============================================================
// nativeSetWowFolder()
// ============================================================

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeSetWowFolder(
        JNIEnv* env,
        jobject,
        jstring uri
) {
    if (uri == nullptr) {
        engineSetWowFolder("");
        return;
    }

    const char* chars =
            env->GetStringUTFChars(
                    uri,
                    nullptr
            );

    if (chars == nullptr) {
        engineSetWowFolder("");
        return;
    }

    engineSetWowFolder(
            chars
    );

    env->ReleaseStringUTFChars(
            uri,
            chars
    );
}


// ============================================================
// nativeTestVfs()
// ============================================================

extern "C"
JNIEXPORT jstring JNICALL
Java_com_wowmobile_client_MainActivity_nativeTestVfs(
        JNIEnv* env,
        jobject
) {
    std::string result =
            engineTestVfs();

    return env->NewStringUTF(
            result.c_str()
    );
}


// ============================================================
// nativeLogin()
// ============================================================

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeLogin(
        JNIEnv* env,
        jobject,
        jstring username,
        jstring password
) {
    if (username == nullptr ||
        password == nullptr) {
        return;
    }

    const char* userChars =
            env->GetStringUTFChars(
                    username,
                    nullptr
            );

    const char* passChars =
            env->GetStringUTFChars(
                    password,
                    nullptr
            );

    if (userChars == nullptr ||
        passChars == nullptr) {

        if (userChars != nullptr) {
            env->ReleaseStringUTFChars(
                    username,
                    userChars
            );
        }

        if (passChars != nullptr) {
            env->ReleaseStringUTFChars(
                    password,
                    passChars
            );
        }

        return;
    }

    engineLogin(
            userChars,
            passChars
    );

    env->ReleaseStringUTFChars(
            username,
            userChars
    );

    env->ReleaseStringUTFChars(
            password,
            passChars
    );
}


// ============================================================
// nativeTouchDown()
// ============================================================

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeTouchDown(
        JNIEnv*,
        jobject,
        jfloat x,
        jfloat y
) {
    engineTouchDown(
            static_cast<float>(x),
            static_cast<float>(y)
    );
}


// ============================================================
// nativeTouchMove()
// ============================================================

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeTouchMove(
        JNIEnv*,
        jobject,
        jfloat x,
        jfloat y
) {
    engineTouchMove(
            static_cast<float>(x),
            static_cast<float>(y)
    );
}


// ============================================================
// nativeTouchUp()
// ============================================================

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeTouchUp(
        JNIEnv*,
        jobject,
        jfloat x,
        jfloat y
) {
    engineTouchUp(
            static_cast<float>(x),
            static_cast<float>(y)
    );
}
