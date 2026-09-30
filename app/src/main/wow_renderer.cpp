#include <jni.h>
#include <android/native_window_jni.h>
#include <android/log.h>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

#define LOG_TAG "WoWRenderer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

struct Vertex { float x,y,z; float nx,ny,nz; };
struct Mat4 { float m[16]{}; };

std::mutex gMutex;
ANativeWindow* gWindow=nullptr;
EGLDisplay gDisplay=EGL_NO_DISPLAY;
EGLSurface gSurface=EGL_NO_SURFACE;
EGLContext gContext=EGL_NO_CONTEXT;
std::thread gRenderThread;
std::atomic<bool> gRunning(false);
std::atomic<bool> gSurfaceReady(false);
int gWidth=1,gHeight=1;
float gCameraX=0,gCameraY=0,gZoom=1;
float gLastTouchX=0,gLastTouchY=0;
bool gTouchInitialized=false;

std::vector<Vertex> gCharacterVertices;
std::vector<uint16_t> gCharacterIndices;
std::vector<Vertex> gWorldVertices;
std::vector<uint32_t> gWorldIndices;

bool gCharacterDirty=false,gWorldDirty=false;
enum SceneType { SCENE_NONE=0, SCENE_CHARACTER=1, SCENE_WORLD=2 };
SceneType gScene=SCENE_NONE;

GLuint gProgram=0,gVbo=0,gEbo=0,gWorldVbo=0,gWorldEbo=0;
GLint gMvpLocation=-1,gModelLocation=-1;
GLsizei gIndexCount=0,gWorldIndexCount=0;

float gCharacterCenterX=0,gCharacterCenterY=0,gCharacterCenterZ=0;
float gCharacterRadius=1;
float gWorldCenterX=0,gWorldCenterY=0,gWorldCenterZ=0,gWorldRadius=100;

Mat4 identity(){ Mat4 r{}; r.m[0]=r.m[5]=r.m[10]=r.m[15]=1; return r; }
Mat4 multiply(const Mat4&a,const Mat4&b){ Mat4 r{}; for(int c=0;c<4;c++)for(int row=0;row<4;row++)r.m[c*4+row]=a.m[row]*b.m[c*4]+a.m[4+row]*b.m[c*4+1]+a.m[8+row]*b.m[c*4+2]+a.m[12+row]*b.m[c*4+3]; return r; }
Mat4 perspective(float fov,float aspect,float nearPlane,float farPlane){ Mat4 r{}; float f=1.0f/std::tan(fov*0.5f); r.m[0]=f/aspect;r.m[5]=f;r.m[10]=(farPlane+nearPlane)/(nearPlane-farPlane);r.m[11]=-1;r.m[14]=(2*farPlane*nearPlane)/(nearPlane-farPlane);return r; }
Mat4 translation(float x,float y,float z){ Mat4 r=identity();r.m[12]=x;r.m[13]=y;r.m[14]=z;return r; }
Mat4 rotationY(float a){ Mat4 r=identity();float c=std::cos(a),s=std::sin(a);r.m[0]=c;r.m[2]=-s;r.m[8]=s;r.m[10]=c;return r; }
Mat4 rotationX(float a){ Mat4 r=identity();float c=std::cos(a),s=std::sin(a);r.m[5]=c;r.m[6]=s;r.m[9]=-s;r.m[10]=c;return r; }

GLuint compileShader(GLenum type,const char* source){ GLuint s=glCreateShader(type);glShaderSource(s,1,&source,nullptr);glCompileShader(s);GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){char log[2048]{};glGetShaderInfoLog(s,sizeof(log),nullptr,log);LOGE("Shader error: %s",log);glDeleteShader(s);return 0;}return s; }

