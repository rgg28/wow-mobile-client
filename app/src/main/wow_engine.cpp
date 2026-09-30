#include <jni.h>
#include <android/log.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#define LOG_TAG "WoWMobile"

#define LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static JavaVM* g_vm = nullptr;
static jobject g_mainActivity = nullptr;
static std::string g_wowUri;
static std::mutex g_mutex;


// ============================================================
// BYTE READER
// ============================================================

class Reader {

public:

    Reader(
            const std::vector<uint8_t>& data
    )
        : m_data(data),
          m_pos(0) {
    }

    size_t size() const {
        return m_data.size();
    }

    size_t position() const {
        return m_pos;
    }

    bool seek(size_t position) {

        if (position > m_data.size()) {
            return false;
        }

        m_pos = position;
        return true;
    }

    bool skip(size_t count) {

        return seek(
                m_pos + count
        );
    }

    bool canRead(size_t count) const {

        return
            m_pos <= m_data.size() &&
            count <=
                m_data.size() - m_pos;
    }

    uint8_t u8() {

        if (!canRead(1)) {
            return 0;
        }

        return m_data[m_pos++];
    }

    uint16_t u16() {

        if (!canRead(2)) {
            return 0;
        }

        uint16_t v =
            static_cast<uint16_t>(
                m_data[m_pos]
            ) |
            static_cast<uint16_t>(
                m_data[m_pos + 1]
            ) << 8;

        m_pos += 2;

        return v;
    }

    uint32_t u32() {

        if (!canRead(4)) {
            return 0;
        }

        uint32_t v =
            static_cast<uint32_t>(
                m_data[m_pos]
            ) |
            static_cast<uint32_t>(
                m_data[m_pos + 1]
            ) << 8 |
            static_cast<uint32_t>(
                m_data[m_pos + 2]
            ) << 16 |
            static_cast<uint32_t>(
                m_data[m_pos + 3]
            ) << 24;

        m_pos += 4;

        return v;
    }

    float f32() {

        uint32_t raw = u32();

        float value;

        std::memcpy(
                &value,
                &raw,
                sizeof(value)
        );

        return value;
    }

    const uint8_t* ptr(
            size_t offset
    ) const {

        if (offset >= m_data.size()) {
            return nullptr;
        }

        return
            m_data.data() +
            offset;
    }

private:

    const std::vector<uint8_t>& m_data;
    size_t m_pos;
};


// ============================================================
// JNI
// ============================================================

