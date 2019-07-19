//
// Created by tangcaigao on 2019/7/19.
//

#ifndef FFMPEGDEMO_FFNATIVEPLAYER_H
#define FFMPEGDEMO_FFNATIVEPLAYER_H

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include <libavutil/log.h>

#include <android/native_window_jni.h>
#include <unistd.h>
#include "util/FF_Log.h"
}

class FFNativePlayer {

public:
    void ff_register();
};


#endif //FFMPEGDEMO_FFNATIVEPLAYER_H
