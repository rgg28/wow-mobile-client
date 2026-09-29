#include <jni.h>
#include <android/log.h>

#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>
#include <cmath>

#define LOG_TAG "WoWMobile"

#define LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static std::mutex g_mutex;

static std::string g_wowFolder;

static std::string g_username;

static std::atomic<bool> g_running(false);

static int g_selectedCharacter = -1;

static float g_joystickX = 0.0f;
static float g_joystickY = 0.0f;

static bool g_modifierL1 = false;
static bool g_modifierR1 = false;

static void engineInit()
{
    std::lock_guard<std::mutex> lock(g_mutex);

    LOGI("================================");
    LOGI("WoW Android Client");
    LOGI("Native engine inicializado");
    LOGI("================================");

    g_running = true;
}

static void engineSetWowFolder(
        const std::string& folder)
{
    std::lock_guard<std::mutex> lock(g_mutex);

    g_wowFolder = folder;

    LOGI(
        "Carpeta WoW seleccionada: %s",
        g_wowFolder.c_str()
    );
}

static void engineLogin(
        const std::string& username,
        const std::string& password)
{
    std::lock_guard<std::mutex> lock(g_mutex);

    g_username = username;

    LOGI(
        "Solicitud de login para cuenta: %s",
        username.c_str()
    );

    /*
     * TODAVIA NO enviamos el protocolo completo.
     *
     * Esta fase solamente confirma que la UI
     * y el puente Java <-> C++ funcionan.
     *
     * Posteriormente:
     *
     * AUTH_LOGON_CHALLENGE
     * AUTH_LOGON_PROOF
     * REALM_LIST
     * CMSG_AUTH_SESSION
     * etc.
     */

    LOGI("Cliente preparado para autenticacion.");
}

static void engineSelectCharacter(
        int index)
{
    std::lock_guard<std::mutex> lock(g_mutex);

    g_selectedCharacter = index;

    LOGI(
        "Personaje seleccionado: %d",
        index
    );

    /*
     * Posteriormente:
     *
     * CMSG_PLAYER_LOGIN
     */
}

static void engineTouch(
        int action,
        float x,
        float y)
{
    LOGI(
        "Touch action=%d x=%f y=%f",
        action,
        x,
        y
    );

    /*
     * Aqui posteriormente implementaremos:
     *
     * - joystick
     * - botones
     * - menu radial
     * - camara
     * - interaccion
     */
}

static void engineJoystick(
        float x,
        float y)
{
    float length =
        std::sqrt(
            x * x +
            y * y
        );

    if (length > 1.0f) {

        x /= length;
        y /= length;
    }

    {
        std::lock_guard<std::mutex> lock(g_mutex);

        g_joystickX = x;
        g_joystickY = y;
    }

    LOGI(
        "Joystick X=%f Y=%f",
        x,
        y
    );
}

static void engineSpell(
        int spellId)
{
    std::lock_guard<std::mutex> lock(g_mutex);

    LOGI(
        "Solicitud de lanzamiento Spell ID=%d",
        spellId
    );

    /*
     * Posteriormente construiremos:
     *
     * CMSG_CAST_SPELL
     *
     * con el formato exacto de 3.3.5a.
     */
}

static void engineJump()
{
    LOGI("Solicitud JUMP");

    /*
     * Posteriormente:
     *
     * CMSG_MOVE_JUMP
     */
}


/* ============================================================
 * JNI
 * ============================================================ */

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeInit(
        JNIEnv* env,
        jobject thiz)
{
    engineInit();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeSetWowFolder(
        JNIEnv* env,
        jobject thiz,
        jstring folder)
{
    if (folder == nullptr)
        return;

    const char* chars =
        env->GetStringUTFChars(
            folder,
            nullptr
        );

    if (chars == nullptr)
        return;

    engineSetWowFolder(
        std::string(chars)
    );

    env->ReleaseStringUTFChars(
        folder,
        chars
    );
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeLogin(
        JNIEnv* env,
        jobject thiz,
        jstring username,
        jstring password)
{
    if (
        username == nullptr ||
        password == nullptr
    )
        return;

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
    )
        return;

    engineLogin(
        std::string(user),
        std::string(pass)
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

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeSelectCharacter(
        JNIEnv* env,
        jobject thiz,
        jint index)
{
    engineSelectCharacter(
        static_cast<int>(index)
    );
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeTouch(
        JNIEnv* env,
        jobject thiz,
        jint action,
        jfloat x,
        jfloat y)
{
    engineTouch(
        static_cast<int>(action),
        static_cast<float>(x),
        static_cast<float>(y)
    );
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeJoystick(
        JNIEnv* env,
        jobject thiz,
        jfloat x,
        jfloat y)
{
    engineJoystick(
        static_cast<float>(x),
        static_cast<float>(y)
    );
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeSpell(
        JNIEnv* env,
        jobject thiz,
        jint spellId)
{
    engineSpell(
        static_cast<int>(spellId)
    );
}

extern "C"
JNIEXPORT void JNICALL
Java_com_wowmobile_client_MainActivity_nativeJump(
        JNIEnv* env,
        jobject thiz)
{
    engineJump();
}