static JNIEnv* getJNIEnv() {

    if (!g_vm) {
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

    if (!env || !value) {
        return "";
    }

    const char* chars =
        env->GetStringUTFChars(
                value,
                nullptr
        );

    if (!chars) {
        return "";
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

    return env->NewStringUTF(
            value.c_str()
    );
}


static jmethodID findMethod(
        JNIEnv* env,
        const char* name,
        const char* signature
) {

    if (!env || !g_mainActivity) {
        return nullptr;
    }

    jclass clazz =
        env->GetObjectClass(
                g_mainActivity
        );

    if (!clazz) {
        return nullptr;
    }

    return env->GetMethodID(
            clazz,
            name,
            signature
    );
}


// ============================================================
// VFS
// ============================================================

static std::string vfsList(
        const std::string& path
) {

    std::lock_guard<std::mutex> lock(
            g_mutex
    );

    JNIEnv* env =
        getJNIEnv();

    if (!env || !g_mainActivity) {
        return "";
    }

    jmethodID method =
        findMethod(
                env,
                "vfsList",
                "(Ljava/lang/String;)Ljava/lang/String;"
        );

    if (!method) {
        return "";
    }

    jstring jpath =
        env->NewStringUTF(
                path.c_str()
        );

    jstring result =
        static_cast<jstring>(
                env->CallObjectMethod(
                        g_mainActivity,
                        method,
                        jpath
                )
        );

    env->DeleteLocalRef(
            jpath
    );

    if (!result) {
        return "";
    }

    std::string output =
        jstringToString(
                env,
                result
        );

    env->DeleteLocalRef(
            result
    );

    return output;
}


static std::vector<uint8_t> vfsReadFile(
        const std::string& path,
        int maxBytes
) {

    std::lock_guard<std::mutex> lock(
            g_mutex
    );

    JNIEnv* env =
        getJNIEnv();

    if (!env || !g_mainActivity) {
        return {};
    }

    jmethodID method =
        findMethod(
                env,
                "vfsReadFile",
                "(Ljava/lang/String;I)[B"
        );

    if (!method) {
        return {};
    }

    jstring jpath =
        env->NewStringUTF(
                path.c_str()
        );

    jbyteArray array =
        static_cast<jbyteArray>(
                env->CallObjectMethod(
                        g_mainActivity,
                        method,
                        jpath,
                        maxBytes
                )
        );

    env->DeleteLocalRef(
            jpath
    );

    if (!array) {
        return {};
    }

    jsize length =
        env->GetArrayLength(
                array
        );

    std::vector<uint8_t> data(
            static_cast<size_t>(length)
    );

    if (length > 0) {

        env->GetByteArrayRegion(
                array,
                0,
                length,
                reinterpret_cast<jbyte*>(
                        data.data()
                )
        );
    }

    env->DeleteLocalRef(
            array
    );

    return data;
}


// ============================================================
// EXTENSION
// ============================================================

static std::string extension(
        const std::string& path
) {

    size_t dot =
        path.find_last_of('.');

    if (dot == std::string::npos) {
        return "";
    }

    std::string result =
        path.substr(dot + 1);

    std::transform(
            result.begin(),
            result.end(),
            result.begin(),
            [](unsigned char c) {
                return static_cast<char>(
                        std::tolower(c)
                );
            }
    );

    return result;
}


// ============================================================
// BLP
// ============================================================

static std::string readBLP(
        const std::string& path,
        const std::vector<uint8_t>& data
) {

    Reader r(data);

    if (data.size() < 148) {

        return
            "BLP ERROR\n\n"
            "Archivo demasiado pequeño.";
    }

    uint32_t magic = r.u32();

    uint32_t version = r.u32();

    if (magic != 0x32504C42) {

        return
            "BLP ERROR\n\n"
            "Magic no reconocido.\n"
            "Esperado: BLP2";
    }

    uint32_t compression = r.u32();
    uint32_t alphaBits = r.u32();
    uint32_t alphaType = r.u32();
    uint32_t hasMips = r.u32();

    uint32_t width = r.u32();
    uint32_t height = r.u32();

    uint32_t offsets[16];
    uint32_t sizes[16];

    for (int i = 0; i < 16; ++i) {
        offsets[i] = r.u32();
    }

    for (int i = 0; i < 16; ++i) {
        sizes[i] = r.u32();
    }

    std::ostringstream out;

    out << "BLP2\n";
    out << "==============================\n\n";

    out << "Archivo: "
        << path
        << "\n\n";

    out << "Versión: "
        << version
        << "\n";

    out << "Dimensiones: "
        << width
        << " x "
        << height
        << "\n";

    out << "Compresión: "
        << compression
        << "\n";

    out << "Alpha bits: "
        << alphaBits
        << "\n";

    out << "Alpha type: "
        << alphaType
        << "\n";

    out << "Mipmaps: "
        << (hasMips ? "sí" : "no")
        << "\n\n";

    out << "MIPMAPS\n";
    out << "------------------------------\n";

    for (int i = 0; i < 16; ++i) {

        if (offsets[i] == 0 ||
            sizes[i] == 0) {
            continue;
        }

        bool valid =
            static_cast<uint64_t>(
                offsets[i]
            ) +
            static_cast<uint64_t>(
                sizes[i]
            ) <= data.size();

        out << "Mip "
            << i
            << ": offset="
            << offsets[i]
            << " size="
            << sizes[i]
            << " "
            << (valid ? "OK" : "INVALID")
            << "\n";
    }

    return out.str();
}


// ============================================================
// M2
// ============================================================

static std::string readM2(
        const std::string& path,
        const std::vector<uint8_t>& data
) {

    Reader r(data);

    if (data.size() < 64) {

        return
            "M2 ERROR\n\n"
            "Archivo demasiado pequeño.";
    }

    uint32_t magic = r.u32();

    if (magic != 0x3032444D) {

        return
            "M2 ERROR\n\n"
            "Magic no reconocido.\n"
            "Esperado: MD20";
    }

    uint32_t version = r.u32();

    uint32_t nameLength = r.u32();
    uint32_t nameOffset = r.u32();

    uint32_t globalFlags = r.u32();

    uint32_t nGlobalSequences = r.u32();
    uint32_t ofsGlobalSequences = r.u32();

    uint32_t nAnimations = r.u32();
    uint32_t ofsAnimations = r.u32();

    uint32_t nAnimationLookup = r.u32();
    uint32_t ofsAnimationLookup = r.u32();

    uint32_t nBones = r.u32();
    uint32_t ofsBones = r.u32();

    uint32_t nKeyBoneLookup = r.u32();
    uint32_t ofsKeyBoneLookup = r.u32();

    uint32_t nVertices = r.u32();
    uint32_t ofsVertices = r.u32();

    uint32_t nViews = r.u32();
    uint32_t ofsViews = r.u32();

    std::ostringstream out;

    out << "M2 / MD20\n";
    out << "==============================\n\n";

    out << "Archivo: "
        << path
        << "\n\n";

    out << "Versión: "
        << version
        << "\n";

    out << "Tamaño: "
        << data.size()
        << " bytes\n\n";

    out << "Nombre length: "
        << nameLength
        << "\n";

    out << "Nombre offset: "
        << nameOffset
        << "\n";

    out << "Global flags: 0x"
        << std::hex
        << globalFlags
        << std::dec
        << "\n\n";

    out << "GLOBAL SEQUENCES\n";
    out << "Count: "
        << nGlobalSequences
        << "\nOffset: "
        << ofsGlobalSequences
        << "\n\n";

    out << "ANIMACIONES\n";
    out << "Count: "
        << nAnimations
        << "\nOffset: "
        << ofsAnimations
        << "\n\n";

    out << "ANIMATION LOOKUP\n";
    out << "Count: "
        << nAnimationLookup
        << "\nOffset: "
        << ofsAnimationLookup
        << "\n\n";

    out << "HUESOS\n";
    out << "Count: "
        << nBones
        << "\nOffset: "
        << ofsBones
        << "\n\n";

    out << "BONE LOOKUP\n";
    out << "Count: "
        << nKeyBoneLookup
        << "\nOffset: "
        << ofsKeyBoneLookup
        << "\n\n";

    out << "VERTICES\n";
    out << "Count: "
        << nVertices
        << "\nOffset: "
        << ofsVertices
        << "\n\n";

    out << "VIEWS\n";
    out << "Count: "
        << nViews
        << "\nOffset: "
        << ofsViews
        << "\n\n";

    auto validRange =
            [&](uint32_t offset,
                uint64_t bytes) {

        return
            static_cast<uint64_t>(
                offset
            ) +
            bytes <= data.size();
    };

    out << "VALIDACIÓN\n";
    out << "------------------------------\n";

    out << "Global sequences: "
        << (
            validRange(
                ofsGlobalSequences,
                static_cast<uint64_t>(
                    nGlobalSequences
                ) * 4
            )
            ? "OK"
            : "INVALID"
        )
        << "\n";

    out << "Animation table: "
        << (
            validRange(
                ofsAnimations,
                static_cast<uint64_t>(
                    nAnimations
                ) * 64
            )
            ? "OK"
            : "FUERA DE RANGO"
        )
        << "\n";

    out << "Bones: "
        << (
            validRange(
                ofsBones,
                static_cast<uint64_t>(
                    nBones
                ) * 128
            )
            ? "OK"
            : "FUERA DE RANGO"
        )
        << "\n";

    out << "Vertices: "
        << (
            validRange(
                ofsVertices,
                static_cast<uint64_t>(
                    nVertices
                ) * 48
            )
            ? "OK"
            : "FUERA DE RANGO"
        )
        << "\n";

    return out.str();
}


// ============================================================
// SKIN
// ============================================================

static std::string readSKIN(
        const std::string& path,
        const std::vector<uint8_t>& data
) {

    Reader r(data);

    if (data.size() < 40) {

        return
            "SKIN ERROR\n\n"
            "Archivo demasiado pequeño.";
    }

    uint32_t magic = r.u32();

    if (magic != 0x4E494B53) {

        return
            "SKIN ERROR\n\n"
            "Magic no reconocido.\n"
            "Esperado: SKIN";
    }

    uint32_t nIndices = r.u32();
    uint32_t ofsIndices = r.u32();

    uint32_t nTriangles = r.u32();
    uint32_t ofsTriangles = r.u32();

    uint32_t nProperties = r.u32();
    uint32_t ofsProperties = r.u32();

    uint32_t nSubmeshes = r.u32();
    uint32_t ofsSubmeshes = r.u32();

    uint32_t nTextureUnits = r.u32();
    uint32_t ofsTextureUnits = r.u32();

    std::ostringstream out;

    out << "SKIN\n";
    out << "==============================\n\n";

    out << "Archivo: "
        << path
        << "\n";

    out << "Tamaño: "
        << data.size()
        << " bytes\n\n";

    out << "ÍNDICES\n";
    out << "Count: "
        << nIndices
        << "\nOffset: "
        << ofsIndices
        << "\n\n";

    out << "TRIÁNGULOS\n";
    out << "Count: "
        << nTriangles
        << "\nOffset: "
        << ofsTriangles
        << "\n\n";

    out << "PROPIEDADES\n";
    out << "Count: "
        << nProperties
        << "\nOffset: "
        << ofsProperties
        << "\n\n";

    out << "SUBMESHES\n";
    out << "Count: "
        << nSubmeshes
        << "\nOffset: "
        << ofsSubmeshes
        << "\n\n";

    out << "TEXTURE UNITS\n";
    out << "Count: "
        << nTextureUnits
        << "\nOffset: "
        << ofsTextureUnits
        << "\n";

    return out.str();
}


// ============================================================
// ANIM
// ============================================================

static std::string readANIM(
        const std::string& path,
        const std::vector<uint8_t>& data
) {

    std::ostringstream out;

    out << "ANIM\n";
    out << "==============================\n\n";

    out << "Archivo: "
        << path
        << "\n";

    out << "Tamaño: "
        << data.size()
        << " bytes\n\n";

    if (data.size() >= 4) {

        uint32_t magic =
            static_cast<uint32_t>(data[0]) |
            static_cast<uint32_t>(data[1]) << 8 |
            static_cast<uint32_t>(data[2]) << 16 |
            static_cast<uint32_t>(data[3]) << 24;

        out << "Magic: 0x"
            << std::hex
            << std::setw(8)
            << std::setfill('0')
            << magic
            << std::dec
            << "\n";
    }

    out << "\nEl archivo fue abierto correctamente.\n";

    out << "El lector conserva el bloque binario\n";
    out << "para la futura integración con el sistema\n";
    out << "de animación M2.\n";

    return out.str();
}


// ============================================================
// SBT
// ============================================================

static std::string readSBT(
        const std::string& path,
        const std::vector<uint8_t>& data
) {

    std::ostringstream out;

    out << "SBT\n";
    out << "==============================\n\n";

    out << "Archivo: "
        << path
        << "\n";

    out << "Tamaño: "
        << data.size()
        << " bytes\n\n";

    if (data.size() >= 4) {

        out << "Magic: ";

        for (int i = 0; i < 4; ++i) {

            uint8_t c =
                    data[i];

            if (c >= 32 &&
                c <= 126) {

                out
                    << static_cast<char>(
                        c
                    );

            } else {

                out << '.';
            }
        }

        out << "\n";
    }

    out << "\nEl recurso fue abierto correctamente.\n";

    return out.str();
}


// ============================================================
// DISPATCHER
// ============================================================

static std::string readResource(
        const std::string& path
) {

    std::string ext =
        extension(path);

    /*
     * Los lectores de metadatos necesitan solamente una
     * cantidad limitada de datos.
     *
     * No se intenta cargar recursos gigantescos.
     */

    constexpr int MAX_READ =
        16 * 1024 * 1024;

    std::vector<uint8_t> data =
        vfsReadFile(
                path,
                MAX_READ
        );

    if (data.empty()) {

        return
            "ERROR\n\n"
            "No se pudo leer:\n" +
            path;
    }

    if (ext == "blp") {

        return readBLP(
                path,
                data
        );
    }

    if (ext == "m2") {

        return readM2(
                path,
                data
        );
    }

    if (ext == "skin") {

        return readSKIN(
                path,
                data
        );
    }

    if (ext == "anim") {

        return readANIM(
                path,
                data
        );
    }

    if (ext == "sbt") {

        return readSBT(
                path,
                data
        );
    }

    return
        "RECURSO DESCONOCIDO\n\n" +
        path;
}


// ============================================================
// INSPECCIÓN DE RAÍZ
// ============================================================

static std::string inspectRoot() {

    if (g_wowUri.empty()) {

        return
            "NO HAY CARPETA SELECCIONADA.";
    }

    std::string list =
        vfsList("");

    if (list.empty()) {

        return
            "NO SE PUDO LEER LA CARPETA.";
    }

    int directories = 0;
    int files = 0;

    std::vector<std::string> dirs;

    std::stringstream stream(list);

    std::string line;

    while (std::getline(
            stream,
            line
    )) {

        if (line.size() >= 2 &&
            line[0] == 'D' &&
            line[1] == '|') {

            directories++;

            dirs.push_back(
                    line.substr(2)
            );
        }

        else if (
                line.size() >= 2 &&
                line[0] == 'F' &&
                line[1] == '|'
        ) {

            files++;
        }
    }

    std::sort(
            dirs.begin(),
            dirs.end()
    );

    std::ostringstream out;

    out << "CLIENTE DETECTADO\n";
    out << "==============================\n\n";

    out << "WoW 3.3.5a / Build 12340\n\n";

    out << "Carpeta raíz: OK\n";

    out << "Subcarpetas directas: "
        << directories
        << "\n";

    out << "Archivos directos: "
        << files
        << "\n\n";

    out << "RECURSOS\n";
    out << "------------------------------\n";

    out << ".BLP  = texturas\n";
    out << ".M2   = modelos\n";
    out << ".SKIN = geometría/render\n";
    out << ".ANIM = animaciones\n";
    out << ".SBT  = recursos auxiliares\n\n";

    out << "CARPETAS\n";
    out << "------------------------------\n";

    size_t max =
            std::min<size_t>(
                    dirs.size(),
                    100
            );

    for (size_t i = 0;
         i < max;
         ++i) {

        out << "[DIR] "
            << dirs[i]
            << "\n";
    }

    if (dirs.size() > max) {

        out << "\n... "
            << dirs.size() - max
            << " carpetas adicionales.\n";
    }

    out << "\nLos archivos se leen bajo demanda.";

    return out.str();
}


// ============================================================
// LOGIN
// ============================================================

static std::string engineLogin(
        const std::string& username,
        const std::string& password
) {

    (void) username;
    (void) password;

    return
        "NO IMPLEMENTADO\n\n"
        "La interfaz de autenticación está preparada,\n"
        "pero el protocolo real de WoW 3.3.5a todavía\n"
        "no está conectado.\n\n"
        "No se simula un inicio de sesión exitoso.";
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

    if (g_mainActivity) {

        env->DeleteGlobalRef(
                g_mainActivity
        );

        g_mainActivity = nullptr;
    }

    g_mainActivity =
            env->NewGlobalRef(
                    activity
            );

    LOGI(
            "WoW Mobile Engine iniciado"
    );
}


extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeSetWowFolder(
        JNIEnv* env,
        jobject,
        jstring uri
) {

    g_wowUri =
            jstringToString(
                    env,
                    uri
            );

    LOGI(
            "WoW root configurada"
    );
}


extern "C"
JNIEXPORT jstring JNICALL
Java_com_wowmobile_client_MainActivity_nativeInspectRoot(
        JNIEnv* env,
        jobject
) {

    std::string result =
            inspectRoot();

    return stringToJString(
            env,
            result
    );
}


extern "C"
JNIEXPORT jstring JNICALL
Java_com_wowmobile_client_MainActivity_nativeListDirectory(
        JNIEnv* env,
        jobject,
        jstring relativePath
) {

    std::string path =
            jstringToString(
                    env,
                    relativePath
            );

    std::string result =
            vfsList(path);

    return stringToJString(
            env,
            result
    );
}


extern "C"
JNIEXPORT jstring JNICALL
Java_com_wowmobile_client_MainActivity_nativeReadResource(
        JNIEnv* env,
        jobject,
        jstring relativePath
) {

    std::string path =
            jstringToString(
                    env,
                    relativePath
            );

    std::string result =
            readResource(path);

    return stringToJString(
            env,
            result
    );
}


extern "C"
JNIEXPORT jstring JNICALL
Java_com_wowmobile_client_MainActivity_nativeLogin(
        JNIEnv* env,
        jobject,
        jstring username,
        jstring password
) {

    std::string user =
            jstringToString(
                    env,
                    username
            );

    std::string pass =
            jstringToString(
                    env,
                    password
            );

    std::string result =
            engineLogin(
                    user,
                    pass
            );

    return stringToJString(
            env,
            result
    );
}
