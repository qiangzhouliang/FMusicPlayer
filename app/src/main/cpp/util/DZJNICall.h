//
// Created by swan on 2023/10/10.
//

#ifndef FMUSICPLAYER_DZJNICALL_H
#define FMUSICPLAYER_DZJNICALL_H
#include <jni.h>

enum ThreadMode{
    THREAD_CHILD,THREAD_MAIN
};

class DZJNICall {
public:
    JavaVM *javaVM;
    JNIEnv *env;
    jmethodID jPlayerErrorMid;
    jmethodID jPlayerPreparedMid;
    jobject jPlayerObj;
public:
    DZJNICall(JavaVM *javaVm, JNIEnv *env, jobject jPlayerObj);
    ~DZJNICall();
public:
    void callPlayerError(ThreadMode threadMode,int code, char *msg);

    void callPlayerPrepared(ThreadMode mode);
};


#endif //FMUSICPLAYER_DZJNICALL_H
