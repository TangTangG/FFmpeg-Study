//
// Created by tangcaigao on 2019/7/18.
//

#ifndef FFMPEGDEMO_FFTHREADPOOL_H
#define FFMPEGDEMO_FFTHREADPOOL_H

#define MAX_THREADS 100
#define MAX_QUEUE 1024

#include <pthread.h>

typedef enum {
    FF_THREADPOLL_INVALID = -1,
    FF_THREADPOLL_LOCK_FAILURE = -2,
    FF_THREADPOLL_QUEUE_FULL = -3,
    FF_THREADPOLL_SHUTDOWN = -4,
    FF_THREADPOLL_THREAD_FAILURE = -5
} FFThreadPoolErrorType;

typedef enum {
    //立即关闭
    FF_IMMEDIATE_SHUTDOWN = 1,
    //执行完已有任务后关闭（空闲时关闭）
    FF_LEISURELY_SHUTDOWN = 2,
} FFThreadPoolShutdownType;

typedef void (*Runable)(void *, void *);

/**
 *  @var function Pointer to the function that will perform the task.
 *  @var in_arg Argument to be passed to the function.
 *  @var out_arg Argument to be passed to the call function.
 */
typedef struct FFThreadPoolTask {
    Runable function;
    void *in_arg;
    void *out_arg;
} FFThreadPoolTask;

/**
 *  @var notify        Condition variable to notify worker threads.
 *  @var threads       Array containing worker threads ID.
 *  @var thread_count  Number of threads
 *  @var queue         Array containing the task queue.
 *  @var queue_size    Size of the task queue.
 *  @var queue_head    Index of the first element.
 *  @var queue_tail    Index of the next element.
 *  @var pending_count Number of pending tasks
 *  @var shutdown      Flag indicating if the pool is shutting down
 *  @var started       Number of started threads
 */
typedef struct FFThreadPoolContext {
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
    FFThreadPoolTask *queue;
    int thread_count;
    int queue_size;
    int queue_head;
    int queue_tail;
    int pending_count;
    int shutdown;
    int started_count;
} FFThreadPoolContext;

FFThreadPoolContext *ff_threadpool_create(int thread_count, int queue_size, int flags);

int ff_threadpool_add(FFThreadPoolContext *ctx, Runable fun, void *in_arg, void *out_arg);

int ff_threadpool_destory(FFThreadPoolContext *ctx, int flags);


#endif //FFMPEGDEMO_FFTHREADPOOL_H