bool createProgram(){
 const char* vs=R"(#version 300 es
 layout(location=0) in vec3 aPosition;
 layout(location=1) in vec3 aNormal;
 uniform mat4 uMVP;
 uniform mat4 uModel;
 out vec3 vNormal;
 void main(){ gl_Position=uMVP*vec4(aPosition,1.0); vNormal=mat3(uModel)*aNormal; })";
 const char* fs=R"(#version 300 es
 precision mediump float;
 in vec3 vNormal;
 out vec4 fragColor;
 void main(){ vec3 n=normalize(vNormal); vec3 light=normalize(vec3(-0.4,0.8,0.5)); float diffuse=max(dot(n,light),0.0); float value=0.25+diffuse*0.75; fragColor=vec4(value*0.72,value*0.78,value*0.88,1.0); })";
 GLuint v=compileShader(GL_VERTEX_SHADER,vs),f=compileShader(GL_FRAGMENT_SHADER,fs); if(!v||!f){if(v)glDeleteShader(v);if(f)glDeleteShader(f);return false;}
 gProgram=glCreateProgram();glAttachShader(gProgram,v);glAttachShader(gProgram,f);glLinkProgram(gProgram);glDeleteShader(v);glDeleteShader(f);GLint linked=0;glGetProgramiv(gProgram,GL_LINK_STATUS,&linked);if(!linked){char log[2048]{};glGetProgramInfoLog(gProgram,sizeof(log),nullptr,log);LOGE("Program link error: %s",log);glDeleteProgram(gProgram);gProgram=0;return false;} gMvpLocation=glGetUniformLocation(gProgram,"uMVP");gModelLocation=glGetUniformLocation(gProgram,"uModel");return gMvpLocation>=0&&gModelLocation>=0;
}

uint32_t readU32(const std::vector<uint8_t>&d,size_t p){if(p+4>d.size())return 0;return uint32_t(d[p])|(uint32_t(d[p+1])<<8)|(uint32_t(d[p+2])<<16)|(uint32_t(d[p+3])<<24);}
uint16_t readU16(const std::vector<uint8_t>&d,size_t p){if(p+2>d.size())return 0;return uint16_t(d[p])|uint16_t(d[p+1]<<8);}
float readF32(const std::vector<uint8_t>&d,size_t p){uint32_t v=readU32(d,p);float f=0;std::memcpy(&f,&v,4);return f;}
bool tagAt(const std::vector<uint8_t>&d,size_t p,const char*t){return p+4<=d.size()&&d[p]==t[0]&&d[p+1]==t[1]&&d[p+2]==t[2]&&d[p+3]==t[3];}

bool parseM2(const std::vector<uint8_t>&d){
 gCharacterVertices.clear();gCharacterIndices.clear();
 if(d.size()<0x48||!tagAt(d,0,"MD20"))return false;
 uint32_t n=readU32(d,0x40),ofs=readU32(d,0x44); if(!n||n>1000000||uint64_t(ofs)+uint64_t(n)*48>d.size())return false;
 gCharacterVertices.reserve(n);
 for(uint32_t i=0;i<n;i++){size_t p=ofs+size_t(i)*48;Vertex v{};v.x=readF32(d,p);v.y=readF32(d,p+4);v.z=readF32(d,p+8);v.nx=readF32(d,p+20);v.ny=readF32(d,p+24);v.nz=readF32(d,p+28);gCharacterVertices.push_back(v);}
 float minX=1e30f,minY=1e30f,minZ=1e30f,maxX=-1e30f,maxY=-1e30f,maxZ=-1e30f;for(const auto&v:gCharacterVertices){minX=std::min(minX,v.x);minY=std::min(minY,v.y);minZ=std::min(minZ,v.z);maxX=std::max(maxX,v.x);maxY=std::max(maxY,v.y);maxZ=std::max(maxZ,v.z);}gCharacterCenterX=(minX+maxX)*.5f;gCharacterCenterY=(minY+maxY)*.5f;gCharacterCenterZ=(minZ+maxZ)*.5f;float dx=maxX-minX,dy=maxY-minY,dz=maxZ-minZ;gCharacterRadius=std::max(.01f,std::sqrt(dx*dx+dy*dy+dz*dz)*.5f);
 return true;
}

