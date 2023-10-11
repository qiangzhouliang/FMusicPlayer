//
// Created by swan on 2023/10/11.
//

#ifndef FMUSICPLAYER_DZPACKETQUEUE_H
#define FMUSICPLAYER_DZPACKETQUEUE_H
#include <queue>
#include <pthread.h>

extern "C" {
#include "libavcodec/avcodec.h"
}


class DZPacketQueue {
public:
    std::queue<AVPacket *> *pPacketQueue;
    pthread_mutex_t packetMutex;
    pthread_cond_t packetCond;

public:
    DZPacketQueue();
    ~DZPacketQueue();

public:
    void push(AVPacket *pPacket);

    AVPacket* pop();

    /**
     * 清理整个队列
     */
    void clear();

};


#endif //FMUSICPLAYER_DZPACKETQUEUE_H
