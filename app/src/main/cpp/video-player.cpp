#include <jni.h>
#include <android/native_window_jni.h>
#include "DZConstDefine.h"


extern "C" {
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}


//
// Created by swan on 2023/10/12.
//

extern "C"
JNIEXPORT void JNICALL
Java_com_swan_fmusicplayer_MainActivity_decodeVideo(JNIEnv *env, jobject thiz, jstring uri_,
                                                    jobject surface) {
    const char *url = env->GetStringUTFChars(uri_, 0);
    // 解码视频 和解码音频类似，解码的流程类似
    // 初始化所有组件，只有调用了该函数，才能使用复用器和编解码器
    av_register_all();
    // 初始化网络
    avformat_network_init();

    AVFormatContext *pFormatContext = NULL;
    int formatOpenInputRes = 0; // 0 成功，非0 不成功
    int formatFindStreamInfoRes = 0; // >= 0 成功
    int audioStreamIndex = -1;
    AVCodecParameters *pCodecParameters;
    AVCodec *pCodec = NULL;
    AVCodecContext *pCodecContext = NULL;
    int codecParametersToContextRes = -1;
    int avcodecOpenRes = -1;
    int index = 0;
    AVPacket *pPacket = NULL;
    AVFrame *pFrame = NULL;

    // 函数会读文件头，对 mp4 文件而言，它会解析所有的 box。但它知识把读到的结果保存在对应的数据结构下
    formatOpenInputRes = avformat_open_input(&pFormatContext, url, NULL, NULL);
    if (formatOpenInputRes != 0){
        // 1. 需要回调给Java层
        // 2. 需要释放资源
        //return;
        LOGE("format open input error: %s", av_err2str(formatOpenInputRes));
//        goto __av_resources_destroy;
    }

    // 读取一部分视音频数据并且获得一些相关的信息，会检测一些重要字段，如果是空白的，就设法填充它们
    formatFindStreamInfoRes = avformat_find_stream_info(pFormatContext, NULL);
    if (formatFindStreamInfoRes < 0){
        LOGE("format find stream info error: %s", av_err2str(formatFindStreamInfoRes));
        // 这种方式一般不推荐这么写，但的确是方便
//        goto __av_resources_destroy;
    }

    // 获取音视频及字幕的 stream_index , 以前没有这个函数时，我们一般都是写的 for 循环。
    audioStreamIndex = av_find_best_stream(pFormatContext, AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (audioStreamIndex < 0){
        LOGE("find_best_stream error: %s", av_err2str(audioStreamIndex));
        // 这种方式一般不推荐这么写，但的确是方便
//        goto __av_resources_destroy;
    }
    // 查找解码器
    pCodecParameters = pFormatContext->streams[audioStreamIndex]->codecpar;
    pCodec = avcodec_find_decoder(pCodecParameters->codec_id);
    if (pCodec == NULL){
        LOGE("avcodec_find_decoder error");
        // 这种方式一般不推荐这么写，但的确是方便
//        goto __av_resources_destroy;
    }

    // 打开解码器
    pCodecContext = avcodec_alloc_context3(pCodec);
    // 将 参数 拷到 context
    if (pCodecContext == NULL){
        LOGE("avcodec_alloc_context3 error");
//        goto __av_resources_destroy;
    }
    codecParametersToContextRes = avcodec_parameters_to_context(pCodecContext, pCodecParameters);
    if (codecParametersToContextRes < 0){
        LOGE("avcodec_parameters_to_context error: %s", av_err2str(codecParametersToContextRes));
//        goto __av_resources_destroy;
    }

    avcodecOpenRes = avcodec_open2(pCodecContext, pCodec, NULL);
    if (avcodecOpenRes != 0){
        LOGE("avcodec_open2 error: %s", av_err2str(avcodecOpenRes));
//        goto __av_resources_destroy;
    }

    // 1 获取窗体
    ANativeWindow *pNativeWindow = ANativeWindow_fromSurface(env, surface);
    // 2 设置缓冲区数据
    ANativeWindow_setBuffersGeometry(pNativeWindow, pCodecContext->width, pCodecContext->height, WINDOW_FORMAT_RGBA_8888);
    // window 缓冲区的 Buffer
    ANativeWindow_Buffer outBuffer;
    // 3 初始化转换上下文
    SwsContext* pSwsContext = sws_getContext(pCodecContext->width, pCodecContext->height,pCodecContext->pix_fmt,
                   pCodecContext->width,pCodecContext->height,AV_PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL, NULL);
    AVFrame *pRgbaFrame = av_frame_alloc();
    int frameSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA,pCodecContext->width,pCodecContext->height, 1);
    uint8_t *frameBuffer = static_cast<uint8_t *>(malloc(frameSize));
    av_image_fill_arrays(pRgbaFrame->data, pRgbaFrame->linesize,frameBuffer, AV_PIX_FMT_RGBA, pCodecContext->width,pCodecContext->height, WINDOW_FORMAT_RGBA_8888);

    // 读取每一帧
    pPacket = av_packet_alloc();
    pFrame = av_frame_alloc();
    while (av_read_frame(pFormatContext, pPacket) >= 0){
        if (pPacket->stream_index == audioStreamIndex){ // 处理音频
            // packet 包，是压缩数据，解码成 pcm 数据
            int codecSendPacketRes = avcodec_send_packet(pCodecContext, pPacket);
            if (codecSendPacketRes == 0){
                // 获取每一帧数据
                int codecReceiveFrameRes = avcodec_receive_frame(pCodecContext, pFrame);
                if (codecReceiveFrameRes == 0){
                    // AVPacket -> AVFrame ,没解码的 -> 解码好的
                    index++;
                    LOGE("解码第 %d 帧", index);
                    // 渲染，显示 OpenGLES（高效，硬件支持），SurfaceView
                    // 硬件加速和不加速有什么区别？cpu 主要是用于计算，gpu 图形支持（依赖于硬件）

                    // 这个 pFrame -> data , 一般 yuv420p的，我们一般需要的是 RGBA8888，因此需要转换
                    // 假设拿到了 转换后的 RGBA 的 data 数据，如何渲染，把数据推到缓冲区
                    sws_scale(pSwsContext,(const uint8_t *const *)pFrame->data,pFrame->linesize,0, pCodecContext->height,
                              pRgbaFrame->data, pRgbaFrame->linesize);
                    // 把数据推到缓冲区
                    ANativeWindow_lock(pNativeWindow, &outBuffer, NULL);
                    memcpy(outBuffer.bits, frameBuffer,frameSize);
                    ANativeWindow_unlockAndPost(pNativeWindow);
                }
            }
        }
        // 解引用
        av_packet_unref(pPacket);
        av_frame_unref(pFrame);
    }
    // 1. 解引用数据 data 2. 销毁 pPacket 结构体内存 3. pPacket = NULL
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);

    // 释放网络初始化
    __av_resources_destroy:
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
    avformat_network_deinit();
    env->ReleaseStringUTFChars(uri_, url);
}
