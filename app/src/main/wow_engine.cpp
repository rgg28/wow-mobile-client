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

#define LOG_TAG "WoWEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

JavaVM* g_vm = nullptr;
jobject g_mainActivity = nullptr;
std::string g_wowUri;
std::mutex g_mutex;

class Reader {
public:
    Reader(const std::vector<uint8_t>& bytes) : d(bytes) {}
    size_t size() const { return d.size(); }
    size_t pos() const { return p; }
    bool seek(size_t v) { if (v > d.size()) return false; p = v; return true; }
    bool skip(size_t n) { return seek(p + n); }
    bool canRead(size_t n) const { return p <= d.size() && n <= d.size() - p; }
    uint8_t u8() { return canRead(1) ? d[p++] : 0; }
    uint16_t u16() { if (!canRead(2)) { p = d.size(); return 0; } uint16_t v=d[p]|(uint16_t(d[p+1])<<8); p+=2; return v; }
    uint32_t u32() { if (!canRead(4)) { p=d.size(); return 0; } uint32_t v=uint32_t(d[p])|(uint32_t(d[p+1])<<8)|(uint32_t(d[p+2])<<16)|(uint32_t(d[p+3])<<24); p+=4; return v; }
    float f32() { uint32_t v=u32(); float f=0; std::memcpy(&f,&v,4); return f; }
    const uint8_t* ptr(size_t n=0) const { return canRead(n) ? d.data()+p : nullptr; }
private:
    const std::vector<uint8_t>& d;
    size_t p=0;
};

JNIEnv* getJNIEnv() {
    if (!g_vm) return nullptr;
    JNIEnv* env=nullptr;
    if (g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK) return env;
    if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return nullptr;
    return env;
}

std::string jstringToString(JNIEnv* env, jstring s) {
    if (!env || !s) return {};
    const char* p=env->GetStringUTFChars(s,nullptr);
    if (!p) return {};
    std::string out(p);
    env->ReleaseStringUTFChars(s,p);
    return out;
}

jstring stringToJString(JNIEnv* env, const std::string& s) {
    return env->NewStringUTF(s.c_str());
}

jmethodID findMethod(JNIEnv* env, const char* name, const char* sig) {
    if (!env || !g_mainActivity) return nullptr;
    jclass clazz=env->GetObjectClass(g_mainActivity);
    if (!clazz) return nullptr;
    jmethodID id=env->GetMethodID(clazz,name,sig);
    env->DeleteLocalRef(clazz);
    return id;
}

