//
// Created by swan on 2023/10/10.
//

#include <pthread.h>
#include "DZFFmpeg.h"
#include "DZConstDefine.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

DZFFmpeg::DZFFmpeg(DZJNICall *pJinCall, const char *url) {
    this->pJinCall = pJinCall;
    // 赋值一份 url，因为怕外面方法结束销毁 url
    this->url = static_cast<char *>(malloc(strlen(url) + 1));
    memcpy(this->url, url, strlen(url)+1);
}

DZFFmpeg::~DZFFmpeg() {
    release();
}

void DZFFmpeg::play() {
    if (pAudio != NULL){
        pAudio->play();
    }
}

void DZFFmpeg::callPlayerJniError(ThreadMode threadMode,int code, char *msg) {
    // 释放资源
    release();

    // 回调给 java层
    pJinCall->callPlayerError(threadMode,code, msg);
}

void DZFFmpeg::release() {

    if (pFormatContext != NULL){
        avformat_close_input(&pFormatContext);
        avformat_free_context(pFormatContext);
        pFormatContext == NULL;
    }

    avformat_network_deinit();
    if (url != NULL){
        free(url);
        url = NULL;
    }
}

void DZFFmpeg::prepare() {
    prepare(THREAD_MAIN);
}

void* threadPrepare(void* context){
    DZFFmpeg* pFFmpeg = static_cast<DZFFmpeg *>(context);
    pFFmpeg->prepare(THREAD_CHILD);
    return 0;
}

void DZFFmpeg::prepareAsync() {
    // 创建一个线程去播放，多线程编解码边播放
    pthread_t prepareThreadT;
    pthread_create(&prepareThreadT, NULL, threadPrepare, this);
    pthread_detach(prepareThreadT);
}



void DZFFmpeg::prepare(ThreadMode threadMode) {
    // 讲的理念
    // 初始化所有组件，只有调用了该函数，才能使用复用器和编解码器
    av_register_all();
    // 初始化网络
    avformat_network_init();

    int formatOpenInputRes = 0; // 0 成功，非0 不成功
    int formatFindStreamInfoRes = 0; // >= 0 成功

    // 函数会读文件头，对 mp4 文件而言，它会解析所有的 box。但它知识把读到的结果保存在对应的数据结构下
    formatOpenInputRes = avformat_open_input(&pFormatContext, url, NULL, NULL);
    if (formatOpenInputRes != 0){
        // 1. 需要回调给Java层
        // 2. 需要释放资源
        //return;
        LOGE("format open input error: %s", av_err2str(formatOpenInputRes));
        callPlayerJniError(threadMode,formatOpenInputRes, av_err2str(formatOpenInputRes));
        return;
    }

    // 读取一部分视音频数据并且获得一些相关的信息，会检测一些重要字段，如果是空白的，就设法填充它们
    formatFindStreamInfoRes = avformat_find_stream_info(pFormatContext, NULL);
    if (formatFindStreamInfoRes < 0){
        LOGE("format find stream info error: %s", av_err2str(formatFindStreamInfoRes));
        callPlayerJniError(threadMode,formatFindStreamInfoRes, av_err2str(formatFindStreamInfoRes));
        return;
    }

    // 获取音视频及字幕的 stream_index , 以前没有这个函数时，我们一般都是写的 for 循环。
    int audioStreamIndex = av_find_best_stream(pFormatContext, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (audioStreamIndex < 0){
        LOGE("find_best_stream error: %s");
        callPlayerJniError(threadMode,FIND_STREAM_ERROR_CODE, "find_best_stream error");
        return;
    }

    // 不是我的事我不干，但是也不要想的过于复杂
    pAudio = new DZAudio(audioStreamIndex, pJinCall, pFormatContext);
    pAudio->analysisStream(threadMode, pFormatContext->streams);

    /*// 1s 44100 点 2通道，2字节 => 44100 * 2 *2
    // 1帧不是1s，pFrame->nb_sampples 点
    // size是播放指定的大小，是最终输出的大小
    int outChannels = av_get_channel_layout_nb_channels(out_ch_layout); // 输出的通道
    int dataSize = av_samples_get_buffer_size(NULL,outChannels,
                                              pCodecContext->frame_size,out_sample_fmt,0);
    resampleOutBuffer = static_cast<uint8_t *>(malloc(dataSize));*/
    // -----------重采样---end------------------------
    // 回调到java层，告诉他准备好了
    pJinCall->callPlayerPrepared(threadMode);
}
