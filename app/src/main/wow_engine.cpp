#include <jni.h>
#include <android/log.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdint>
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
// ESTADO GLOBAL
// ============================================================

static JavaVM* g_vm = nullptr;

static jobject g_activity = nullptr;

static std::mutex g_mutex;

static std::string g_wowFolderUri;

static std::string g_username;

static std::atomic<bool> g_running(false);

static std::atomic<bool> g_connected(false);


// ============================================================
// JNI
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

    if (
            g_vm->AttachCurrentThread(
                    &env,
                    nullptr
            ) != JNI_OK
    ) {
        return nullptr;
    }

    attached = true;

    return env;
}

static void releaseJNIEnv(
        bool attached
) {
    if (
            attached &&
            g_vm != nullptr
    ) {
        g_vm->DetachCurrentThread();
    }
}

static jclass getActivityClass(
        JNIEnv* env
) {
    if (
            env == nullptr ||
            g_activity == nullptr
    ) {
        return nullptr;
    }

    return env->GetObjectClass(
            g_activity
    );
}


// ============================================================
// VFS: EXISTE
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

        env->DeleteLocalRef(
                clazz
        );

        releaseJNIEnv(attached);

        return false;
    }

    jstring jpath =
            env->NewStringUTF(
                    path.c_str()
            );

    jboolean value =
            env->CallBooleanMethod(
                    g_activity,
                    method,
                    jpath
            );

    env->DeleteLocalRef(
            jpath
    );

    env->DeleteLocalRef(
            clazz
    );

    releaseJNIEnv(attached);

    return value == JNI_TRUE;
}


// ============================================================
// VFS: DIRECTORIO
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

        env->DeleteLocalRef(
                clazz
        );

        releaseJNIEnv(attached);

        return false;
    }

    jstring jpath =
            env->NewStringUTF(
                    path.c_str()
            );

    jboolean value =
            env->CallBooleanMethod(
                    g_activity,
                    method,
                    jpath
            );

    env->DeleteLocalRef(
            jpath
    );

    env->DeleteLocalRef(
            clazz
    );

    releaseJNIEnv(attached);

    return value == JNI_TRUE;
}


// ============================================================
// VFS: LISTAR
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

        env->DeleteLocalRef(
                clazz
        );

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

    env->DeleteLocalRef(
            jpath
    );

    if (listObject == nullptr) {

        env->DeleteLocalRef(
                clazz
        );

        releaseJNIEnv(attached);

        return result;
    }

    jclass listClass =
            env->FindClass(
                    "java/util/List"
            );

    if (listClass == nullptr) {

        env->DeleteLocalRef(
                listObject
        );

        env->DeleteLocalRef(
                clazz
        );

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

    if (
            sizeMethod == nullptr ||
            getMethod == nullptr
    ) {

        env->DeleteLocalRef(
                listClass
        );

        env->DeleteLocalRef(
                listObject
        );

        env->DeleteLocalRef(
                clazz
        );

        releaseJNIEnv(attached);

        return result;
    }

    jint size =
            env->CallIntMethod(
                    listObject,
                    sizeMethod
            );

    for (
            jint i = 0;
            i < size;
            ++i
    ) {

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
                static_cast<jstring>(
                        item
                );

        const char* chars =
                env->GetStringUTFChars(
                        stringItem,
                        nullptr
                );

        if (chars != nullptr) {

            result.emplace_back(
                    chars
            );

            env->ReleaseStringUTFChars(
                    stringItem,
                    chars
            );
        }

        env->DeleteLocalRef(
                item
        );
    }

    env->DeleteLocalRef(
            listClass
    );

    env->DeleteLocalRef(
            listObject
    );

    env->DeleteLocalRef(
            clazz
    );

    releaseJNIEnv(attached);

    return result;
}


