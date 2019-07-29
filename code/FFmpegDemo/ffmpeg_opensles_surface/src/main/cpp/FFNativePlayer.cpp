//
// Created by tangcaigao on 2019/7/19.
//

#include "util/FFThreadPool.h"
#include "FFNativePlayer.h"

#define FF_THREAD_POOL_CORE 8
#define FF_THREAD_POOL_QUEUE 64

static bool ff_inited = false;
FFMpegVideo *video;
FFMpegAudio *audio;

NativePlayerContext playerCtx;

void FFNativePlayer::ff_register() {
    //we can omit this function call in ffmpeg 4.0 and later.
    //av_register_all();
    //avcodec_register_all();
    avformat_network_init();

//    av_log_set_callback(ffmpeg_log);
}

void FFNativePlayer::ff_uninit() {
    avformat_network_deinit();
}

/**
 * 负责ff以及其他消息中心的注册操作
 */
void FFNativePlayer::ff_init(JNIEnv *env) {
    if (ff_inited) {
        return;
    }

    ff_register();
    ff_inited = true;
}

/**
 * 负责主要变量的初始化操作
 */
void FFNativePlayer::ff_prepare() {

    video = new FFMpegVideo();
    audio = new FFMpegAudio();

    playerCtx = NativePlayerContext();
    playerCtx.formatCtx = avformat_alloc_context();
    playerCtx.debug = true;
    playerCtx.play_state = PLAYER_STATE_PLAY;
    //分配线程池
    playerCtx.threadPoolCtx = ff_threadpool_create(FF_THREAD_POOL_CORE, FF_THREAD_POOL_QUEUE, 0);
    video->create(&playerCtx);
    audio->create(&playerCtx);
}

/**
 * 准备数据流 -------- 解码
 */
jlong FFNativePlayer::ff_set_data_source(JNIEnv *pEnv, const char *url) {
    char errorBuf[] = {0};
    int errorState;
    AVFormatContext *formatCtx = playerCtx.formatCtx;
    errorState = avformat_open_input(&formatCtx, url, NULL, NULL);
    if (errorState < 0) {
        av_strerror(errorState, errorBuf, 1024);
        LOGE("Couldn't open file %s: %d(%s)", url, errorState, errorBuf);
        ff_notify_msg(errorState, "FFMPEG Player Error: Can not open video file");
        return 0L;
    }
    //查看文件视频流信息
    errorState = avformat_find_stream_info(formatCtx, NULL);
    if (errorState < 0) {
        ff_notify_msg(errorState, "FFMPEG Player Error: Can not find video file stream info");
        return 0L;
    }
    jlong duration = video->decode( url);
    audio->decode(url);
    return duration;
}

void FFNativePlayer::ff_attach_window(JNIEnv *pEnv, jobject surface) {

    playerCtx.display = ANativeWindow_fromSurface(pEnv, surface);
}

static void ff_do_video_render(void *playerCtx, void *out) {
    video->render( 0);
}

static void ff_do_audio_render(void *playerCtx, void *out) {
    audio->render();
}

void FFNativePlayer::ff_start() {
    if (!playerCheck()) {
        return;
    }

    ff_threadpool_add(playerCtx.threadPoolCtx, ff_do_video_render, &playerCtx, NULL);
    ff_threadpool_add(playerCtx.threadPoolCtx, ff_do_audio_render, &playerCtx, NULL);
}

void FFNativePlayer::ff_destroy() {
    ff_uninit();
    video->release();
    audio->release();
    delete video;
    video = NULL;
    delete audio;
    audio = NULL;
}

int FFNativePlayer::ff_state() {
    return 0;
}

/**
 * 暂停
 * 返回暂停时的播放位置
 */
jlong FFNativePlayer::ff_pause() {
    return 0;
}

/**
 * 停止
 * 返回停止时的播放位置
 */
jlong FFNativePlayer::ff_stop() {
    return 0;
}

/**
 * 拖动到指定位置
 * 返回拖动完成后的播放位置
 */
jlong FFNativePlayer::ff_seek_to(jlong targetPos) {
    return 0;
}

void FFNativePlayer::ff_rest(JNIEnv *pEnv) {

}

jlong FFNativePlayer::ff_get_current_pos(JNIEnv *pEnv) {
    return 0;
}

bool FFNativePlayer::playerCheck() {
    if (!ff_inited) {
        return false;
    }

    if (playerCtx.display == 0) {
        ff_notify_msg(1, "attach view.");
        return false;
    }
    return true;
}