bool parseSkin(const std::vector<uint8_t>&d){
 gCharacterIndices.clear();if(d.size()<44||!tagAt(d,0,"SKIN"))return false;
 uint32_t ni=readU32(d,4),oi=readU32(d,8),nt=readU32(d,12),ot=readU32(d,16);if(!ni||!nt||uint64_t(oi)+uint64_t(ni)*2>d.size()||uint64_t(ot)+uint64_t(nt)*2>d.size())return false;
 std::vector<uint16_t> indices(ni);for(uint32_t i=0;i<ni;i++)indices[i]=readU16(d,oi+i*2);gCharacterIndices.reserve(nt);for(uint32_t i=0;i<nt;i++){uint16_t local=readU16(d,ot+i*2);if(local>=indices.size())continue;uint16_t real=indices[local];if(real>=gCharacterVertices.size())continue;gCharacterIndices.push_back(real);}return gCharacterIndices.size()>=3;
}

bool findSubChunk(const std::vector<uint8_t>&d,size_t start,size_t end,const char*w,size_t&payload,size_t&size){end=std::min(end,d.size());for(size_t p=start;p+8<=end;){uint32_t n=readU32(d,p+4);size_t e=p+8+size_t(n);if(e>end||e>d.size())return false;if(tagAt(d,p,w)){payload=p+8;size=n;return true;}p=e;}return false;}

bool parseMCNK(const std::vector<uint8_t>&d,size_t mcnk,size_t end,int cx,int cy){
 size_t payload=0,sz=0;if(!findSubChunk(d,mcnk+8,end,"MCVT",payload,sz)||sz<580)return false;float baseZ=readF32(d,mcnk+0x68);float h[145];for(int i=0;i<145;i++)h[i]=readF32(d,payload+i*4);
 const float chunkSize=33.333333f,step=chunkSize/8.0f;std::vector<Vertex> outer;outer.reserve(81);int k=0;for(int y=0;y<9;y++){for(int x=0;x<9;x++){float wx=cx*chunkSize+x*step;float wz=cy*chunkSize+y*step;outer.push_back({wx,baseZ+h[k++],wz,0,1,0});}if(y<8)k+=8;}
 uint32_t base=(uint32_t)gWorldVertices.size();gWorldVertices.insert(gWorldVertices.end(),outer.begin(),outer.end());for(int y=0;y<8;y++)for(int x=0;x<8;x++){uint32_t a=base+y*9+x,b=a+1,c=base+(y+1)*9+x,dv=c+1;gWorldIndices.push_back(a);gWorldIndices.push_back(c);gWorldIndices.push_back(b);gWorldIndices.push_back(b);gWorldIndices.push_back(c);gWorldIndices.push_back(dv);}return true;
}

bool parseADT(const std::vector<uint8_t>&d){
 gWorldVertices.clear();gWorldIndices.clear();if(d.size()<8)return false;size_t p=0;int found=0;while(p+8<=d.size()){uint32_t n=readU32(d,p+4);size_t e=p+8+size_t(n);if(e>d.size())break;if(tagAt(d,p,"MCNK")){int cx=found%16,cy=found/16;if(parseMCNK(d,p,e,cx,cy))found++;}p=e;}
 if(gWorldVertices.empty()||gWorldIndices.empty())return false;float minX=1e30f,minY=1e30f,minZ=1e30f,maxX=-1e30f,maxY=-1e30f,maxZ=-1e30f;for(const auto&v:gWorldVertices){minX=std::min(minX,v.x);minY=std::min(minY,v.y);minZ=std::min(minZ,v.z);maxX=std::max(maxX,v.x);maxY=std::max(maxY,v.y);maxZ=std::max(maxZ,v.z);}gWorldCenterX=(minX+maxX)*.5f;gWorldCenterY=(minY+maxY)*.5f;gWorldCenterZ=(minZ+maxZ)*.5f;float dx=maxX-minX,dy=maxY-minY,dz=maxZ-minZ;gWorldRadius=std::max(10.0f,std::sqrt(dx*dx+dy*dy+dz*dz)*.5f);LOGI("ADT chunks=%d vertices=%zu indices=%zu",found,gWorldVertices.size(),gWorldIndices.size());return true;
}

