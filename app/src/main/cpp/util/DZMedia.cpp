//
// Created by swan on 2023/10/12.
//

#include "DZMedia.h"
#include "DZConstDefine.h"

DZMedia::DZMedia(int streamIndex, DZJNICall *pJinCall, DZPlayerStatus *pPlayerStatus) : streamIndex(
    streamIndex), pJinCall(pJinCall), pPlayerStatus(pPlayerStatus) {
    pPacketQueue = new DZPacketQueue();
}

DZMedia::~DZMedia() {
    release();
}

void DZMedia::analysisStream(ThreadMode threadMode, AVFormatContext *pFormatContext) {
    publicAnalysisStream(threadMode, pFormatContext);
    privateAnalysisStream(threadMode, pFormatContext);
}

void DZMedia::publicAnalysisStream(ThreadMode threadMode, AVFormatContext *pFormatContext) {
// 查找解码器
    AVCodecParameters *pCodecParameters = pFormatContext->streams[streamIndex]->codecpar;
    AVCodec *pCodec = avcodec_find_decoder(pCodecParameters->codec_id);
    if (pCodec == NULL){
        LOGE("avcodec_find_decoder error");
        callPlayerJniError(threadMode,CODEC_FIND_DECODER_ERROR_CODE, "avcodec_find_decoder error");
        return;
    }

    // 打开解码器
    pCodecContext = avcodec_alloc_context3(pCodec);
    // 将 参数 拷到 context
    if (pCodecContext == NULL){
        LOGE("avcodec_alloc_context3 error");
        callPlayerJniError(threadMode,CODEC_ALLOC_CONTEXT_ERROR_CODE, "avcodec_alloc_context3 error");
        return;
    }
    int codecParametersToContextRes = avcodec_parameters_to_context(pCodecContext, pCodecParameters);
    if (codecParametersToContextRes < 0){
        LOGE("avcodec_parameters_to_context error: %s", av_err2str(codecParametersToContextRes));
        callPlayerJniError(threadMode,codecParametersToContextRes, av_err2str(codecParametersToContextRes));
        return;
    }

    int avcodecOpenRes = avcodec_open2(pCodecContext, pCodec, NULL);
    if (avcodecOpenRes != 0){
        LOGE("avcodec_open2 error: %s", av_err2str(avcodecOpenRes));
        callPlayerJniError(threadMode,avcodecOpenRes, av_err2str(avcodecOpenRes));
        return;
    }
    LOGE("采样率：%d, 声道：%d", pCodecParameters->sample_rate,pCodecParameters->channels);
    duration = pFormatContext->duration;
    timeBase = pFormatContext->streams[streamIndex]->time_base;
}

void DZMedia::release() {
    if (pPacketQueue){
        delete pPacketQueue;
        pPacketQueue = NULL;
    }

    if (pCodecContext){
        avcodec_close(pCodecContext);
        avcodec_free_context(&pCodecContext);
        pCodecContext = NULL;
    }
}

void DZMedia::callPlayerJniError(ThreadMode threadMode,int code, char *msg) {
    // 释放资源
    release();

    // 回调给 java层
    pJinCall->callPlayerError(threadMode,code, msg);
}