// ============================================================
// VFS: LEER ARCHIVO
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

        env->DeleteLocalRef(
                clazz
        );

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

    env->DeleteLocalRef(
            jpath
    );

    if (data == nullptr) {

        env->DeleteLocalRef(
                clazz
        );

        releaseJNIEnv(attached);

        return result;
    }

    jsize length =
            env->GetArrayLength(
                    data
            );

    if (length > 0) {

        result.resize(
                static_cast<size_t>(
                        length
                )
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

    env->DeleteLocalRef(
            data
    );

    env->DeleteLocalRef(
            clazz
    );

    releaseJNIEnv(attached);

    return result;
}


// ============================================================
// EXTENSIÓN
// ============================================================

static std::string getExtension(
        const std::string& name
) {
    size_t slash =
            name.find_last_of(
                    "/\\"
            );

    size_t dot =
            name.find_last_of(
                    '.'
            );

    if (
            dot == std::string::npos ||
            (
                    slash != std::string::npos &&
                    dot < slash
            )
    ) {
        return "";
    }

    std::string ext =
            name.substr(
                    dot + 1
            );

    std::transform(
            ext.begin(),
            ext.end(),
            ext.begin(),
            [](unsigned char c) {
                return static_cast<char>(
                        std::toupper(c)
                );
            }
    );

    return ext;
}


// ============================================================
// CONTADORES DEL ESCANEO
// ============================================================

struct ScanStats {

    uint64_t directories = 0;

    uint64_t files = 0;

    uint64_t mpq = 0;

    uint64_t blp = 0;

    uint64_t m2 = 0;

    uint64_t wmo = 0;

    uint64_t adt = 0;

    uint64_t wdt = 0;

    uint64_t dbc = 0;

    uint64_t db2 = 0;

    uint64_t lua = 0;

    uint64_t xml = 0;

    uint64_t other = 0;

    uint64_t errors = 0;

    uint64_t items = 0;

    bool rootReadable = false;
};


// ============================================================
// ESCANEO RECURSIVO
// ============================================================

static void scanDirectory(
        const std::string& path,
        int depth,
        ScanStats& stats,
        std::ostringstream& output
) {
    if (
            depth > 8 ||
            stats.items >= 50000
    ) {
        return;
    }

    std::vector<std::string> items =
            vfsList(path);

    if (path.empty()) {
        stats.rootReadable =
                !items.empty();
    }

    for (
            const std::string& item :
            items
    ) {

        if (
                stats.items >= 50000
        ) {
            break;
        }

        ++stats.items;

        bool isDirectory =
                !item.empty() &&
                item.back() == '/';

        std::string cleanName =
                item;

        if (isDirectory) {
            cleanName.pop_back();
        }

        std::string fullPath;

        if (path.empty()) {
            fullPath =
                    cleanName;
        } else {
            fullPath =
                    path +
                    "/" +
                    cleanName;
        }

        if (isDirectory) {

            ++stats.directories;

            output
                    << "[DIR] "
                    << fullPath
                    << "\n";

            scanDirectory(
                    fullPath,
                    depth + 1,
                    stats,
                    output
            );

            continue;
        }

        ++stats.files;

        std::string extension =
                getExtension(
                        cleanName
                );

        if (extension == "MPQ") {
            ++stats.mpq;
        } else if (
                extension == "BLP"
        ) {
            ++stats.blp;
        } else if (
                extension == "M2"
        ) {
            ++stats.m2;
        } else if (
                extension == "WMO"
        ) {
            ++stats.wmo;
        } else if (
                extension == "ADT"
        ) {
            ++stats.adt;
        } else if (
                extension == "WDT"
        ) {
            ++stats.wdt;
        } else if (
                extension == "DBC"
        ) {
            ++stats.dbc;
        } else if (
                extension == "DB2"
        ) {
            ++stats.db2;
        } else if (
                extension == "LUA"
        ) {
            ++stats.lua;
        } else if (
                extension == "XML"
        ) {
            ++stats.xml;
        } else {
            ++stats.other;
        }

        output
                << "[FILE] "
                << fullPath;

        if (!extension.empty()) {

            output
                    << "  [TYPE] ."
                    << extension;
        }

        output
                << "\n";
    }
}


// ============================================================
// SCAN CLIENTE
// ============================================================

static std::string engineScanClient() {

    std::ostringstream output;

    output
            << "=== WOW MOBILE CLIENT SCANNER ===\n\n";

    if (g_wowFolderUri.empty()) {

        output
                << "SCAN_ERROR\n"
                << "[ERROR] No hay una carpeta raíz configurada.\n";

        return output.str();
    }

    output
            << "[INFO] Escaneando la carpeta seleccionada "
            << "como raíz del cliente.\n";

    output
            << "[INFO] No se presupone ninguna estructura "
            << "de carpetas.\n\n";

    ScanStats stats;

    // --------------------------------------------------------
    // RAÍZ
    // --------------------------------------------------------

    std::vector<std::string> root =
            vfsList("");

    if (root.empty()) {

        output
                << "SCAN_ERROR\n"
                << "[ERROR] No se pudo leer la carpeta raíz "
                << "mediante SAF.\n";

        return output.str();
    }

    stats.rootReadable = true;

    output
            << "[OK] Carpeta raíz accesible.\n";

    output
            << "[INFO] Elementos inmediatos: "
            << root.size()
            << "\n\n";


    // --------------------------------------------------------
    // ESCANEO
    // --------------------------------------------------------

    output
            << "--- ESTRUCTURA DETECTADA ---\n";

    scanDirectory(
            "",
            0,
            stats,
            output
    );

    output
            << "\n";


    // --------------------------------------------------------
    // RESUMEN
    // --------------------------------------------------------

    output
            << "--- RESUMEN ---\n";

    output
            << "[INFO] Directorios: "
            << stats.directories
            << "\n";

    output
            << "[INFO] Archivos: "
            << stats.files
            << "\n";

    output
            << "[INFO] Elementos procesados: "
            << stats.items
            << "\n";

    output
            << "\n";


    // --------------------------------------------------------
    // FORMATOS
    // --------------------------------------------------------

    output
            << "--- FORMATOS WOW DETECTADOS ---\n";

    output
            << "[TYPE] MPQ : "
            << stats.mpq
            << "\n";

    output
            << "[TYPE] BLP : "
            << stats.blp
            << "\n";

    output
            << "[TYPE] M2  : "
            << stats.m2
            << "\n";

    output
            << "[TYPE] WMO : "
            << stats.wmo
            << "\n";

    output
            << "[TYPE] ADT : "
            << stats.adt
            << "\n";

    output
            << "[TYPE] WDT : "
            << stats.wdt
            << "\n";

    output
            << "[TYPE] DBC : "
            << stats.dbc
            << "\n";

    output
            << "[TYPE] DB2 : "
            << stats.db2
            << "\n";

    output
            << "[TYPE] LUA : "
            << stats.lua
            << "\n";

    output
            << "[TYPE] XML : "
            << stats.xml
            << "\n";

    output
            << "[TYPE] Otros: "
            << stats.other
            << "\n";

    output
            << "\n";


    // --------------------------------------------------------
    // CONCLUSIÓN
    // --------------------------------------------------------

    if (
            stats.rootReadable &&
            stats.items > 0
    ) {

        output
                << "SCAN_OK\n";

        output
                << "[OK] El cliente fue descubierto "
                << "mediante el VFS Android.\n";

        output
                << "[OK] La aplicación no depende de "
                << "nombres fijos de carpetas.\n";

    } else {

        output
                << "SCAN_ERROR\n";

        output
                << "[ERROR] No se pudo construir "
                << "la estructura del cliente.\n";
    }

    if (
            stats.items >= 50000
    ) {

        output
                << "[INFO] Se alcanzó el límite de "
                << "50.000 elementos.\n";
    }

    output
            << "[INFO] Profundidad máxima: 8 niveles.\n";

    return output.str();
}


// ============================================================
// LOGIN PROVISIONAL
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
            "Solicitud de login: %s",
            username.c_str()
    );

    /*
     * Todavía no se simula autenticación.
     *
     * La implementación real de WoW 3.3.5a
     * deberá implementar:
     *
     * AUTH_LOGON_CHALLENGE
     * AUTH_LOGON_PROOF
     * REALM_LIST
     * CMSG_AUTH_SESSION
     * SMSG_AUTH_RESPONSE
     * CMSG_CHAR_ENUM
     * SMSG_CHAR_ENUM
     * CMSG_PLAYER_LOGIN
     *
     * etc.
     */

    LOGI(
            "Protocolo WoW 3.3.5a pendiente."
    );

    (void) password;
}