void uploadCharacter(){if(!gCharacterDirty)return;gCharacterDirty=false;if(gVbo)glDeleteBuffers(1,&gVbo);if(gEbo)glDeleteBuffers(1,&gEbo);glGenBuffers(1,&gVbo);glGenBuffers(1,&gEbo);glBindBuffer(GL_ARRAY_BUFFER,gVbo);glBufferData(GL_ARRAY_BUFFER,(GLsizeiptr)(gCharacterVertices.size()*sizeof(Vertex)),gCharacterVertices.data(),GL_STATIC_DRAW);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,gEbo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,(GLsizeiptr)(gCharacterIndices.size()*sizeof(uint16_t)),gCharacterIndices.data(),GL_STATIC_DRAW);gIndexCount=(GLsizei)gCharacterIndices.size();}
void uploadWorld(){if(!gWorldDirty)return;gWorldDirty=false;if(gWorldVbo)glDeleteBuffers(1,&gWorldVbo);if(gWorldEbo)glDeleteBuffers(1,&gWorldEbo);glGenBuffers(1,&gWorldVbo);glGenBuffers(1,&gWorldEbo);glBindBuffer(GL_ARRAY_BUFFER,gWorldVbo);glBufferData(GL_ARRAY_BUFFER,(GLsizeiptr)(gWorldVertices.size()*sizeof(Vertex)),gWorldVertices.data(),GL_STATIC_DRAW);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,gWorldEbo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,(GLsizeiptr)(gWorldIndices.size()*sizeof(uint32_t)),gWorldIndices.data(),GL_STATIC_DRAW);gWorldIndexCount=(GLsizei)gWorldIndices.size();}

void deleteGLObjects(){if(gVbo){glDeleteBuffers(1,&gVbo);gVbo=0;}if(gEbo){glDeleteBuffers(1,&gEbo);gEbo=0;}if(gWorldVbo){glDeleteBuffers(1,&gWorldVbo);gWorldVbo=0;}if(gWorldEbo){glDeleteBuffers(1,&gWorldEbo);gWorldEbo=0;}if(gProgram){glDeleteProgram(gProgram);gProgram=0;}gMvpLocation=gModelLocation=-1;}

bool initEGL(){
 gDisplay=eglGetDisplay(EGL_DEFAULT_DISPLAY);if(gDisplay==EGL_NO_DISPLAY||!eglInitialize(gDisplay,nullptr,nullptr))return false;
 const EGLint attrs[]={EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_DEPTH_SIZE,24,EGL_NONE};EGLConfig config{};EGLint n=0;if(!eglChooseConfig(gDisplay,attrs,&config,1,&n)||n==0)return false;
 const EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};gContext=eglCreateContext(gDisplay,config,EGL_NO_CONTEXT,ca);if(gContext==EGL_NO_CONTEXT)return false;gSurface=eglCreateWindowSurface(gDisplay,config,gWindow,nullptr);if(gSurface==EGL_NO_SURFACE)return false;if(!eglMakeCurrent(gDisplay,gSurface,gSurface,gContext))return false;eglSwapInterval(gDisplay,1);if(!createProgram())return false;glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);glCullFace(GL_BACK);glClearColor(.015f,.025f,.055f,1);return true;
}

void shutdownEGL(){
 if(gDisplay!=EGL_NO_DISPLAY){if(gContext!=EGL_NO_CONTEXT&&gSurface!=EGL_NO_SURFACE) {eglMakeCurrent(gDisplay,gSurface,gSurface,gContext);deleteGLObjects();} eglMakeCurrent(gDisplay,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);if(gSurface!=EGL_NO_SURFACE)eglDestroySurface(gDisplay,gSurface);if(gContext!=EGL_NO_CONTEXT)eglDestroyContext(gDisplay,gContext);eglTerminate(gDisplay);}gDisplay=EGL_NO_DISPLAY;gSurface=EGL_NO_SURFACE;gContext=EGL_NO_CONTEXT;
}

