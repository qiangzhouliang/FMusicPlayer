#include <jni.h>
#include <string>
#include <android/native_window_jni.h>
#include <unistd.h>

#include "util/DZConstDefine.h"
#include "util/DZJNICall.h"
#include "util/DZFFmpeg.h"
#include "util/DZAudio.h"

// 因为FFmpeg是用c语言编写的，我们需要用c的编译器，所有用extern "C" 方式导入
extern "C" {
//    解码
#include <libavcodec/avcodec.h>
//    封装格式
#include "libavformat/avformat.h"
//缩放
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
//重采样
#include "libswresample/swresample.h"
}
DZJNICall *pJniCall;
DZFFmpeg *pFFmpeg;

JavaVM *pJavaVm = NULL;
// 重写 so 被加载时会调用的一个方法
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *javaVM, void *reserved){
    LOGE("JNI_OnLoad -->");
    pJavaVm = javaVM;
    JNIEnv *env;
    if (javaVM->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_4) != JNI_OK){
        return -1;
    }
    return JNI_VERSION_1_4;
}


extern "C" JNIEXPORT jstring JNICALL
Java_com_swan_media_SwanPlayer_stringFromJNI(JNIEnv* env,jobject /* this */) {
    return env->NewStringUTF(av_version_info()); // av_version_info 获取FFmpeg版本
}
extern "C"
JNIEXPORT void JNICALL
Java_com_swan_media_SwanPlayer_nPlay(JNIEnv *env, jobject jPlayerObj) {
    if (pFFmpeg != NULL){
        pFFmpeg->play();
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_swan_media_SwanPlayer_nPrepare(JNIEnv *env, jobject jPlayerObj, jstring url_) {

    const char *url = env->GetStringUTFChars(url_, 0);
    if (pFFmpeg == NULL){
        pJniCall = new DZJNICall(pJavaVm, env, jPlayerObj);
        pFFmpeg = new DZFFmpeg(pJniCall, url);
    }

    pFFmpeg->prepare();
    env->ReleaseStringUTFChars(url_, url);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_swan_media_SwanPlayer_nPrepareAsync(JNIEnv *env, jobject jPlayerObj, jstring url_) {
    const char *url = env->GetStringUTFChars(url_, 0);
    if (pFFmpeg == NULL){
        pJniCall = new DZJNICall(pJavaVm, env, jPlayerObj);
        pFFmpeg = new DZFFmpeg(pJniCall, url);
    }

    pFFmpeg->prepareAsync();
    env->ReleaseStringUTFChars(url_, url);
}

/**
 * 创建 surface
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_swan_media_SwanPlayer_setSurface(JNIEnv *env, jobject thiz, jobject surface) {
    if (pFFmpeg != NULL){
        pFFmpeg->setSurface(surface);
    }
}