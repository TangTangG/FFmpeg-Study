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
#include "FFMpegAudio.h"
#include "FFMpegVideo.h"
}

class FFNativePlayer {

public:
    void ff_init(jobject surface);
    void ff_register();
    void ff_prepare();
    int ff_state();
    void ff_destroy();

    static bool ff_inited = false;
};


#endif //FFMPEGDEMO_FFNATIVEPLAYER_H
