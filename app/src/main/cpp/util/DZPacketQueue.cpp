//
// Created by swan on 2023/10/11.
//

#include "DZPacketQueue.h"
DZPacketQueue::DZPacketQueue() {
    pPacketQueue = new std::queue<AVPacket *>();

    pthread_mutex_init(&packetMutex, NULL);
    pthread_cond_init(&packetCond, NULL);
}

void DZPacketQueue::clear() {
    // 需要清楚队列，还需要清楚每个 AVPacket* 的内存数据

}

DZPacketQueue::~DZPacketQueue() {
    if (pPacketQueue != NULL){
        clear();
        delete pPacketQueue;
        pPacketQueue = NULL;
    }

    pthread_mutex_destroy(&packetMutex);
    pthread_cond_destroy(&packetCond);
}

void DZPacketQueue::push(AVPacket *pPacket) {
    pthread_mutex_lock(&packetMutex);
    pPacketQueue->push(pPacket);
    // 通知消费者消费
    pthread_cond_signal(&packetCond);
    pthread_mutex_unlock(&packetMutex);
}

AVPacket *DZPacketQueue::pop() {
    AVPacket *pPacket;
    pthread_mutex_lock(&packetMutex);
    while (pPacketQueue->empty()){
        pthread_cond_wait(&packetCond, &packetMutex);
    }
    pPacket = pPacketQueue->front();
    pPacketQueue->pop();

    pthread_mutex_unlock(&packetMutex);
    return pPacket;
}