void clearJavaException(JNIEnv* env, const char* where) {
    if (env && env->ExceptionCheck()) {
        LOGE("Java exception in %s", where);
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}

std::string vfsList(const std::string& path) {
    std::lock_guard<std::mutex> lock(g_mutex);
    JNIEnv* env=getJNIEnv();
    jmethodID mid=findMethod(env,"vfsList","(Ljava/lang/String;)Ljava/lang/String;");
    if (!env || !mid || !g_mainActivity) return {};
    jstring jp=stringToJString(env,path);
    jstring result=(jstring)env->CallObjectMethod(g_mainActivity,mid,jp);
    env->DeleteLocalRef(jp);
    clearJavaException(env,"vfsList");
    std::string out=jstringToString(env,result);
    if (result) env->DeleteLocalRef(result);
    return out;
}

std::vector<uint8_t> vfsReadFile(const std::string& path, int maxBytes) {
    std::lock_guard<std::mutex> lock(g_mutex);
    JNIEnv* env=getJNIEnv();
    jmethodID mid=findMethod(env,"vfsReadFile","(Ljava/lang/String;I)[B");
    if (!env || !mid || !g_mainActivity) return {};
    jstring jp=stringToJString(env,path);
    jbyteArray result=(jbyteArray)env->CallObjectMethod(g_mainActivity,mid,jp,maxBytes);
    env->DeleteLocalRef(jp);
    clearJavaException(env,"vfsReadFile");
    if (!result) return {};
    jsize n=env->GetArrayLength(result);
    std::vector<uint8_t> out;
    if (n>0 && n<=maxBytes) {
        out.resize((size_t)n);
        env->GetByteArrayRegion(result,0,n,reinterpret_cast<jbyte*>(out.data()));
    }
    env->DeleteLocalRef(result);
    clearJavaException(env,"GetByteArrayRegion");
    return out;
}

std::string extension(const std::string& path) {
    size_t p=path.find_last_of('.');
    if (p==std::string::npos) return {};
    std::string e=path.substr(p+1);
    std::transform(e.begin(),e.end(),e.begin(),[](unsigned char c){return (char)std::tolower(c);});
    return e;
}

bool rangeOk(size_t fileSize, uint32_t offset, uint64_t bytes) {
    return uint64_t(offset)+bytes <= fileSize;
}

std::string readBLP(const std::vector<uint8_t>& d) {
    Reader r(d); std::ostringstream o;
    if (d.size()<148 || r.u32()!=0x32504C42u) return "BLP inválido";
    uint32_t version=r.u32(), compression=r.u8(), alphaBits=r.u8(), alphaType=r.u8(), hasMips=r.u8();
    uint32_t width=r.u32(), height=r.u32();
    uint32_t offsets[16], sizes[16];
    for(auto& v:offsets) v=r.u32(); for(auto& v:sizes)v=r.u32();
    o<<"BLP2 version="<<version<<" "<<width<<"x"<<height<<" compression="<<compression<<" alphaBits="<<alphaBits<<" alphaType="<<alphaType<<" hasMips="<<hasMips;
    bool ok=true; for(int i=0;i<16;i++) if(sizes[i] && !rangeOk(d.size(),offsets[i],sizes[i])) ok=false;
    o<<" ranges="<<(ok?"OK":"INVALID");
    return o.str();
}

std::string readM2(const std::vector<uint8_t>& d) {
    Reader r(d); std::ostringstream o;
    if(d.size()<0x48 || r.u32()!=0x3032444Du) return "M2 inválido";
    uint32_t version=r.u32(); r.skip(4);
    uint32_t nGlobal=r.u32(),oGlobal=r.u32(), nAnim=r.u32(),oAnim=r.u32(), nLookup=r.u32(),oLookup=r.u32();
    uint32_t nBones=r.u32(),oBones=r.u32(),nBoneLookup=r.u32(),oBoneLookup=r.u32(),nVerts=r.u32(),oVerts=r.u32(),nViews=r.u32(),oViews=r.u32();
    bool ok=rangeOk(d.size(),oGlobal,uint64_t(nGlobal)*4) && rangeOk(d.size(),oAnim,uint64_t(nAnim)*64) && rangeOk(d.size(),oLookup,uint64_t(nLookup)*2) && rangeOk(d.size(),oBones,uint64_t(nBones)*128) && rangeOk(d.size(),oBoneLookup,uint64_t(nBoneLookup)*2) && rangeOk(d.size(),oVerts,uint64_t(nVerts)*48);
    o<<"MD20 version="<<version<<" animations="<<nAnim<<" bones="<<nBones<<" vertices="<<nVerts<<" views="<<nViews<<" ranges="<<(ok?"OK":"INVALID");
    if(oVerts>=d.size() && nVerts) o<<" vertexOffset=INVALID";
    return o.str();
}

std::string readSKIN(const std::vector<uint8_t>& d) {
    std::ostringstream o;
    if(d.size()<44 || readU32(d,0)!=0x4E494B53u) return "SKIN inválido";
    uint32_t ni=readU32(d,4), oi=readU32(d,8), nt=readU32(d,12), ot=readU32(d,16);
    uint32_t np=readU32(d,20), op=readU32(d,24), ns=readU32(d,28), os=readU32(d,32);
    uint32_t ntu=readU32(d,36), otu=readU32(d,40);
    bool ok=rangeOk(d.size(),oi,uint64_t(ni)*2)&&rangeOk(d.size(),ot,uint64_t(nt)*2)&&rangeOk(d.size(),op,uint64_t(np)*4)&&rangeOk(d.size(),os,uint64_t(ns)*48)&&rangeOk(d.size(),otu,uint64_t(ntu)*40);
    o<<"SKIN indices="<<ni<<" triangles="<<nt<<" properties="<<np<<" submeshes="<<ns<<" textureUnits="<<ntu<<" ranges="<<(ok?"OK":"INVALID");
    return o.str();
}

std::string readANIM(const std::vector<uint8_t>& d) { std::ostringstream o; o<<"ANIM size="<<d.size()<<" bytes"; return o.str(); }
std::string readSBT(const std::vector<uint8_t>& d) { std::ostringstream o; o<<"SBT size="<<d.size()<<" bytes"; return o.str(); }

std::string readResource(const std::string& path) {
    auto d=vfsReadFile(path,16*1024*1024); if(d.empty()) return "No se pudo leer: "+path;
    std::string e=extension(path); if(e=="blp") return readBLP(d); if(e=="m2") return readM2(d); if(e=="skin") return readSKIN(d); if(e=="anim") return readANIM(d); if(e=="sbt") return readSBT(d);
    std::ostringstream o; o<<"Recurso "<<path<<" size="<<d.size()<<" bytes"; return o.str();
}

std::string inspectRoot() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if(g_wowUri.empty()) return "No hay carpeta WoW seleccionada";
    JNIEnv* env=getJNIEnv(); jmethodID mid=findMethod(env,"vfsList","(Ljava/lang/String;)Ljava/lang/String;"); if(!env||!mid||!g_mainActivity) return "VFS no disponible";
    jstring jp=stringToJString(env,""); jstring jr=(jstring)env->CallObjectMethod(g_mainActivity,mid,jp); env->DeleteLocalRef(jp); clearJavaException(env,"inspectRoot/vfsList");
    std::string listing=jstringToString(env,jr); if(jr) env->DeleteLocalRef(jr);
    std::ostringstream o; o<<"WoW 3.3.5a / build 12340\nURI="<<g_wowUri<<"\n";
    std::istringstream in(listing); std::string line; int count=0; while(std::getline(in,line) && count<100) { if(line.rfind("D|",0)==0) {o<<"DIR  "<<line.substr(2)<<"\n"; ++count;} else if(line.rfind("F|",0)==0) {o<<"FILE "<<line.substr(2)<<"\n"; ++count;} }
    if(listing.empty()) o<<"VFS vacío o no pudo listar la raíz\n";
    return o.str();
}

} // namespace

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) { g_vm=vm; return JNI_VERSION_1_6; }

