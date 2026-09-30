#include <jni.h>
#include <android/log.h>
#include <android/native_window_jni.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include <memory>

// --- COMPONENTES NATIVOS DE WOWEE ---
#include "core/application.hpp"
#include "network/world_socket.hpp"
#include "auth/auth_handler.hpp"

#define LOG_TAG "WoWMobileEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ============================================================
// STATE & MOTOR INSTANCES
// ============================================================
static JavaVM* g_vm = nullptr;
static jobject g_mainActivity = nullptr;
static std::string g_wowUri;
static std::mutex g_mutex;

std::unique_ptr<WoWee::Application> g_WoWApplication = nullptr;
std::unique_ptr<WoWee::WorldSocket>   g_WoWWorldSocket = nullptr;

// ============================================================
// TUS SAFE BINARY READ HELPERS (Conservados de tu base)
// ============================================================
static uint16_t readU16(const std::vector<uint8_t>& data, size_t offset) {
    if (offset + 2 > data.size()) return 0;
    return static_cast<uint16_t>(static_cast<uint16_t>(data[offset]) | (static_cast<uint16_t>(data[offset + 1]) << 8));
}

static uint32_t readU32(const std::vector<uint8_t>& data, size_t offset) {
    if (offset + 4 > data.size()) return 0;
    return static_cast<uint32_t>(data[offset]) | (static_cast<uint32_t>(data[offset + 1]) << 8) | (static_cast<uint32_t>(data[offset + 2]) << 16) | (static_cast<uint32_t>(data[offset + 3]) << 24);
}

static int32_t readS32(const std::vector<uint8_t>& data, size_t offset) {
    return static_cast<int32_t>(readU32(data, offset));
}

static float readF32(const std::vector<uint8_t>& data, size_t offset) {
    if (offset + 4 > data.size()) return 0.0f;
    const uint32_t value = readU32(data, offset);
    float result = 0.0f;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

static bool readBytes(const std::vector<uint8_t>& data, size_t offset, void* destination, size_t size) {
    if (destination == nullptr || offset + size > data.size()) return false;
    std::memcpy(destination, data.data() + offset, size);
    return true;
}

// ============================================================
// TU CLASE READER ORIGINAL
// ============================================================
class Reader {
public:
    explicit Reader(const std::vector<uint8_t>& buffer) : data(buffer), position(0) {}
    size_t size() const { return data.size(); }
    size_t tell() const { return position; }
    bool seek(size_t newPosition) { if (newPosition > data.size()) return false; position = newPosition; return true; }
    bool skip(size_t amount) { return seek(position + amount); }
    bool canRead(size_t amount) const { return position + amount <= data.size(); }
    uint8_t u8() { if (!canRead(1)) return 0; return data[position++]; }
    uint16_t u16() {
        if (!canRead(2)) { position = data.size(); return 0; }
        const uint16_t value = static_cast<uint16_t>(data[position]) | (static_cast<uint16_t>(data[position + 1]) << 8);
        position += 2; return value;
    }
    uint32_t u32() {
        if (!canRead(4)) { position = data.size(); return 0; }
        const uint32_t value = static_cast<uint32_t>(data[position]) | (static_cast<uint32_t>(data[position + 1]) << 8) | (static_cast<uint32_t>(data[position + 2]) << 16) | (static_cast<uint32_t>(data[position + 3]) << 24);
        position += 4; return value;
    }
    int32_t s32() { return static_cast<int32_t>(u32()); }
    float f32() { const uint32_t value = u32(); float result = 0.0f; std::memcpy(&result, &value, sizeof(result)); return result; }
    const uint8_t* ptr(size_t amount) { if (!canRead(amount)) return nullptr; return data.data() + position; }
private:
    const std::vector<uint8_t>& data;
    size_t position;
};

// ============================================================
// JNI ENVIROMENT HELPERS
// ============================================================
static JNIEnv* getJNIEnv() {
    if (g_vm == nullptr) return nullptr;
    JNIEnv* env = nullptr;
    const jint result = g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (result == JNI_OK) return env;
    if (result == JNI_EDETACHED) {
        if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return nullptr;
        return env;
    }
    return nullptr;
}

static std::string jstringToString(JNIEnv* env, jstring value) {
    if (env == nullptr || value == nullptr) return {};
    const char* chars = env->GetStringUTFChars(value, nullptr);
    if (chars == nullptr) return {};
    std::string result(chars);
    env->ReleaseStringUTFChars(value, chars);
    return result;
}

static jmethodID findMethod(JNIEnv* env, const char* name, const char* signature) {
    if (env == nullptr || g_mainActivity == nullptr) return nullptr;
    jclass clazz = env->GetObjectClass(g_mainActivity);
    if (clazz == nullptr) return nullptr;
    jmethodID method = env->GetMethodID(clazz, name, signature);
    env->DeleteLocalRef(clazz);
    return method;
}

// ============================================================
// JAVA VIRTUAL FILE SYSTEM (VFS)
// ============================================================
static std::string vfsList(const std::string& path) {
    std::lock_guard<std::mutex> lock(g_mutex);
    JNIEnv* env = getJNIEnv();
    if (env == nullptr || g_mainActivity == nullptr) return {};
    jmethodID method = findMethod(env, "vfsList", "(Ljava/lang/String;)Ljava/lang/String;");
    if (method == nullptr) return {};
    jstring jPath = env->NewStringUTF(path.c_str());
    jstring result = static_cast<jstring>(env->CallObjectMethod(g_mainActivity, method, jPath));
    env->DeleteLocalRef(jPath);
    std::string output = jstringToString(env, result);
    if (result != nullptr) env->DeleteLocalRef(result);
    return output;
}

// ============================================================
// EXPORTACIONES JNI (`JNICALL`)
// ============================================================
extern "C" {

JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_initWoWEngine(JNIEnv* env, jobject thiz, jobject surface) {
    if (surface == nullptr) return;
    ANativeWindow* nativeWindow = ANativeWindow_fromSurface(env, surface);
    if (nativeWindow == nullptr) return;

    g_WoWApplication = std::make_unique<Application>();
    g_WoWApplication->Initialize(nativeWindow);
}

JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_renderFrame(JNIEnv* env, jobject thiz) {
    if (g_WoWApplication) g_WoWApplication->RunFrame();
}

JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_connectToServer(JNIEnv* env, jobject thiz, jstring host, jint port, jstring user, jstring pass) {
    std::string c_host = jstringToString(env, host);
    std::string c_user = jstringToString(env, user);
    std::string c_pass = jstringToString(env, pass);

    g_WoWWorldSocket = std::make_unique<WorldSocket>();
    if (g_WoWWorldSocket->Connect(c_host, port)) {
        AuthHandler auth(g_WoWWorldSocket.get());
        auth.StartAuthentication(c_user, c_pass);
    }
}

} // extern "C"