// ============================================================
// INPUT
// ============================================================

static void engineTouchDown(
        float x,
        float y
) {
    LOGI(
            "Touch DOWN %.1f %.1f",
            x,
            y
    );
}

static void engineTouchMove(
        float x,
        float y
) {
    LOGI(
            "Touch MOVE %.1f %.1f",
            x,
            y
    );
}

static void engineTouchUp(
        float x,
        float y
) {
    LOGI(
            "Touch UP %.1f %.1f",
            x,
            y
    );
}


// ============================================================
// JNI ON LOAD
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
// nativeInit
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

    g_running = true;

    LOGI(
            "Motor nativo inicializado."
    );
}


// ============================================================
// nativeSetWowFolder
// ============================================================

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeSetWowFolder(
        JNIEnv* env,
        jobject,
        jstring uri
) {
    if (uri == nullptr) {

        std::lock_guard<std::mutex> lock(
                g_mutex
        );

        g_wowFolderUri.clear();

        return;
    }

    const char* chars =
            env->GetStringUTFChars(
                    uri,
                    nullptr
            );

    if (chars == nullptr) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(
                g_mutex
        );

        g_wowFolderUri =
                chars;
    }

    env->ReleaseStringUTFChars(
            uri,
            chars
    );

    LOGI(
            "Raiz WoW configurada."
    );
}


// ============================================================
// nativeScanClient
// ============================================================

extern "C"
JNIEXPORT jstring JNICALL
Java_com_wowmobile_client_MainActivity_nativeScanClient(
        JNIEnv* env,
        jobject
) {
    std::string result =
            engineScanClient();

    return env->NewStringUTF(
            result.c_str()
    );
}


// ============================================================
// nativeLogin
// ============================================================

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeLogin(
        JNIEnv* env,
        jobject,
        jstring username,
        jstring password
) {
    if (
            username == nullptr ||
            password == nullptr
    ) {
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

    if (
            user == nullptr ||
            pass == nullptr
    ) {

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

        return;
    }

    engineLogin(
            user,
            pass
    );

    env->ReleaseStringUTFChars(
            username,
            user
    );

    env->ReleaseStringUTFChars(
            password,
            pass
    );
}


// ============================================================
// nativeTouchDown
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
// nativeTouchMove
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
// nativeTouchUp
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