void drawScene(){
 glViewport(0,0,gWidth,gHeight);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);if(!gProgram)return;uploadCharacter();uploadWorld();glUseProgram(gProgram);
 float aspect=(float)gWidth/(float)std::max(1,gHeight);float radius=gScene==SCENE_WORLD?gWorldRadius:gCharacterRadius;float distance=std::max(.5f,radius*(gScene==SCENE_WORLD?(1.35f+gZoom*.35f):(2.5f+gZoom*.8f)));
 Mat4 model=identity();if(gScene==SCENE_WORLD)model=translation(-gWorldCenterX,-gWorldCenterY,-gWorldCenterZ);else model=translation(-gCharacterCenterX,-gCharacterCenterY,-gCharacterCenterZ);
 Mat4 projection=perspective(1.0f,aspect,std::max(.01f,radius*.01f),std::max(100.0f,radius*20.0f));Mat4 camera=translation(0,-distance*.05f,-distance);Mat4 view=multiply(camera,multiply(rotationX(gCameraY),rotationY(gCameraX)));Mat4 mvp=multiply(projection,multiply(view,model));glUniformMatrix4fv(gMvpLocation,1,GL_FALSE,mvp.m);glUniformMatrix4fv(gModelLocation,1,GL_FALSE,model.m);glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);
 if(gScene==SCENE_CHARACTER&&gVbo&&gEbo&&gIndexCount){glBindBuffer(GL_ARRAY_BUFFER,gVbo);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,gEbo);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)0);glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)(3*sizeof(float)));glDrawElements(GL_TRIANGLES,gIndexCount,GL_UNSIGNED_SHORT,nullptr);}
 else if(gScene==SCENE_WORLD&&gWorldVbo&&gWorldEbo&&gWorldIndexCount){glBindBuffer(GL_ARRAY_BUFFER,gWorldVbo);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,gWorldEbo);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)0);glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)(3*sizeof(float)));glDrawElements(GL_TRIANGLES,gWorldIndexCount,GL_UNSIGNED_INT,nullptr);}
 glDisableVertexAttribArray(0);glDisableVertexAttribArray(1);eglSwapBuffers(gDisplay,gSurface);
}

void renderLoop(){LOGI("Render thread iniciado");if(!initEGL()){LOGE("No se pudo iniciar EGL");gRunning=false;shutdownEGL();return;}gSurfaceReady=true;while(gRunning){{std::lock_guard<std::mutex> lock(gMutex);gWidth=std::max(1,gWidth);gHeight=std::max(1,gHeight);drawScene();}std::this_thread::sleep_for(std::chrono::milliseconds(16));}gSurfaceReady=false;shutdownEGL();LOGI("Render thread detenido");}

void stopRendererInternal(){gRunning=false;if(gRenderThread.joinable())gRenderThread.join();if(gWindow){ANativeWindow_release(gWindow);gWindow=nullptr;}}

} // namespace

extern "C" JNIEXPORT jint JNICALL Java_com_wowmobile_client_MainActivity_nativeRendererGetBackend(JNIEnv*,jobject){return 1;}
extern "C" JNIEXPORT void JNICALL Java_com_wowmobile_client_MainActivity_nativeRendererSetBackend(JNIEnv*,jobject,jint backend){LOGI("Backend seleccionado: %d",backend);}

extern "C" JNIEXPORT void JNICALL Java_com_wowmobile_client_MainActivity_nativeRendererSetSurface(JNIEnv* env,jobject,jobject surface){
 // A new Android Surface replaces the old EGL surface. Stop first so the render thread can
 // release its EGL objects before the ANativeWindow is replaced.
 if(gRunning) stopRendererInternal();
 std::lock_guard<std::mutex> lock(gMutex);if(gWindow){ANativeWindow_release(gWindow);gWindow=nullptr;}gWindow=ANativeWindow_fromSurface(env,surface);if(!gWindow){LOGE("No se pudo obtener ANativeWindow");return;}gRunning=true;gRenderThread=std::thread(renderLoop);
}

