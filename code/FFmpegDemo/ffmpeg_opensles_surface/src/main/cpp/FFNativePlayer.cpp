//
// Created by tangcaigao on 2019/7/19.
//

#include "FFNativePlayer.h"

static AVPacket flush_pkt;

void FFNativePlayer::ff_init(jobject surface) {

}

void FFNativePlayer::ff_register() {
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
}

int FFNativePlayer::ff_state() {
    return 0;
}
