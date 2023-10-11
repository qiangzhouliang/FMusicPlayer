//
// Created by swan on 2023/10/10.
//
#include <jni.h>
#include "DZJNICall.h"
#include "DZConstDefine.h"

DZJNICall::DZJNICall(JavaVM *javaVm, JNIEnv *env, jobject jPlayerObj) {
    this->javaVM = javaVm;
    this->env = env;
    this->jPlayerObj = env->NewGlobalRef(jPlayerObj);


//    jclass jPlayerClass = env->FindClass("com/swan/media/SwanPlayer");
    jclass jPlayerClass = env->GetObjectClass(jPlayerObj);
    jPlayerErrorMid = env->GetMethodID(jPlayerClass, "onError", "(ILjava/lang/String;)V");
    jPlayerPreparedMid = env->GetMethodID(jPlayerClass, "onPrepared", "()V");
    LOGE("------------>");
}



DZJNICall::~DZJNICall() {
    env->DeleteGlobalRef(jPlayerObj);
}

void DZJNICall::callPlayerError(ThreadMode threadMode,int code, char *msg) {
    // 子线程用不了主线程 jniEnv （native 线程）
    // 子线程是不共享 jniEnv ，他们有自己所独有的
    if (threadMode == THREAD_MAIN) {
        jstring jMsg = env->NewStringUTF(msg);
        env->CallVoidMethod(jPlayerObj, jPlayerErrorMid, code, jMsg);
        env->DeleteLocalRef(jMsg);
    } else if (threadMode == THREAD_CHILD) {
        // 获取当前线程的 JNIEnv， 通过 JavaVM
        JNIEnv *env;
        if (javaVM->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("get child thread jniEnv error!");
            return;
        }

        jstring jMsg = env->NewStringUTF(msg);
        env->CallVoidMethod(jPlayerObj, jPlayerErrorMid, code, jMsg);
        env->DeleteLocalRef(jMsg);

        javaVM->DetachCurrentThread();
    }
}
/**
 * 回调到java层，告诉他准备好了
 * @param mode
 */
void DZJNICall::callPlayerPrepared(ThreadMode threadMode) {
// 子线程用不了主线程 jniEnv （native 线程）
    // 子线程是不共享 jniEnv ，他们有自己所独有的
    if (threadMode == THREAD_MAIN) {
        env->CallVoidMethod(jPlayerObj, jPlayerPreparedMid);
    } else if (threadMode == THREAD_CHILD) {
        // 获取当前线程的 JNIEnv， 通过 JavaVM
        JNIEnv *env;
        if (javaVM->AttachCurrentThread(&env, 0) != JNI_OK) {
            LOGE("get child thread jniEnv error!");
            return;
        }

        env->CallVoidMethod(jPlayerObj, jPlayerPreparedMid);
        javaVM->DetachCurrentThread();
    }
}