extern "C" JNIEXPORT void JNICALL Java_com_wowmobile_client_MainActivity_nativeRendererResize(JNIEnv*,jobject,jint width,jint height){std::lock_guard<std::mutex> lock(gMutex);gWidth=std::max(1,(int)width);gHeight=std::max(1,(int)height);}
extern "C" JNIEXPORT void JNICALL Java_com_wowmobile_client_MainActivity_nativeRendererStop(JNIEnv*,jobject){stopRendererInternal();}

extern "C" JNIEXPORT void JNICALL Java_com_wowmobile_client_MainActivity_nativeRendererCamera(JNIEnv*,jobject,jfloat x,jfloat y){std::lock_guard<std::mutex> lock(gMutex);if(!gTouchInitialized){gLastTouchX=x;gLastTouchY=y;gTouchInitialized=true;return;}float dx=x-gLastTouchX,dy=y-gLastTouchY;gLastTouchX=x;gLastTouchY=y;gCameraX+=dx*.008f;gCameraY=std::clamp(gCameraY+dy*.005f,-1.3f,1.3f);}
extern "C" JNIEXPORT void JNICALL Java_com_wowmobile_client_MainActivity_nativeRendererZoom(JNIEnv*,jobject,jfloat delta){std::lock_guard<std::mutex> lock(gMutex);gZoom=std::clamp(gZoom+delta*.08f,.1f,8.0f);}

extern "C" JNIEXPORT jboolean JNICALL Java_com_wowmobile_client_MainActivity_nativeRendererLoadCharacter(JNIEnv* env,jobject,jbyteArray m2Array,jbyteArray skinArray){
 if(!m2Array)return JNI_FALSE;jsize n=env->GetArrayLength(m2Array);if(n<=0||n>64*1024*1024)return JNI_FALSE;std::vector<uint8_t>m2((size_t)n);env->GetByteArrayRegion(m2Array,0,n,reinterpret_cast<jbyte*>(m2.data()));std::vector<uint8_t>skin;if(skinArray){jsize s=env->GetArrayLength(skinArray);if(s>0&&s<=16*1024*1024){skin.resize((size_t)s);env->GetByteArrayRegion(skinArray,0,s,reinterpret_cast<jbyte*>(skin.data()));}}
 std::lock_guard<std::mutex> lock(gMutex);if(!parseM2(m2)){LOGE("No se pudo interpretar M2");return JNI_FALSE;}if(!skin.empty()){if(!parseSkin(skin)){LOGE("No se pudo interpretar SKIN");return JNI_FALSE;}}else{gCharacterIndices.clear();if(gCharacterVertices.size()>65535)return JNI_FALSE;for(size_t i=0;i+2<gCharacterVertices.size();i+=3){gCharacterIndices.push_back((uint16_t)i);gCharacterIndices.push_back((uint16_t)(i+1));gCharacterIndices.push_back((uint16_t)(i+2));}}
 if(gCharacterIndices.empty())return JNI_FALSE;gScene=SCENE_CHARACTER;gCharacterDirty=true;gZoom=1;gCameraX=0;gCameraY=0;gTouchInitialized=false;LOGI("Personaje cargado: vertices=%zu indices=%zu",gCharacterVertices.size(),gCharacterIndices.size());return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_wowmobile_client_MainActivity_nativeRendererLoadWorld(JNIEnv* env,jobject,jbyteArray adtArray){
 if(!adtArray)return JNI_FALSE;jsize n=env->GetArrayLength(adtArray);if(n<=0||n>64*1024*1024)return JNI_FALSE;std::vector<uint8_t>d((size_t)n);env->GetByteArrayRegion(adtArray,0,n,reinterpret_cast<jbyte*>(d.data()));std::lock_guard<std::mutex> lock(gMutex);if(!parseADT(d)){LOGE("No se pudo interpretar ADT");return JNI_FALSE;}gScene=SCENE_WORLD;gWorldDirty=true;gCameraX=0;gCameraY=-.55f;gZoom=1;gTouchInitialized=false;LOGI("Mundo ADT preparado");return JNI_TRUE;
}
