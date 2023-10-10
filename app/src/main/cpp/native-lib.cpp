#include <jni.h>
#include <string>
#include <android/native_window_jni.h>
#include <unistd.h>

#include "util/DZConstDefine.h"
#include "util/DZJNICall.h"
#include "util/DZFFmpeg.h"

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

extern "C" JNIEXPORT jstring JNICALL
Java_com_swan_media_SwanPlayer_stringFromJNI(JNIEnv* env,jobject /* this */) {
    return env->NewStringUTF(av_version_info()); // av_version_info 获取FFmpeg版本
}
extern "C"
JNIEXPORT void JNICALL
Java_com_swan_media_SwanPlayer_nPlay(JNIEnv *env, jobject thiz, jstring url_) {
    pJniCall = new DZJNICall(NULL, env);
    const char *url = env->GetStringUTFChars(url_, 0);
    pFFmpeg = new DZFFmpeg(pJniCall, url);
    pFFmpeg->play();
    delete pJniCall;
    delete pFFmpeg;

    env->ReleaseStringUTFChars(url_, url);
}