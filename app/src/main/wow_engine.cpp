#include <jni.h>
#include <android/log.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#define LOG_TAG "WoWMobileEngine"

#define LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define LOGW(...) \
    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

#define LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


// ============================================================
// GLOBAL JNI STATE
// ============================================================

static JavaVM* g_vm = nullptr;
static jobject g_mainActivity = nullptr;

static std::string g_wowUri;
static std::mutex g_mutex;


// ============================================================
// SAFE BINARY READ HELPERS
//
// IMPORTANTE:
// Estas funciones están definidas ANTES de readSKIN(),
// readM2(), readBLP(), etc.
// ============================================================

static uint16_t readU16(
        const std::vector<uint8_t>& data,
        size_t offset
) {
    if (offset + 2 > data.size()) {
        return 0;
    }

    return static_cast<uint16_t>(
            static_cast<uint16_t>(data[offset]) |
            (static_cast<uint16_t>(data[offset + 1]) << 8)
    );
}


static uint32_t readU32(
        const std::vector<uint8_t>& data,
        size_t offset
) {
    if (offset + 4 > data.size()) {
        return 0;
    }

    return
            static_cast<uint32_t>(data[offset]) |
            (static_cast<uint32_t>(data[offset + 1]) << 8) |
            (static_cast<uint32_t>(data[offset + 2]) << 16) |
            (static_cast<uint32_t>(data[offset + 3]) << 24);
}


static int32_t readS32(
        const std::vector<uint8_t>& data,
        size_t offset
) {
    return static_cast<int32_t>(readU32(data, offset));
}


static float readF32(
        const std::vector<uint8_t>& data,
        size_t offset
) {
    if (offset + 4 > data.size()) {
        return 0.0f;
    }

    const uint32_t value = readU32(data, offset);

    float result = 0.0f;

    std::memcpy(
            &result,
            &value,
            sizeof(result)
    );

    return result;
}


static bool readBytes(
        const std::vector<uint8_t>& data,
        size_t offset,
        void* destination,
        size_t size
) {
    if (destination == nullptr) {
        return false;
    }

    if (offset + size > data.size()) {
        return false;
    }

    std::memcpy(
            destination,
            data.data() + offset,
            size
    );

    return true;
}


// ============================================================
// READER
// ============================================================

class Reader {
public:

    explicit Reader(const std::vector<uint8_t>& buffer)
        : data(buffer),
          position(0) {
    }

    size_t size() const {
        return data.size();
    }

    size_t tell() const {
        return position;
    }

    bool seek(size_t newPosition) {
        if (newPosition > data.size()) {
            return false;
        }

        position = newPosition;
        return true;
    }

    bool skip(size_t amount) {
        return seek(position + amount);
    }

    bool canRead(size_t amount) const {
        return position + amount <= data.size();
    }

    uint8_t u8() {
        if (!canRead(1)) {
            return 0;
        }

        return data[position++];
    }

    uint16_t u16() {
        if (!canRead(2)) {
            position = data.size();
            return 0;
        }

        const uint16_t value =
                static_cast<uint16_t>(data[position]) |
                (static_cast<uint16_t>(data[position + 1]) << 8);

        position += 2;

        return value;
    }

    uint32_t u32() {
        if (!canRead(4)) {
            position = data.size();
            return 0;
        }

        const uint32_t value =
                static_cast<uint32_t>(data[position]) |
                (static_cast<uint32_t>(data[position + 1]) << 8) |
                (static_cast<uint32_t>(data[position + 2]) << 16) |
                (static_cast<uint32_t>(data[position + 3]) << 24);

        position += 4;

        return value;
    }

    int32_t s32() {
        return static_cast<int32_t>(u32());
    }

    float f32() {
        const uint32_t value = u32();

        float result = 0.0f;

        std::memcpy(
                &result,
                &value,
                sizeof(result)
        );

        return result;
    }

    const uint8_t* ptr(size_t amount) {
        if (!canRead(amount)) {
            return nullptr;
        }

        return data.data() + position;
    }

private:

