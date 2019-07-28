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

template <class T>
class FFLockedQueue {

public:
    pthread_mutex_t queue_mutex;//队列同步锁
    pthread_cond_t queue_cond;//锁条件变量
    std::vector<T *> queue;//队列
    int state;

    void init();
    T* pop(T *dst, int (*callback)(T *dst, T *src));
    void push(T *src,T *dst, int (*callback)(T *dst, T *src));
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
T* FFLockedQueue<T>::pop(T *dst, int (*callback)(T *, T *)) {
    T *re = dst;
    pthread_mutex_lock(&queue_mutex);
    while (state) {
        if (!queue.empty()) {
            //返回正数表示克隆失败，0表示成功
            re = queue.front();
            break;
            /*if (callback(dst, queue.front())) {
                break;
            }
            //取成功之后，将旧元素从队列中移除，并释放其内存
            T *&remove = queue.front();
            queue.erase(queue.begin());
            av_free(remove);*/
        } else {
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }
    }
    pthread_mutex_unlock(&queue_mutex);
    return re;
}

template<class T>
void FFLockedQueue<T>::push(T *src, T *dst, int (*callback)(T *, T *)) {
    /*if (callback(dst, src)) {
        return;
    }*/
    pthread_mutex_lock(&queue_mutex);
    queue.push_back(dst);
    //添加队列成功之后，发出信号
    //此时可能pop正在等待
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
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
