#include <jni.h>

#include <android/log.h>

#include <atomic>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#define LOG_TAG "WoWMobile"

#define LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define LOGW(...) \
    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

#define LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


// ============================================================
// ESTADO DEL MOTOR
// ============================================================

static JavaVM* g_vm = nullptr;

static jobject g_activity = nullptr;

static std::mutex g_mutex;

static std::string g_wowFolderUri;

static std::string g_username;

static std::atomic<bool> g_running(false);

static int g_selectedCharacter = -1;

static float g_joystickX = 0.0f;
static float g_joystickY = 0.0f;

static bool g_l1 = false;
static bool g_r1 = false;


// ============================================================
// UTILIDAD JNI
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

    if (result ==
            JNI_EDETACHED) {

        if (g_vm->AttachCurrentThread(
                    &env,
                    nullptr
            ) != JNI_OK) {

            return nullptr;
        }

        attached = true;

        return env;
    }

    return nullptr;
}


static void releaseJNIEnv(
        bool attached
) {

    if (attached &&
        g_vm != nullptr) {

        g_vm->DetachCurrentThread();
    }
}


// ============================================================
// LLAMAR METODO JAVA
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
// VFS: EXISTS
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

    if (env->ExceptionCheck()) {

        env->ExceptionClear();

        releaseJNIEnv(attached);

        return false;
    }

    releaseJNIEnv(attached);

    return result == JNI_TRUE;
}


// ============================================================
// VFS: DIRECTORY
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

    if (env->ExceptionCheck()) {

        env->ExceptionClear();

        releaseJNIEnv(attached);

        return false;
    }

    releaseJNIEnv(attached);

    return result == JNI_TRUE;
}


// ============================================================
// VFS: LIST
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
                    "(Ljava/lang/String;)[Ljava/lang/String;"
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

    jobjectArray array =
            reinterpret_cast<jobjectArray>(
                    env->CallObjectMethod(
                            g_activity,
                            method,
                            jpath
                    )
            );

    env->DeleteLocalRef(jpath);

    if (env->ExceptionCheck()) {

        env->ExceptionClear();

        env->DeleteLocalRef(clazz);

        releaseJNIEnv(attached);

        return result;
    }

    if (array != nullptr) {

        jsize count =
                env->GetArrayLength(
                        array
                );

        for (jsize i = 0;
             i < count;
             ++i) {

            jstring item =
                    reinterpret_cast<jstring>(
                            env->GetObjectArrayElement(
                                    array,
                                    i
                            )
                    );

            if (item != nullptr) {

                const char* chars =
                        env->GetStringUTFChars(
                                item,
                                nullptr
                        );

                if (chars != nullptr) {

                    result.emplace_back(
                            chars
                    );

                    env->ReleaseStringUTFChars(
                            item,
                            chars
                    );
                }

                env->DeleteLocalRef(
                        item
                );
            }
        }

        env->DeleteLocalRef(array);
    }

    env->DeleteLocalRef(clazz);

    releaseJNIEnv(attached);

    return result;
}


// ============================================================
// VFS: READ FILE
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

    jbyteArray array =
            reinterpret_cast<jbyteArray>(
                    env->CallObjectMethod(
                            g_activity,
                            method,
                            jpath
                    )
            );

    env->DeleteLocalRef(jpath);

    if (env->ExceptionCheck()) {

        env->ExceptionClear();

        env->DeleteLocalRef(clazz);

        releaseJNIEnv(attached);

        return result;
    }

    if (array != nullptr) {

        jsize size =
                env->GetArrayLength(
                        array
                );

        if (size > 0) {

            result.resize(
                    static_cast<size_t>(size)
            );

            env->GetByteArrayRegion(
                    array,
                    0,
                    size,
                    reinterpret_cast<jbyte*>(
                            result.data()
                    )
            );
        }

        env->DeleteLocalRef(array);
    }

    env->DeleteLocalRef(clazz);

    releaseJNIEnv(attached);

    return result;
}


// ============================================================
// MOTOR
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


static void engineSetWowFolder(
        const std::string& uri
) {

    std::lock_guard<std::mutex> lock(
            g_mutex
    );

    g_wowFolderUri = uri;

    LOGI(
            "WoW VFS root: %s",
            g_wowFolderUri.c_str()
    );
}


static void engineLogin(
        const std::string& username,
        const std::string& password
) {

    std::lock_guard<std::mutex> lock(
            g_mutex
    );

    g_username = username;

    /*
     * Todavía no enviamos autenticación.
     *
     * Esta función queda preparada para implementar:
     *
     * AUTH_LOGON_CHALLENGE
     * AUTH_LOGON_PROOF
     * REALM_LIST
     * CMSG_AUTH_SESSION
     *
     * posteriormente.
     */

    LOGI(
            "Login solicitado para usuario: %s",
            username.c_str()
    );

    LOGI(
            "Contraseña recibida (%zu caracteres)",
            password.size()
    );
}


static void engineSelectCharacter(
        int index
) {

    std::lock_guard<std::mutex> lock(
            g_mutex
    );

    g_selectedCharacter = index;

    LOGI(
            "Personaje seleccionado: %d",
            index
    );
}


static void engineTouch(
        int action,
        float x,
        float y
) {

    LOGI(
            "Touch action=%d x=%.2f y=%.2f",
            action,
            x,
            y
    );
}


static void engineJoystick(
        float x,
        float y
) {

    const float length =
            std::sqrt(
                    x * x +
                    y * y
            );

    if (length > 1.0f) {

        x /= length;
        y /= length;
    }

    g_joystickX = x;
    g_joystickY = y;

    LOGI(
            "Joystick x=%.2f y=%.2f",
            x,
            y
    );
}