    const std::vector<uint8_t>& data;
    size_t position;
};


// ============================================================
// JNI HELPERS
// ============================================================

static JNIEnv* getJNIEnv() {

    if (g_vm == nullptr) {
        return nullptr;
    }

    JNIEnv* env = nullptr;

    const jint result =
            g_vm->GetEnv(
                    reinterpret_cast<void**>(&env),
                    JNI_VERSION_1_6
            );

    if (result == JNI_OK) {
        return env;
    }

    if (result == JNI_EDETACHED) {

        if (g_vm->AttachCurrentThread(
                &env,
                nullptr
        ) != JNI_OK) {
            return nullptr;
        }

        return env;
    }

    return nullptr;
}


static std::string jstringToString(
        JNIEnv* env,
        jstring value
) {
    if (env == nullptr || value == nullptr) {
        return {};
    }

    const char* chars =
            env->GetStringUTFChars(
                    value,
                    nullptr
            );

    if (chars == nullptr) {
        return {};
    }

    std::string result(chars);

    env->ReleaseStringUTFChars(
            value,
            chars
    );

    return result;
}


static jstring stringToJString(
        JNIEnv* env,
        const std::string& value
) {
    if (env == nullptr) {
        return nullptr;
    }

    return env->NewStringUTF(
            value.c_str()
    );
}


static jmethodID findMethod(
        JNIEnv* env,
        const char* name,
        const char* signature
) {
    if (env == nullptr || g_mainActivity == nullptr) {
        return nullptr;
    }

    jclass clazz =
            env->GetObjectClass(
                    g_mainActivity
            );

    if (clazz == nullptr) {
        return nullptr;
    }

    jmethodID method =
            env->GetMethodID(
                    clazz,
                    name,
                    signature
            );

    env->DeleteLocalRef(clazz);

    return method;
}


// ============================================================
// JAVA VIRTUAL FILE SYSTEM
// ============================================================

static std::string vfsList(
        const std::string& path
) {
    std::lock_guard<std::mutex> lock(g_mutex);

    JNIEnv* env = getJNIEnv();

    if (env == nullptr || g_mainActivity == nullptr) {
        return {};
    }

    jmethodID method =
            findMethod(
                    env,
                    "vfsList",
                    "(Ljava/lang/String;)Ljava/lang/String;"
            );

    if (method == nullptr) {
        LOGE("No se encontró MainActivity.vfsList()");
        return {};
    }

    jstring jPath =
            env->NewStringUTF(
                    path.c_str()
            );

    if (jPath == nullptr) {
        return {};
    }

    jstring result =
            static_cast<jstring>(
                    env->CallObjectMethod(
                            g_mainActivity,
                            method,
                            jPath
                    )
            );

    env->DeleteLocalRef(jPath);

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return {};
    }

    std::string output =
            jstringToString(
                    env,
                    result
            );

    if (result != nullptr) {
        env->DeleteLocalRef(result);
    }

    return output;
}


static std::vector<uint8_t> vfsReadFile(
        const std::string& path,
        int maxBytes = 0
) {
    std::lock_guard<std::mutex> lock(g_mutex);

    JNIEnv* env = getJNIEnv();

    if (env == nullptr || g_mainActivity == nullptr) {
        return {};
    }

    jmethodID method =
            findMethod(
                    env,
                    "vfsReadFile",
                    "(Ljava/lang/String;I)[B"
            );

    if (method == nullptr) {
        LOGE("No se encontró MainActivity.vfsReadFile()");
        return {};
    }

    jstring jPath =
            env->NewStringUTF(
                    path.c_str()
            );

    if (jPath == nullptr) {
        return {};
    }

    jbyteArray result =
            static_cast<jbyteArray>(
                    env->CallObjectMethod(
                            g_mainActivity,
                            method,
                            jPath,
                            static_cast<jint>(maxBytes)
                    )
            );

    env->DeleteLocalRef(jPath);

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return {};
    }

    if (result == nullptr) {
        return {};
    }

    const jsize length =
            env->GetArrayLength(
                    result
            );

    if (length <= 0) {
        env->DeleteLocalRef(result);
        return {};
    }

    std::vector<uint8_t> output(
            static_cast<size_t>(length)
    );

    env->GetByteArrayRegion(
            result,
            0,
            length,
            reinterpret_cast<jbyte*>(output.data())
    );

    env->DeleteLocalRef(result);

    return output;
}


