//
// Created by swan on 2023/10/11.
//

#include <pthread.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include "DZAudio.h"
#include "DZConstDefine.h"

DZAudio::DZAudio(int audioStreamIndex, DZJNICall *pJinCall,AVFormatContext *pFormatContext) {
    this->audioStreamIndex = audioStreamIndex;
    this->pJinCall = pJinCall;
    this->pFormatContext = pFormatContext;

    pPacketQueue = new DZPacketQueue();
    pPlayerStatus = new DZPlayerStatus();
}

// 播放线程
void* threadPlay(void* context){
    DZAudio* pAudio = static_cast<DZAudio *>(context);
    // 创建播放OpenSLES
    pAudio->initCreateOpenSLES();
    return 0;
}

// 解码放数据线程
void* threadReadPacket(void* context){
    DZAudio* pAudio = static_cast<DZAudio *>(context);
    while (pAudio->pPlayerStatus != NULL && !pAudio->pPlayerStatus->isExit){
        // 读取每一帧
        AVPacket *pPacket = av_packet_alloc();
        if (av_read_frame(pAudio->pFormatContext, pPacket) >= 0){
            if (pPacket->stream_index == pAudio->audioStreamIndex){ // 处理音频
                pAudio->pPacketQueue->push(pPacket);
            } else {
                // 1. 解引用数据 data 2. 销毁 pPacket 结构体内存 3. pPacket = NULL
                av_packet_free(&pPacket);
            }
        } else {
            // 1. 解引用数据 data 2. 销毁 pPacket 结构体内存 3. pPacket = NULL
            av_packet_free(&pPacket);
            // 睡眠一下，尽量不去消耗 cpu 的资源,也可以退出销毁这个线程
            //break;
        }
    }


    return 0;
}

void DZAudio::play() {
    // 一个线程去读取 packet
    pthread_t decodePacketThreadT;
    pthread_create(&decodePacketThreadT, NULL, threadReadPacket, this);
    pthread_detach(decodePacketThreadT);

    // 创建一个线程去播放，多线程编解码边播放
    // 一个线程去解码播放
    pthread_t playThreadT;
    pthread_create(&playThreadT, NULL, threadPlay, this);
    pthread_detach(playThreadT);
}

// this callback handler is called every time a buffer finishes playing
void playerCallback(SLAndroidSimpleBufferQueueItf caller, void *context){
    // 回调函数，循环
    DZAudio *pAudio = static_cast<DZAudio *>(context);
    int dataSize = pAudio->resampleAudio();
    (*caller)->Enqueue(caller, pAudio->resampleOutBuffer, dataSize);
}

void DZAudio::initCreateOpenSLES() {
    //1.创建引擎接口对象
    // engine interfaces
    SLObjectItf engineObject = NULL;
    SLEngineItf engineEngine;
    // create engine
    slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);

    // realize the engine
    (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);

    // get the engine interface, which is needed in order to create other objects
    (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    //2.设置混音器
    // output mix interfaces
    SLObjectItf outputMixObject = NULL;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;
    SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
    SLboolean req[1] = {SL_BOOLEAN_FALSE};
    (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
    (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                     &outputMixEnvironmentalReverb);
    SLEnvironmentalReverbSettings reverbSettings =SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;
    (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
        outputMixEnvironmentalReverb, &reverbSettings);
    //3.创建播放器
    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue simpleBufferQueue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM formatPcm = {SL_DATAFORMAT_PCM,
                                  2,
                                  SL_SAMPLINGRATE_44_1,
                                  SL_PCMSAMPLEFORMAT_FIXED_16,
                                  SL_PCMSAMPLEFORMAT_FIXED_16,
                                  SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT,
                                  SL_BYTEORDER_LITTLEENDIAN};
    SLObjectItf pPlayer = NULL;
    static SLPlayItf bqPlayerPlay;
    SLmilliHertz bqPlayerSampleRate = 0;
    SLDataSource audioSrc = {&simpleBufferQueue, &formatPcm};

    // configure audio sink
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&outputMix, NULL};
    const SLInterfaceID interfaceIds[3] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_PLAYBACKRATE};
    const SLboolean interfaceReq[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    (*engineEngine)->CreateAudioPlayer(engineEngine, &pPlayer, &audioSrc, &audioSnk,
                                       bqPlayerSampleRate? 2 : 3, interfaceIds, interfaceReq);

    // realize the player
    (*pPlayer)->Realize(pPlayer, SL_BOOLEAN_FALSE);

    // get the play interface
    (*pPlayer)->GetInterface(pPlayer, SL_IID_PLAY, &bqPlayerPlay);
    //4.设置缓存队列和回调函数
    static SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
    (*pPlayer)->GetInterface(pPlayer, SL_IID_BUFFERQUEUE,
                             &bqPlayerBufferQueue);
    // 每次回调 this 会被带给 playerCallback 的 context
    (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, playerCallback, this);

    //5.设置播放状态
    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    //6.调用回调函数
    playerCallback(bqPlayerBufferQueue, this);
}

