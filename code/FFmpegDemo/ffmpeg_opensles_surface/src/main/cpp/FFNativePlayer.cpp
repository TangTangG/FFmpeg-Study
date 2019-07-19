//
// Created by tangcaigao on 2019/7/19.
//

#include "FFNativePlayer.h"


void FFNativePlayer::ff_register() {
    //we can omit this function call in ffmpeg 4.0 and later.
    //av_register_all();
    //avcodec_register_all();
    avformat_network_init();

}
