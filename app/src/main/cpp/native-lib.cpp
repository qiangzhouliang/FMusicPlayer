#include <jni.h>
#include <string>
#include <android/native_window_jni.h>
#include <unistd.h>

#include "util/DZConstDefine.h"

// 因为FFmpeg是用c语言编写的，我们需要用c的编译器，所有用extern "C" 方式导入
extern "C" {
//    解码
#include <libavcodec/avcodec.h>
//    封装格式
#include "libavformat/avformat.h"
//缩放
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
//重采样
#include "libswresample/swresample.h"
}

jobject initCreateAudioTrack(JNIEnv *env) {
    jclass jAudioTrackClass = env->FindClass("android/media/AudioTrack");
    jmethodID jAudioTrackCMid = env->GetMethodID(jAudioTrackClass, "<init>", "(IIIIII)V");

    //  public static final int STREAM_MUSIC = 3;
    int streamType = 3;
    int sampleRateInHz = AUDIO_SAMPLE_RATE;
    // public static final int CHANNEL_OUT_STEREO = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT);
    int channelConfig = (0x4 | 0x8); // 立体声
    // public static final int ENCODING_PCM_16BIT = 2;
    int audioFormat = 2;
    // getMinBufferSize(int sampleRateInHz, int channelConfig, int audioFormat)
    jmethodID jGetMinBufferSizeMid = env->GetStaticMethodID(jAudioTrackClass, "getMinBufferSize", "(III)I");
    int bufferSizeInBytes = env->CallStaticIntMethod(jAudioTrackClass, jGetMinBufferSizeMid, sampleRateInHz, channelConfig, audioFormat);
    // public static final int MODE_STREAM = 1;
    int mode = 1;
    jobject jAudioTrack = env->NewObject(jAudioTrackClass, jAudioTrackCMid, streamType, sampleRateInHz, channelConfig, audioFormat, bufferSizeInBytes, mode);

    // play()
    jmethodID jPlayMid = env->GetMethodID(jAudioTrackClass, "play", "()V");
    env->CallVoidMethod(jAudioTrack, jPlayMid);

    return jAudioTrack;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_swan_media_SwanPlayer_stringFromJNI(JNIEnv* env,jobject /* this */) {
    return env->NewStringUTF(av_version_info()); // av_version_info 获取FFmpeg版本
}
extern "C"
JNIEXPORT void JNICALL
Java_com_swan_media_SwanPlayer_nPlay(JNIEnv *env, jobject thiz, jstring url_) {
    const char *url = env->GetStringUTFChars(url_, 0);
    // 讲的理念
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
    jobject jAudioTrackObj = NULL;
    jclass jAudioTrackClass = NULL;
    jmethodID jWriteMid = NULL;

    // 函数会读文件头，对 mp4 文件而言，它会解析所有的 box。但它知识把读到的结果保存在对应的数据结构下
    formatOpenInputRes = avformat_open_input(&pFormatContext, url, NULL, NULL);
    if (formatOpenInputRes != 0){
        // 1. 需要回调给Java层
        // 2. 需要释放资源
        //return;
        LOGE("format open input error: %s", av_err2str(formatOpenInputRes));
        goto __av_resources_destroy;
    }

    // 读取一部分视音频数据并且获得一些相关的信息，会检测一些重要字段，如果是空白的，就设法填充它们
    formatFindStreamInfoRes = avformat_find_stream_info(pFormatContext, NULL);
    if (formatFindStreamInfoRes < 0){
        LOGE("format find stream info error: %s", av_err2str(formatFindStreamInfoRes));
        // 这种方式一般不推荐这么写，但的确是方便
        goto __av_resources_destroy;
    }

    // 获取音视频及字幕的 stream_index , 以前没有这个函数时，我们一般都是写的 for 循环。
    audioStreamIndex = av_find_best_stream(pFormatContext, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (audioStreamIndex < 0){
        LOGE("find_best_stream error: %s", av_err2str(audioStreamIndex));
        // 这种方式一般不推荐这么写，但的确是方便
        goto __av_resources_destroy;
    }
    // 查找解码器
    pCodecParameters = pFormatContext->streams[audioStreamIndex]->codecpar;
    pCodec = avcodec_find_decoder(pCodecParameters->codec_id);
    if (pCodec == NULL){
        LOGE("avcodec_find_decoder error");
        // 这种方式一般不推荐这么写，但的确是方便
        goto __av_resources_destroy;
    }

    // 打开解码器
    pCodecContext = avcodec_alloc_context3(pCodec);
    // 将 参数 拷到 context
    if (pCodecContext == NULL){
        LOGE("avcodec_alloc_context3 error");
        goto __av_resources_destroy;
    }
    codecParametersToContextRes = avcodec_parameters_to_context(pCodecContext, pCodecParameters);
    if (codecParametersToContextRes < 0){
        LOGE("avcodec_parameters_to_context error: %s", av_err2str(codecParametersToContextRes));
        goto __av_resources_destroy;
    }

    avcodecOpenRes = avcodec_open2(pCodecContext, pCodec, NULL);
    if (avcodecOpenRes != 0){
        LOGE("avcodec_open2 error: %s", av_err2str(avcodecOpenRes));
        goto __av_resources_destroy;
    }

    LOGE("采样率：%d, 声道：%d", pCodecParameters->sample_rate,pCodecParameters->channels);

    jAudioTrackClass = env->FindClass("android/media/AudioTrack");
    jWriteMid = env->GetMethodID(jAudioTrackClass, "write", "([BII)I");

    jAudioTrackObj = initCreateAudioTrack(env);
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
                    // write 写到缓冲区 pFrame.data -> javabyte
                    // size 多大，装 pcm 数据
                    // 1s 44100 点 2通道，2字节 => 44100 * 2 *2
                    // 1帧不是1s，pFrame->nb_sampples 点
                    int dataSize = av_samples_get_buffer_size(NULL,pFrame->channels,pFrame->nb_samples,
                                                              pCodecContext->sample_fmt,0);
                    jbyteArray jPcmByteArray = env->NewByteArray(dataSize);
                    // 将c的数据同步到java里面来, native 层创建 c 数组
                    jbyte* jPcmData = env->GetByteArrayElements(jPcmByteArray, NULL);
                    memcpy(jPcmData, pFrame->data, dataSize);

                    // 同步数据 把 c 的数组的数据 同步到 jbyteArray，然后释放 native 数组
                    env->ReleaseByteArrayElements(jPcmByteArray, jPcmData, 0);
                    // 调用 Write 方法写入数据
                    env->CallIntMethod(jAudioTrackObj, jWriteMid, jPcmByteArray, 0, dataSize);
                    // 折行代码必须要加(要不然会崩)，但是内存还是会往上涨
                    env->DeleteLocalRef(jPcmByteArray);
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
    env->DeleteLocalRef(jAudioTrackObj);

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
    env->ReleaseStringUTFChars(url_, url);
}