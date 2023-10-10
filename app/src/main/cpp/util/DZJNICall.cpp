//
// Created by swan on 2023/10/10.
//
#include <jni.h>
#include "DZJNICall.h"
#include "DZConstDefine.h"

DZJNICall::DZJNICall(JavaVM *javaVm, JNIEnv *env) : javaVm(javaVm), env(env) {
    this->javaVm = javaVm;
    this->env = env;
    initCreateAudioTrack();
}

void DZJNICall::initCreateAudioTrack() {
    jclass jAudioTrackClass = env->FindClass("android/media/AudioTrack");
    jmethodID jAudioTrackCMid = env->GetMethodID(jAudioTrackClass, "<init>", "(IIIIII)V");

    //  public static final int STREAM_MUSIC = 3;
    int streamType = 3;
    int sampleRateInHz = AUDIO_SAMPLE_RATE;
    // public static final int CHANNEL_OUT_STEREO = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT);
    int channelConfig = (0x4 | 0x8); // 立体声
    // public static final int ENCODING_PCM_16BIT = 2;
    int audioFormat = 2;
    // getMinBufferSize(int sampleRateInHz, int channelConfig, int audioFormat)
    jmethodID jGetMinBufferSizeMid = env->GetStaticMethodID(jAudioTrackClass, "getMinBufferSize", "(III)I");
    int bufferSizeInBytes = env->CallStaticIntMethod(jAudioTrackClass, jGetMinBufferSizeMid, sampleRateInHz, channelConfig, audioFormat);
    // public static final int MODE_STREAM = 1;
    int mode = 1;
    jAudioTrackObj = env->NewObject(jAudioTrackClass, jAudioTrackCMid, streamType, sampleRateInHz, channelConfig, audioFormat, bufferSizeInBytes, mode);

    //  play()
    jmethodID jPlayMid = env->GetMethodID(jAudioTrackClass, "play", "()V");
    env->CallVoidMethod(jAudioTrackObj, jPlayMid);

    // write method
    jAudioTrackWriteMid = env->GetMethodID(jAudioTrackClass, "write", "([BII)I");
}

void DZJNICall::callAudioTrackWrite(jbyteArray audioData, int offsetInBytes, int sizeInBytes) {
    // 调用 Write 方法写入数据
    env->CallIntMethod(jAudioTrackObj, jAudioTrackWriteMid, audioData, offsetInBytes, sizeInBytes);
}

DZJNICall::~DZJNICall() {
    env->DeleteLocalRef(jAudioTrackObj);
}
