//
// Created by swan on 2023/10/10.
//

#ifndef FMUSICPLAYER_DZJNICALL_H
#define FMUSICPLAYER_DZJNICALL_H
#include <jni.h>

class DZJNICall {
public:
    jobject jAudioTrackObj;
    jmethodID jAudioTrackWriteMid;
    JavaVM *javaVm;
    JNIEnv *env;
    jmethodID jPlayerErrorMid;
    jobject jPlayerObj;
public:
    DZJNICall(JavaVM *javaVm, JNIEnv *env, jobject jPlayerObj);
    ~DZJNICall();
private:
    void initCreateAudioTrack();
public:
    void callAudioTrackWrite(jbyteArray audioData, int offsetInBytes, int sizeInBytes);

    void callPlayerError(int code, char *msg);
};


#endif //FMUSICPLAYER_DZJNICALL_H