// ============================================================
// STRING / PATH HELPERS
// ============================================================

static std::string extension(
        const std::string& path
) {
    const size_t slash =
            path.find_last_of(
                    "/\\"
            );

    const size_t dot =
            path.find_last_of(
                    '.'
            );

    if (dot == std::string::npos) {
        return {};
    }

    if (slash != std::string::npos &&
        dot < slash) {
        return {};
    }

    std::string ext =
            path.substr(
                    dot + 1
            );

    std::transform(
            ext.begin(),
            ext.end(),
            ext.begin(),
            [](unsigned char c) {
                return static_cast<char>(
                        std::tolower(c)
                );
            }
    );

    return ext;
}


static std::string basename(
        const std::string& path
) {
    const size_t slash =
            path.find_last_of(
                    "/\\"
            );

    if (slash == std::string::npos) {
        return path;
    }

    return path.substr(
            slash + 1
    );
}


static std::string fourCC(
        uint32_t value
) {
    std::string result(4, '\0');

    result[0] =
            static_cast<char>(
                    value & 0xFF
            );

    result[1] =
            static_cast<char>(
                    (value >> 8) & 0xFF
            );

    result[2] =
            static_cast<char>(
                    (value >> 16) & 0xFF
            );

    result[3] =
            static_cast<char>(
                    (value >> 24) & 0xFF
            );

    return result;
}


static std::string hex32(
        uint32_t value
) {
    std::ostringstream stream;

    stream << std::hex
           << std::uppercase
           << std::setfill('0')
           << std::setw(8)
           << value;

    return stream.str();
}


// ============================================================
// BLP READER
// ============================================================

static std::string readBLP(
        const std::vector<uint8_t>& data
) {
    if (data.size() < 20) {
        return "BLP demasiado pequeño";
    }

    const uint32_t magic =
            readU32(
                    data,
                    0
            );

    if (magic != 0x32504C42u) {
        return "BLP inválido";
    }

    const uint32_t compression =
            readU32(
                    data,
                    4
            );

    const uint32_t alphaDepth =
            readU32(
                    data,
                    8
            );

    const uint32_t alphaEncoding =
            readU32(
                    data,
                    12
            );

    const uint32_t hasMips =
            readU32(
                    data,
                    16
            );

    std::ostringstream result;

    result
            << "BLP2\n"
            << "compression=" << compression << "\n"
            << "alphaDepth=" << alphaDepth << "\n"
            << "alphaEncoding=" << alphaEncoding << "\n"
            << "hasMips=" << hasMips << "\n"
            << "size=" << data.size();

    return result.str();
}


// ============================================================
// M2 READER
//
// Lectura básica de cabecera para WoW 3.3.5a.
// ============================================================

static std::string readM2(
        const std::vector<uint8_t>& data
) {
    if (data.size() < 20) {
        return "M2 demasiado pequeño";
    }

    const uint32_t magic =
            readU32(
                    data,
                    0
            );

    if (magic != 0x3032444Du) {
        return "M2 inválido";
    }

    const uint32_t version =
            readU32(
                    data,
                    4
            );

    const uint32_t nameLength =
            readU32(
                    data,
                    8
            );

    const uint32_t nameOffset =
            readU32(
                    data,
                    12
            );

    std::ostringstream result;

    result
            << "M2\n"
            << "version=" << version << "\n"
            << "nameLength=" << nameLength << "\n"
            << "nameOffset=" << nameOffset << "\n"
            << "size=" << data.size();

    return result.str();
}


