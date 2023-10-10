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

// -----------播放错误码 start----------------------
#define FIND_STREAM_ERROR_CODE -0x10
#define CODEC_FIND_DECODER_ERROR_CODE -0x11
#define CODEC_ALLOC_CONTEXT_ERROR_CODE -0x12
#define SWR_ALLOC_SET_OPTS_ERROR_CODE -0x13
#define SWR_INIT_ERROR_CODE -0x14

// -----------播放错误码 end------------------------


#endif //FMUSICPLAYER_DZCONSTDEFINE_H