static void engineSpell(
        int spellId
) {

    LOGI(
            "Spell solicitado: %d",
            spellId
    );
}


static void engineJump() {

    LOGI(
            "Jump solicitado"
    );
}


// ============================================================
// PRUEBA DEL VFS
// ============================================================

static void engineTestVfs() {

    LOGI(
            "========================================"
    );

    LOGI(
            "INICIANDO PRUEBA DEL VFS"
    );

    LOGI(
            "========================================"
    );

    if (g_wowFolderUri.empty()) {

        LOGE(
                "VFS ERROR: no hay carpeta seleccionada"
        );

        return;
    }

    LOGI(
            "Root URI: %s",
            g_wowFolderUri.c_str()
    );


    // --------------------------------------------------------
    // Directorio raíz
    // --------------------------------------------------------

    LOGI(
            "Listado de la raiz:"
    );

    std::vector<std::string> rootFiles =
            vfsList("");

    for (const std::string& item :
            rootFiles) {

        LOGI(
                "  %s",
                item.c_str()
        );
    }


    // --------------------------------------------------------
    // Directorios conocidos
    // --------------------------------------------------------

    const char* directories[] = {
            "cameras",
            "character",
            "creatures",
            "Interface",
            "World",
            "Data"
    };

    for (const char* directory :
            directories) {

        bool exists =
                vfsExists(
                        directory
                );

        bool isDirectory =
                vfsIsDirectory(
                        directory
                );

        LOGI(
                "%s/ -> exists=%s directory=%s",
                directory,
                exists ? "YES" : "NO",
                isDirectory ? "YES" : "NO"
        );

        if (exists && isDirectory) {

            std::vector<std::string> files =
                    vfsList(directory);

            LOGI(
                    "  Contenido de %s/: %zu elementos",
                    directory,
                    files.size()
            );

            size_t limit =
                    files.size() < 10
                            ? files.size()
                            : 10;

            for (size_t i = 0;
                 i < limit;
                 ++i) {

                LOGI(
                        "    %s",
                        files[i].c_str()
                );
            }
        }
    }


    // --------------------------------------------------------
    // Buscar algunos archivos conocidos
    // --------------------------------------------------------

    const char* testFiles[] = {
            "realmlist.wtf",
            "Data/World.MPQ",
            "Data/common.MPQ",
            "Data/expansion.MPQ"
    };

    for (const char* file :
            testFiles) {

        bool exists =
                vfsExists(file);

        LOGI(
                "Archivo %s -> %s",
                file,
                exists ? "ENCONTRADO" : "no encontrado"
        );

        if (exists) {

            std::vector<uint8_t> data =
                    vfsReadFile(file);

            if (!data.empty()) {

                LOGI(
                        "  Lectura OK: %zu bytes",
                        data.size()
                );

                size_t preview =
                        data.size() < 16
                                ? data.size()
                                : 16;

                std::string hex;

                char buffer[4];

                for (size_t i = 0;
                     i < preview;
                     ++i) {

                    snprintf(
                            buffer,
                            sizeof(buffer),
                            "%02X ",
                            data[i]
                    );

                    hex += buffer;
                }

                LOGI(
                        "  Primeros bytes: %s",
                        hex.c_str()
                );

            } else {

                LOGW(
                        "  Archivo demasiado grande " 
                        "o no se pudo leer"
                );
            }
        }
    }


    LOGI(
            "========================================"
    );

    LOGI(
            "PRUEBA DEL VFS FINALIZADA"
    );

    LOGI(
            "========================================"
    );
}


// ============================================================
// JNI
// ============================================================

extern "C"
JNIEXPORT jint JNICALL
JNI_OnLoad(
        JavaVM* vm,
        void*
) {

    g_vm = vm;

    return JNI_VERSION_1_6;
}


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


extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeSetWowFolder(
        JNIEnv* env,
        jobject,
        jstring path
) {

    if (path == nullptr) {
        return;
    }

    const char* chars =
            env->GetStringUTFChars(
                    path,
                    nullptr
            );

    if (chars == nullptr) {
        return;
    }

    engineSetWowFolder(
            chars
    );

    env->ReleaseStringUTFChars(
            path,
            chars
    );
}


extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeTestVfs(
        JNIEnv*,
        jobject
) {

    engineTestVfs();
}


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

    const char* user =
            env->GetStringUTFChars(
                    username,
                    nullptr
            );

    const char* pass =
            env->GetStringUTFChars(
                    password,
                    nullptr
            );

    if (user != nullptr &&
        pass != nullptr) {

        engineLogin(
                user,
                pass
        );
    }

    if (user != nullptr) {

        env->ReleaseStringUTFChars(
                username,
                user
        );
    }

    if (pass != nullptr) {

        env->ReleaseStringUTFChars(
                password,
                pass
        );
    }
}


extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeSelectCharacter(
        JNIEnv*,
        jobject,
        jint index
) {

    engineSelectCharacter(
            static_cast<int>(index)
    );
}


extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeTouch(
        JNIEnv*,
        jobject,
        jint action,
        jfloat x,
        jfloat y
) {

    engineTouch(
            static_cast<int>(action),
            static_cast<float>(x),
            static_cast<float>(y)
    );
}


extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeJoystick(
        JNIEnv*,
        jobject,
        jfloat x,
        jfloat y
) {

    engineJoystick(
            static_cast<float>(x),
            static_cast<float>(y)
    );
}


extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeSpell(
        JNIEnv*,
        jobject,
        jint spellId
) {

    engineSpell(
            static_cast<int>(spellId)
    );
}


extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeJump(
        JNIEnv*,
        jobject
) {

    engineJump();
}