// ============================================================
// SKIN READER
//
// IMPORTANTE:
// Aquí estaba el error de compilación del archivo anterior.
// readU32() ahora existe antes de esta función.
// ============================================================

static std::string readSKIN(
        const std::vector<uint8_t>& data
) {
    if (data.size() < 44 ||
        readU32(data, 0) != 0x4E494B53u) {
        return "SKIN inválido";
    }

    const uint32_t indexCount =
            readU32(
                    data,
                    4
            );

    const uint32_t indexOffset =
            readU32(
                    data,
                    8
            );

    const uint32_t triangleCount =
            readU32(
                    data,
                    12
            );

    const uint32_t triangleOffset =
            readU32(
                    data,
                    16
            );

    const uint32_t boneCount =
            readU32(
                    data,
                    20
            );

    const uint32_t boneOffset =
            readU32(
                    data,
                    24
            );

    const uint32_t submeshCount =
            readU32(
                    data,
                    28
            );

    const uint32_t submeshOffset =
            readU32(
                    data,
                    32
            );

    const uint32_t textureUnitCount =
            readU32(
                    data,
                    36
            );

    const uint32_t textureUnitOffset =
            readU32(
                    data,
                    40
            );

    std::ostringstream result;

    result
            << "SKIN\n"
            << "indexCount=" << indexCount << "\n"
            << "indexOffset=" << indexOffset << "\n"
            << "triangleCount=" << triangleCount << "\n"
            << "triangleOffset=" << triangleOffset << "\n"
            << "boneCount=" << boneCount << "\n"
            << "boneOffset=" << boneOffset << "\n"
            << "submeshCount=" << submeshCount << "\n"
            << "submeshOffset=" << submeshOffset << "\n"
            << "textureUnitCount=" << textureUnitCount << "\n"
            << "textureUnitOffset=" << textureUnitOffset << "\n"
            << "size=" << data.size();

    return result.str();
}


// ============================================================
// ANIM READER
// ============================================================

static std::string readANIM(
        const std::vector<uint8_t>& data
) {
    if (data.size() < 8) {
        return "ANIM demasiado pequeño";
    }

    std::ostringstream result;

    result
            << "ANIM\n"
            << "magic="
            << fourCC(readU32(data, 0))
            << "\n"
            << "version="
            << readU32(data, 4)
            << "\n"
            << "size="
            << data.size();

    return result.str();
}


// ============================================================
// SBT READER
// ============================================================

static std::string readSBT(
        const std::vector<uint8_t>& data
) {
    if (data.size() < 8) {
        return "SBT demasiado pequeño";
    }

    std::ostringstream result;

    result
            << "SBT\n"
            << "magic="
            << fourCC(readU32(data, 0))
            << "\n"
            << "value="
            << readU32(data, 4)
            << "\n"
            << "size="
            << data.size();

    return result.str();
}


// ============================================================
// RESOURCE INSPECTOR
// ============================================================

static std::string readResource(
        const std::string& path,
        const std::vector<uint8_t>& data
) {
    if (data.empty()) {
        return "RESOURCE VACÍO\npath=" + path;
    }

    const std::string ext =
            extension(path);

    if (ext == "blp") {
        return readBLP(data);
    }

    if (ext == "m2") {
        return readM2(data);
    }

    if (ext == "skin") {
        return readSKIN(data);
    }

    if (ext == "anim") {
        return readANIM(data);
    }

    if (ext == "sbt") {
        return readSBT(data);
    }

    std::ostringstream result;

    result
            << "RESOURCE\n"
            << "path=" << path << "\n"
            << "extension=" << ext << "\n"
            << "size=" << data.size() << "\n"
            << "first32=";

    const size_t count =
            std::min<size_t>(
                    32,
                    data.size()
            );

    for (size_t i = 0; i < count; ++i) {

        if (i != 0) {
            result << ' ';
        }

        result
                << std::hex
                << std::uppercase
                << std::setfill('0')
                << std::setw(2)
                << static_cast<unsigned int>(
                        data[i]
                );
    }

    return result.str();
}


