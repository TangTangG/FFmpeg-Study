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

    void ff_init(JNIEnv *env);

    void ff_prepare();

    void ff_destroy();

    void ff_start();

    jlong ff_pause();

    jlong ff_stop();

    jlong ff_seek_to(jlong i);

    void ff_rest(JNIEnv *pEnv);

    jlong ff_set_data_source(JNIEnv *pEnv, const char *url);

    void ff_attach_window(JNIEnv *pEnv, jobject surface);

    jlong ff_get_current_pos(JNIEnv *pEnv);

    int ff_state();

private:



    bool playerCheck();

    void ff_notify_msg(int tag, const char *msg);

    void ff_register();

    void ff_uninit();

    void ff_release();
};


#endif //FFMPEGDEMO_FFNATIVEPLAYER_H
