#include <jni.h>
#include <string>
#include <android/log.h>

#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR, "player", FORMAT, ##__VA_ARGS__);

#define LOGD(FORMAT, ...) __android_log_print(ANDROID_LOG_DEBUG, "player", FORMAT, ##__VA_ARGS__);

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
//#include "FFMpeg_log.h"
#include <android/native_window_jni.h>
#include <unistd.h>
}


void custom_log(void *ptr, int level, const char *fmt, va_list vl) {
    FILE *fp = fopen("/storage/emulated/0/av_log.txt", "a+");
    if (fp) {
        vfprintf(fp, fmt, vl);
        fflush(fp);
        fclose(fp);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_gu_ffmpeg_1decode_FFDecodePlayer_decode(JNIEnv *env, jobject instance, jstring url_) {
    const char *url = env->GetStringUTFChars(url_, 0);

    av_log_set_callback(custom_log);

    env->ReleaseStringUTFChars(url_, url);
}