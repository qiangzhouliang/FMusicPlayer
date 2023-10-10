//
// Created by swan on 2023/10/10.
//

#ifndef FMUSICPLAYER_DZCONSTDEFINE_H
#define FMUSICPLAYER_DZCONSTDEFINE_H
#include <android/log.h>

#define TAG "JNI_TAG"
// 方法宏定义 __VA_ARGS__：固定写法
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#define LOGI(args...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__);

// 采样率
#define AUDIO_SAMPLE_RATE 44100


#endif //FMUSICPLAYER_DZCONSTDEFINE_H