extern "C" JNIEXPORT void JNICALL JNI_OnUnload(JavaVM*, void*) {
    if(g_vm) { JNIEnv* env=nullptr; if(g_vm->GetEnv(reinterpret_cast<void**>(&env),JNI_VERSION_1_6)==JNI_OK && env && g_mainActivity) env->DeleteGlobalRef(g_mainActivity); }
    g_mainActivity=nullptr; g_vm=nullptr;
}

extern "C" JNIEXPORT void JNICALL Java_com_wowmobile_client_MainActivity_nativeInit(JNIEnv* env,jobject thiz) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if(g_mainActivity) env->DeleteGlobalRef(g_mainActivity);
    g_mainActivity=env->NewGlobalRef(thiz);
}

extern "C" JNIEXPORT void JNICALL Java_com_wowmobile_client_MainActivity_nativeSetWowFolder(JNIEnv* env,jobject,jstring uri) {
    std::lock_guard<std::mutex> lock(g_mutex); g_wowUri=jstringToString(env,uri); LOGI("Carpeta WoW seleccionada: %s",g_wowUri.c_str());
}

extern "C" JNIEXPORT jstring JNICALL Java_com_wowmobile_client_MainActivity_nativeInspectRoot(JNIEnv* env,jobject) { return stringToJString(env,inspectRoot()); }
extern "C" JNIEXPORT jstring JNICALL Java_com_wowmobile_client_MainActivity_nativeListDirectory(JNIEnv* env,jobject,jstring path) { return stringToJString(env,vfsList(jstringToString(env,path))); }
extern "C" JNIEXPORT jstring JNICALL Java_com_wowmobile_client_MainActivity_nativeReadResource(JNIEnv* env,jobject,jstring path) { return stringToJString(env,readResource(jstringToString(env,path))); }

extern "C" JNIEXPORT jboolean JNICALL Java_com_wowmobile_client_MainActivity_nativeLogin(JNIEnv*,jobject,jstring,jstring) {
    LOGI("nativeLogin: autenticación WoW real aún no implementada; no se simula éxito"); return JNI_FALSE;
}