int DZAudio::resampleAudio(){
    int dataSize = 0;
    // 读取每一帧
    AVPacket *pPacket = NULL;
    AVFrame *pFrame = av_frame_alloc();
    while (pPlayerStatus != NULL && !pPlayerStatus->isExit){
        pPacket = pPacketQueue->pop();
        // packet 包，是压缩数据，解码成 pcm 数据
        int codecSendPacketRes = avcodec_send_packet(pCodecContext, pPacket);
        if (codecSendPacketRes == 0){
            // 获取每一帧数据
            int codecReceiveFrameRes = avcodec_receive_frame(pCodecContext, pFrame);
            if (codecReceiveFrameRes == 0){
                // AVPacket -> AVFrame ,没解码的 -> 解码好的
                LOGE("解码第 音频 帧");
                // 调用重采样的方法,对音频数据重新进行采样，返回值是返回重采样的个数，也就是 pFrame->nb_samples
                dataSize = swr_convert(swrContext, &resampleOutBuffer, pFrame->nb_samples,
                                       (const uint8_t **)pFrame->data, pFrame->nb_samples);
                // write 写到缓冲区 pFrame.data -> javabyte
                // size 多大，装 pcm 数据
                // 立体声 * 2，16位 * 2
                dataSize = dataSize * 2 * 2;
                break;
            }
        }
        // 解引用
        av_packet_unref(pPacket);
        av_frame_unref(pFrame);
    }
    // 1. 解引用数据 data 2. 销毁 pPacket 结构体内存 3. pPacket = NULL
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);
    return dataSize;
}

DZAudio::~DZAudio() {
    release();
}
/**
 * 分析视频流
 * @param threadMode
 * @param streams
 */
void DZAudio::analysisStream(ThreadMode threadMode, AVStream **streams) {
    // 查找解码器
    AVCodecParameters *pCodecParameters = streams[audioStreamIndex]->codecpar;
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
    // -----------重采样---srart------------------------
    // 设置重采样的参数
    int64_t out_ch_layout = AV_CH_LAYOUT_STEREO;   // 输出通道
    enum AVSampleFormat out_sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_S16; // 输出格式
    int out_sample_rate = AUDIO_SAMPLE_RATE;
    int64_t in_ch_layout = pCodecContext->channels;
    enum AVSampleFormat in_sample_fmt = pCodecContext->sample_fmt;
    int in_sample_rate = pCodecContext->sample_rate;
    swrContext = swr_alloc_set_opts(NULL, out_ch_layout, out_sample_fmt, out_sample_rate,
                                    in_ch_layout,  in_sample_fmt, in_sample_rate,0, NULL);
    if (swrContext == NULL){
        // 提示错误
        callPlayerJniError(threadMode,SWR_ALLOC_SET_OPTS_ERROR_CODE, "swr_alloc_set_opts error");
        return;
    }
    int swrInitRes = swr_init(swrContext);
    if (swrInitRes < 0){
        callPlayerJniError(threadMode,SWR_INIT_ERROR_CODE, "swr_init error");
        return;
    }
    resampleOutBuffer = static_cast<uint8_t *>(malloc(pCodecContext->frame_size * 2 * 2));
    // -----------重采样---end------------------------
}

void DZAudio::callPlayerJniError(ThreadMode threadMode,int code, char *msg) {
    // 释放资源
    release();

    // 回调给 java层
    pJinCall->callPlayerError(threadMode,code, msg);
}

void DZAudio::release() {
    if (pPacketQueue){
        delete pPacketQueue;
        pPacketQueue = NULL;
    }

    if (resampleOutBuffer){
        free(resampleOutBuffer);
        resampleOutBuffer = NULL;
    }

    if (pPlayerStatus){
        free(pPlayerStatus);
        pPlayerStatus = NULL;
    }

    if (pCodecContext != NULL){
        avcodec_close(pCodecContext);
        avcodec_free_context(&pCodecContext);
        pCodecContext == NULL;
    }

    if (pFormatContext != NULL){
        avformat_close_input(&pFormatContext);
        avformat_free_context(pFormatContext);
        pFormatContext == NULL;
    }

    if (swrContext != NULL){
        swr_free(&swrContext);
        free(swrContext);
        swrContext = NULL;
    }

}
