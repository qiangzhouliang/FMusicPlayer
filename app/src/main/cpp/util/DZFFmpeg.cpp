//
// Created by swan on 2023/10/10.
//

#include "DZFFmpeg.h"
#include "DZConstDefine.h"

DZFFmpeg::DZFFmpeg(DZJNICall *pJinCall, const char *url) {
    this->pJinCall = pJinCall;
    this->url = url;
}

DZFFmpeg::~DZFFmpeg() {
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

    if (resampleOutBuffer != NULL){
        free(resampleOutBuffer);
        resampleOutBuffer = NULL;
    }
    avformat_network_deinit();
}

void DZFFmpeg::play() {

    // 讲的理念
    // 初始化所有组件，只有调用了该函数，才能使用复用器和编解码器
    av_register_all();
    // 初始化网络
    avformat_network_init();

    int formatOpenInputRes = 0; // 0 成功，非0 不成功
    int formatFindStreamInfoRes = 0; // >= 0 成功
    int audioStreamIndex = -1;
    AVCodecParameters *pCodecParameters;
    AVCodec *pCodec = NULL;
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
        return;
    }

    // 读取一部分视音频数据并且获得一些相关的信息，会检测一些重要字段，如果是空白的，就设法填充它们
    formatFindStreamInfoRes = avformat_find_stream_info(pFormatContext, NULL);
    if (formatFindStreamInfoRes < 0){
        LOGE("format find stream info error: %s", av_err2str(formatFindStreamInfoRes));
        // 这种方式一般不推荐这么写，但的确是方便
        //        goto __av_resources_destroy;
        return;
    }

    // 获取音视频及字幕的 stream_index , 以前没有这个函数时，我们一般都是写的 for 循环。
    audioStreamIndex = av_find_best_stream(pFormatContext, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (audioStreamIndex < 0){
        LOGE("find_best_stream error: %s", av_err2str(audioStreamIndex));
        // 这种方式一般不推荐这么写，但的确是方便
        //        goto __av_resources_destroy;
        return;
    }
    // 查找解码器
    pCodecParameters = pFormatContext->streams[audioStreamIndex]->codecpar;
    pCodec = avcodec_find_decoder(pCodecParameters->codec_id);
    if (pCodec == NULL){
        LOGE("avcodec_find_decoder error");
        // 这种方式一般不推荐这么写，但的确是方便
//        goto __av_resources_destroy;
        return;
    }

    // 打开解码器
    pCodecContext = avcodec_alloc_context3(pCodec);
    // 将 参数 拷到 context
    if (pCodecContext == NULL){
        LOGE("avcodec_alloc_context3 error");
        //        goto __av_resources_destroy;
        return;
    }
    codecParametersToContextRes = avcodec_parameters_to_context(pCodecContext, pCodecParameters);
    if (codecParametersToContextRes < 0){
        LOGE("avcodec_parameters_to_context error: %s", av_err2str(codecParametersToContextRes));
        //        goto __av_resources_destroy;
        return;
    }

    avcodecOpenRes = avcodec_open2(pCodecContext, pCodec, NULL);
    if (avcodecOpenRes != 0){
        LOGE("avcodec_open2 error: %s", av_err2str(avcodecOpenRes));
        //        goto __av_resources_destroy;
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
        return;
    }
    int swrInitRes = swr_init(swrContext);
    if (swrInitRes < 0){
        return;
    }

    // 1s 44100 点 2通道，2字节 => 44100 * 2 *2
    // 1帧不是1s，pFrame->nb_sampples 点
    // size是播放指定的大小，是最终输出的大小
    int outChannels = av_get_channel_layout_nb_channels(out_ch_layout); // 输出的通道
    int dataSize = av_samples_get_buffer_size(NULL,outChannels,
                                              pCodecContext->frame_size,out_sample_fmt,0);
    uint8_t *resampleOutBuffer = static_cast<uint8_t *>(malloc(dataSize));
    // -----------重采样---end------------------------

    jbyteArray jPcmByteArray = pJinCall->env->NewByteArray(dataSize);
    // 将c的数据同步到java里面来, native 层创建 c 数组
    jbyte* jPcmData = pJinCall->env->GetByteArrayElements(jPcmByteArray, NULL);

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

                    // 调用重采样的方法,对音频数据重新进行采样
                    swr_convert(swrContext, &resampleOutBuffer, pFrame->nb_samples,
                                (const uint8_t **)pFrame->data, pFrame->nb_samples);

                    // write 写到缓冲区 pFrame.data -> javabyte
                    // size 多大，装 pcm 数据
                    memcpy(jPcmData, resampleOutBuffer, dataSize);

                    // 同步数据 把 c 的数组的数据 同步到 jbyteArray，mode 传 0 会释放 native 数组
                    pJinCall->env->ReleaseByteArrayElements(jPcmByteArray, jPcmData, JNI_COMMIT);
                    // 调用 Write 方法写入数据
                    pJinCall->callAudioTrackWrite(jPcmByteArray, 0, dataSize);
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
    // 折行代码必须要加(要不然会崩)，但是内存还是会往上涨
    // 解除 jPcmByteArray 的持有，让 javaGC回收
    pJinCall->env->ReleaseByteArrayElements(jPcmByteArray, jPcmData, 0);
    pJinCall->env->DeleteLocalRef(jPcmByteArray);
}

void DZFFmpeg::callPlayerJniError(int code, char *msg) {

}