// ============================================================
// ROOT INSPECTION
// ============================================================

static std::string inspectRoot() {

    std::ostringstream result;

    result << "WoW CLIENT INSPECTION\n";
    result << "URI=" << g_wowUri << "\n\n";

    const std::string root =
            vfsList("");

    result << root;

    return result.str();
}


// ============================================================
// LOGIN
//
// Todavía NO simula un login exitoso.
// La conexión real 3.3.5a necesita:
//   - Auth SRP6
//   - Realm list
//   - World socket
//   - Handshake
//   - World authentication
//
// ============================================================

static std::string engineLogin(
        const std::string& username,
        const std::string& password
) {
    (void)username;
    (void)password;

    LOGW(
            "engineLogin: protocolo WoW 3.3.5a todavía no implementado"
    );

    return
            "LOGIN_NOT_IMPLEMENTED\n"
            "El cliente nativo todavía no ejecuta "
            "el protocolo SRP6/world de WoW 3.3.5a.";
}


// ============================================================
// JNI_OnLoad
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


// ============================================================
// nativeInit
// ============================================================

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeInit(
        JNIEnv* env,
        jobject thiz
) {
    if (env == nullptr || thiz == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_mutex);

    if (g_mainActivity != nullptr) {
        env->DeleteGlobalRef(
                g_mainActivity
        );

        g_mainActivity = nullptr;
    }

    g_mainActivity =
            env->NewGlobalRef(
                    thiz
            );

    LOGI(
            "WoW native engine inicializado"
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
    if (env == nullptr) {
        return;
    }

    const std::string value =
            jstringToString(
                    env,
                    uri
            );

    {
        std::lock_guard<std::mutex> lock(g_mutex);

        g_wowUri = value;
    }

    LOGI(
            "WoW folder/URI: %s",
            g_wowUri.c_str()
    );
}


// ============================================================
// nativeInspectRoot
// ============================================================

extern "C"
JNIEXPORT jstring JNICALL
Java_com_wowmobile_client_MainActivity_nativeInspectRoot(
        JNIEnv* env,
        jobject
) {
    if (env == nullptr) {
        return nullptr;
    }

    const std::string result =
            inspectRoot();

    return stringToJString(
            env,
            result
    );
}


// ============================================================
// nativeListDirectory
// ============================================================

extern "C"
JNIEXPORT jstring JNICALL
Java_com_wowmobile_client_MainActivity_nativeListDirectory(
        JNIEnv* env,
        jobject,
        jstring path
) {
    if (env == nullptr) {
        return nullptr;
    }

    const std::string directory =
            jstringToString(
                    env,
                    path
            );

    const std::string result =
            vfsList(
                    directory
            );

    return stringToJString(
            env,
            result
    );
}


// ============================================================
// nativeReadResource
// ============================================================

extern "C"
JNIEXPORT jstring JNICALL
Java_com_wowmobile_client_MainActivity_nativeReadResource(
        JNIEnv* env,
        jobject,
        jstring path
) {
    if (env == nullptr) {
        return nullptr;
    }

    const std::string resourcePath =
            jstringToString(
                    env,
                    path
            );

    std::vector<uint8_t> data =
            vfsReadFile(
                    resourcePath,
                    0
            );

    const std::string result =
            readResource(
                    resourcePath,
                    data
            );

    return stringToJString(
            env,
            result
    );
}


// ============================================================
// nativeLogin
// ============================================================

extern "C"
JNIEXPORT jstring JNICALL
Java_com_wowmobile_client_MainActivity_nativeLogin(
        JNIEnv* env,
        jobject,
        jstring username,
        jstring password
) {
    if (env == nullptr) {
        return nullptr;
    }

    const std::string user =
            jstringToString(
                    env,
                    username
            );

    const std::string pass =
            jstringToString(
                    env,
                    password
            );

    const std::string result =
            engineLogin(
                    user,
                    pass
            );

    return stringToJString(
            env,
            result
    );
}
