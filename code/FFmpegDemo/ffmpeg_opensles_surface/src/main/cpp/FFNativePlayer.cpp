//
// Created by tangcaigao on 2019/7/19.
//

#include "FFNativePlayer.h"

static AVPacket flush_pkt;

void ff_register() {
    //we can omit this function call in ffmpeg 4.0 and later.
    //av_register_all();
    //avcodec_register_all();
    avformat_network_init();

    av_log_set_callback(ffmpeg_log);
}

void ff_uninit() {
    avformat_network_deinit();
}

void FFNativePlayer::ff_prepare() {

}

void FFNativePlayer::ff_destroy() {
    ff_uninit();
    delete video;
    video = NULL;
    delete audio;
    audio = NULL;
    delete &flush_pkt;
    flush_pkt = NULL;
}

int FFNativePlayer::ff_state() {
    return 0;
}

void FFNativePlayer::ff_init(JNIEnv *env) {
    if (ff_inited) {
        return;
    }
    video = (FFMpegVideo *)av_malloc(sizeof(FFMpegVideo));
    audio = (FFMpegAudio *)av_malloc(sizeof(FFMpegAudio));

    ff_register();

    ff_inited = true;
}

void FFNativePlayer::ff_start() {

}

jlong FFNativePlayer::ff_pause() {
    return 0;
}

jlong FFNativePlayer::ff_stop() {
    return 0;
}

jlong FFNativePlayer::ff_seek_to(jlong targetPos) {
    return 0;
}

void FFNativePlayer::ff_rest(JNIEnv *pEnv) {

}

/**
 * 准备数据流
 */
jlong FFNativePlayer::ff_set_data_source(JNIEnv *pEnv, const char *string) {
    return 0;
}

void FFNativePlayer::ff_attach_window(JNIEnv *pEnv, jobject pJobject) {

}

jlong FFNativePlayer::ff_get_current_pos(JNIEnv *pEnv) {
    return 0;
}
