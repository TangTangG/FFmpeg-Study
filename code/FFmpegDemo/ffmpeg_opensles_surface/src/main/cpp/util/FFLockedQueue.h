//
// Created by CaiGao on 2019/7/27.
//

#ifndef FFMPEGDEMO_FFLOCKQUEUE_H
#define FFMPEGDEMO_FFLOCKQUEUE_H

#include <libavutil/mem.h>

#define QUEUE_STATE_RUNNING 1
#define QUEUE_STATE_STOP 0
extern "C++" {
#include<vector>
#include <pthread.h>

template<class T>
class FFLockedQueue {

public:
    pthread_mutex_t queue_mutex;//队列同步锁
    pthread_cond_t queue_cond;//锁条件变量
    std::vector<T *> queue;//队列
    int state;

    void init();

    T *pop();

    T *releaseHead();

    void push(T *t);

    void stop();

    void start();

    void clear();

    void free();

    int size();
};

template<class T>
void FFLockedQueue<T>::init() {
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_cond, NULL);
    start();
}

template<class T>
T *FFLockedQueue<T>::pop() {
    if (state == 0) {
        return NULL;
    }
    T *re = NULL;
    pthread_mutex_lock(&queue_mutex);
    if (!queue.empty()) {
        re = queue.front();
    }
    pthread_mutex_unlock(&queue_mutex);
    return re;
}

template<class T>
void FFLockedQueue<T>::push(T *t) {
    pthread_mutex_lock(&queue_mutex);
    queue.push_back(t);
    //添加队列成功之后，发出信号
    //此时可能pop正在等待
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
}

template<class T>
T *FFLockedQueue<T>::releaseHead() {
    if (queue.empty()) {
        return NULL;
    }
    pthread_mutex_lock(&queue_mutex);
    T *re = queue.front();
    queue.erase(queue.begin());
    pthread_mutex_unlock(&queue_mutex);
    return re;
}

template<class T>
void FFLockedQueue<T>::clear() {
    queue.clear();
}

template<class T>
void FFLockedQueue<T>::free() {
    pthread_cond_destroy(&queue_cond);
    pthread_mutex_destroy(&queue_mutex);
    queue.clear();
//    delete queue;
//    queue = NULL;
}

/**
 * 不允许从队列获取元素
 */
template<class T>
void FFLockedQueue<T>::stop() {
    state = QUEUE_STATE_STOP;
}

/**
 * 允许从队列获取元素
 */
template<class T>
void FFLockedQueue<T>::start() {
    state = QUEUE_STATE_RUNNING;
}

template<class T>
int FFLockedQueue<T>::size() {
    return queue.size();
}
}

#endif //FFMPEGDEMO_FFLOCKQUEUE_H
