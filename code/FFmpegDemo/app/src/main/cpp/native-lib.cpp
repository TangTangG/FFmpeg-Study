#include <jni.h>
#include <string>
#include <android/log.h>

#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR, "player", FORMAT, ##__VA_ARGS__);

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include <android/native_window_jni.h>
#include <unistd.h>
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_player_ffmpegdemo_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_player_ffmpegdemo_FFPlayer_doFFplay(JNIEnv *env, jobject instance, jstring url_) {
    int result;

    const char *url = env->GetStringUTFChars(url_, 0);

    //we can omit this function call in ffmpeg 4.0 and later.
    av_register_all();

    //初始化 码流参数 上下文
    AVFormatContext *formatContext = avformat_alloc_context();
    //读取视频文件
    result = avformat_open_input(&formatContext, url, NULL, NULL);
    if (result < 0) {
        LOGE("FFMPEG Player Error: Can not open video file")
        return 0;
    }

    //查看文件视频流信息
    result = avformat_find_stream_info(formatContext, NULL);
    if (result < 0) {
        LOGE("FFMPEG Player Error: Can not find video file stream info")
        return 0;
    }
    //查找视频流对应解码器
    int video_stream_index = -1;
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }

    if (video_stream_index == -1) {
        LOGE("FFMPEG Player Error: Can not find video codec")
        return 0;
    }
    //初始化视频流编码器上下文
    AVCodecContext *pCodecContext = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(pCodecContext,formatContext->streams[video_stream_index]->codecpar);

    env->ReleaseStringUTFChars(url_, url);
    return 0;
}