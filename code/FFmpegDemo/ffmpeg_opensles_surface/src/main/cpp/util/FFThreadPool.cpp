//
// Created by tangcaigao on 2019/7/18.
//

#include "FFThreadPool.h"
#include "libavutil/log.h"

#include <stdlib.h>
#include <unistd.h>

/**
 * 创建并执行任务
 */
static void *ff_threadpool_thread(void *pool_ctx) {
    FFThreadPoolContext *ctx = static_cast<FFThreadPoolContext *>(pool_ctx);
    FFThreadPoolTask task;

    for (;;) {
        pthread_mutex_lock(&(ctx->lock));

        while ((ctx->pending_count == 0 && (!ctx->shutdown))) {
            pthread_cond_wait(&(ctx->notify), &(ctx->lock));
        }

        if ((ctx->shutdown == FF_IMMEDIATE_SHUTDOWN) ||
            ((ctx->shutdown == FF_LEISURELY_SHUTDOWN) && (ctx->pending_count == 0))) {
            break;
        }
        //创建任务
        task.function = ctx->queue[ctx->queue_head].function;
        task.in_arg = ctx->queue[ctx->queue_head].in_arg;
        task.out_arg = ctx->queue[ctx->queue_head].out_arg;

        ctx->queue_head = (ctx->queue_head + 1) % ctx->queue_size;
        ctx->pending_count -= 1;

        pthread_mutex_unlock(&(ctx->lock));
        //执行Runable
        (*(task.function))(task.in_arg, task.out_arg);
    }
    ctx->started_count--;

    pthread_mutex_unlock(&(ctx->lock));
    pthread_exit(NULL);
    return (NULL);
}

int ff_threadpool_free(FFThreadPoolContext *ctx) {
    if (ctx == NULL || ctx->started_count > 0) {
        return -1;
    }
    //回收
    if (ctx->threads) {
        free(ctx->threads);
        free(ctx->queue);

        pthread_mutex_lock(&(ctx->lock));
        pthread_mutex_destroy(&(ctx->lock));
        pthread_cond_destroy(&(ctx->notify));
    }
    free(ctx);
    return 0;
}

FFThreadPoolContext *ff_threadpool_create(int thread_count, int queue_size, int flags) {

    if (thread_count <= 0 || thread_count > MAX_THREADS
        || queue_size <= 0 || queue_size > MAX_QUEUE) {
        return NULL;
    }
    FFThreadPoolContext *ctx;
    int i;
    if ((ctx = static_cast<FFThreadPoolContext *>(calloc(1, sizeof(FFThreadPoolContext)))) ==
        NULL) {
        goto err;
    }

    ctx->queue_size = queue_size;
    ctx->threads = static_cast<pthread_t *>(calloc(1, sizeof(pthread_t) * thread_count));
    ctx->queue = static_cast<FFThreadPoolTask *>(calloc(queue_size, sizeof(FFThreadPoolTask)));

    // Initialize mutex and conditional variable first
    // 初始化并检查互斥锁以及条件锁
    if ((ctx->threads) == NULL ||
        (ctx->queue) == NULL ||
        (pthread_cond_init(&(ctx->notify), NULL) != 0) ||
        (pthread_mutex_init(&(ctx->lock), NULL))) {
        goto err;
    }
    //创建并启动等量的核心线程
    for (i = 0; i < thread_count; ++i) {
        if (pthread_create(&(ctx->threads[i]), NULL, ff_threadpool_thread, (void *) ctx) != 0) {
            ff_threadpool_destory(ctx, 0);
            return NULL;
        }
        ctx->thread_count++;
        ctx->started_count++;
    }
    return ctx;

    err:
    if (ctx) {
        ff_threadpool_free(ctx);
    }
    return NULL;
}

/**
 * 1.异常处理
 * 2.容量检测->扩容
 * 3.添加
 */
int ff_threadpool_add(FFThreadPoolContext *ctx, Runable fun, void *in_arg, void *out_arg) {

    int err = 0;
    int next_tail;

    if (ctx == NULL || fun == NULL) {
        return FF_THREADPOLL_INVALID;
    }
    if (pthread_mutex_lock(&(ctx->lock)) != 0) {
        return FF_THREADPOLL_LOCK_FAILURE;
    }
    if (ctx->pending_count == MAX_QUEUE || ctx->pending_count == ctx->queue_size) {
        pthread_mutex_unlock(&(ctx->lock));
        return FF_THREADPOLL_QUEUE_FULL;
    }

    //队列需要扩容
    if (ctx->pending_count == ctx->queue_size - 1) {
        //double
        int new_size = (ctx->queue_size * 2) > MAX_QUEUE ? MAX_QUEUE : (ctx->queue_size * 2);

        FFThreadPoolTask *new_queue = static_cast<FFThreadPoolTask *>(realloc(ctx->queue,
                                                                              sizeof(FFThreadPoolTask) *
                                                                              new_size));
        if (new_queue) {
            ctx->queue = new_queue;
            ctx->queue_size = new_size;
        }
    }

    next_tail = (ctx->queue_tail + 1) % ctx->queue_size;

    do {
        if (ctx->shutdown) {
            err = FF_THREADPOLL_SHUTDOWN;
            break;
        }
        //添加任务到挂起队列中
        ctx->queue[ctx->queue_tail].function = fun;
        ctx->queue[ctx->queue_tail].in_arg = in_arg;
        ctx->queue[ctx->queue_tail].out_arg = out_arg;
        ctx->queue_tail = next_tail;
        ctx->pending_count += 1;

        /* pthread_cond_broadcast */
        //类似于java的notify all
        if (pthread_cond_signal(&(ctx->notify))) {
            err = FF_THREADPOLL_LOCK_FAILURE;
        }
    } while (0);
    if (pthread_mutex_unlock(&(ctx->lock)) != 0) {
        err = FF_THREADPOLL_LOCK_FAILURE;
    }
    return err;
}

int ff_threadpool_freep(FFThreadPoolContext **ctx) {
    int ret = 0;

    if (!ctx || !*ctx)
        return -1;

    ret = ff_threadpool_free(*ctx);
    *ctx = NULL;
    return ret;
}

int ff_threadpool_destory(FFThreadPoolContext *ctx, int flags) {
    int i, err = 0;

    if (ctx == NULL) {
        return FF_THREADPOLL_INVALID;
    }

    if (pthread_mutex_unlock(&(ctx->lock)) != 0) {
        return FF_THREADPOLL_LOCK_FAILURE;
    }

    do {
        if (ctx->shutdown) {
            err = FF_THREADPOLL_SHUTDOWN;
            break;
        }

        ctx->shutdown = flags;
        if ((pthread_cond_broadcast(&(ctx->notify)) != 0) ||
            (pthread_mutex_unlock(&(ctx->lock)) != 0)) {
            err = FF_THREADPOLL_LOCK_FAILURE;
            break;
        }
        for (int i = 0; i < ctx->thread_count; ++i) {
            if (pthread_join(ctx->threads[i], NULL) != 0) {
                err = FF_THREADPOLL_THREAD_FAILURE;
            }
        }
    } while (0);

    if (!err) {
        return ff_threadpool_freep(&ctx);
    }

    return err;
}


