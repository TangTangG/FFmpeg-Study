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
#include <libavutil/log.h>

#include <android/native_window_jni.h>
#include <unistd.h>
#include "util/FF_Log.h"
#include "FFNativePlayer.h"
}

FFNativePlayer *player = 0;

extern "C"
JNIEXPORT void JNICALL
Java_com_player_ffmpeg_1opensles_1surface_FFSurfaceOpenslESPlayer_doFFplay(JNIEnv *env,
                                                                           jobject instance,
                                                                           jobject surface,
                                                                           jstring url_) {
    int result;

    const char *url = env->GetStringUTFChars(url_, 0);
    char buf[] = {0};

    //we can omit this function call in ffmpeg 4.0 and later.
    //av_register_all();
    av_log_set_callback(ffmpeg_log);

    //初始化 码流参数 上下文
    AVFormatContext *formatContext = avformat_alloc_context();
    //读取视频文件
    result = avformat_open_input(&formatContext, url, NULL, NULL);
    if (result < 0) {
        av_strerror(result, buf, 1024);
        // LOGE("%s" ,inputPath)
        LOGE("Couldn't open file %s: %d(%s)", url, result, buf);
        LOGE("FFMPEG Player Error: Can not open video file");
        return;
    }

    //查看文件视频流信息
    result = avformat_find_stream_info(formatContext, NULL);
    if (result < 0) {
        LOGE("FFMPEG Player Error: Can not find video file stream info");
        return;
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
        LOGD("FFMPEG Player : Can not find video codec");
        return;
    }
    //初始化视频流编码器上下文
    AVCodecParameters *codecpar = formatContext->streams[video_stream_index]->codecpar;
    AVCodec *avCodec = avcodec_find_decoder(codecpar->codec_id);
    AVCodecContext *avCodecContext = avcodec_alloc_context3(avCodec);
    avcodec_parameters_to_context(avCodecContext, codecpar);
    //初始化对应流的编码器
    result = avcodec_open2(avCodecContext, avCodec, NULL);
    if (result < 0) {
        LOGE("FFMPEG Player Error: Can not open video file");
        return;
    }

    int width = avCodecContext->width;
    int height = avCodecContext->height;
    //对于视频（Video）来说，AVPacket通常包含一个压缩的Frame；而音频（Audio）则有可能包含多个压缩的Frame
    //关于这些数据的一些附加的信息，如显示时间戳（pts），解码时间戳（dts）,数据时长（duration）
    AVPacket *avPacket = (AVPacket *) av_malloc(sizeof(AVPacket));
    av_init_packet(avPacket);

    AVFrame *avFrame = av_frame_alloc();
    AVFrame *rgb_frame = av_frame_alloc();

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, width, height, 1);
    //建立缓存区
    uint8_t *out_buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    //填充rgb_frame
    av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, out_buffer,
                         AV_PIX_FMT_RGBA,
                         width, height, 1);

    SwsContext *swsContext = sws_getContext(width, height,
                                            avCodecContext->pix_fmt,
                                            width, height,
                                            AV_PIX_FMT_RGBA,
                                            SWS_BICUBIC, NULL, NULL, NULL);
    //获取surface所在window
    ANativeWindow *pNativeWindow = ANativeWindow_fromSurface(env, surface);
    if (pNativeWindow == 0) {
        LOGE("FFMPEG Player Error: ANativeWindow get failed");
        return;
    }
    if (ANativeWindow_setBuffersGeometry(pNativeWindow, width, height, WINDOW_FORMAT_RGBA_8888) <
        0) {
        LOGE("FFMPEG Player Error: Couldn't set buffers geometry.");
        return;
    }

    ANativeWindow_Buffer nativeWindow_buffer;
    int ret;

    LOGD("FFMPEG Player: DECODE -------- BEGIN");

    while (av_read_frame(formatContext, avPacket) >= 0) {
        if (avPacket->stream_index == video_stream_index) {
            //解码
            ret = avcodec_send_packet(avCodecContext, avPacket);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                return;
            }
            ret = avcodec_receive_frame(avCodecContext, avFrame);
            if (ret < 0 && ret != AVERROR_EOF) {
                LOGE("video avcodec_receive_frame error %d", ret);
                av_packet_unref(avPacket);
                continue;
            }

            sws_scale(swsContext, (const uint8_t *const *) avFrame->data, avFrame->linesize, 0,
                      avCodecContext->height, rgb_frame->data, rgb_frame->linesize);
            if (ANativeWindow_lock(pNativeWindow, &nativeWindow_buffer, NULL) < 0) {
                LOGD("FFMPEG Player: can not lock window. ");
                usleep(16000);
            } else {
                uint8_t *dst = (uint8_t *) nativeWindow_buffer.bits;
                //像素数据的首地址
                uint8_t *src = rgb_frame->data[0];
                //一行包含多少RGBA ---> 多少个像素
                int destStride = nativeWindow_buffer.stride * 4;
                //实际一行包含的像素点内存量
                int srcStride = rgb_frame->linesize[0];
                for (int i = 0; i < height; ++i) {
                    memcpy(dst + i * destStride, src + i * srcStride, srcStride);
                }
                ANativeWindow_unlockAndPost(pNativeWindow);
            }
        }
        av_packet_unref(avPacket);
    }
    LOGD("FFMPEG Player: DECODE -------- end");

    //释放资源
    ANativeWindow_release(pNativeWindow);
    sws_freeContext(swsContext);
    av_frame_free(&avFrame);
    av_frame_free(&rgb_frame);
    avcodec_close(avCodecContext);
    avformat_close_input(&formatContext);

    env->ReleaseStringUTFChars(url_, url);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_gu_player_BasePlayer_doFFInit(JNIEnv *env, jobject instance) {

    if (player) {
        player->ff_destroy();
        player = NULL;
    }
    player->ff_init(env);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_gu_player_BasePlayer_doFFPrepare(JNIEnv *env, jobject instance) {
    player->ff_prepare();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_gu_player_BasePlayer_doFFStart(JNIEnv *env, jobject instance) {

    player->ff_start();

}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_gu_player_BasePlayer_doFFPause(JNIEnv *env, jobject instance) {

    jlong pausePos = player->ff_pause();
    return 0;
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_gu_player_BasePlayer_doFFStop(JNIEnv *env, jobject instance) {

    jlong stopPos = player->ff_stop();

    return 0;
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_gu_player_BasePlayer_doFFSeekTo(JNIEnv *env, jobject instance, jlong target) {

    jlong finalPos = player->ff_seek_to(target);
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_gu_player_BasePlayer_doFFDestroy(JNIEnv *env, jobject instance) {

    player->ff_destroy();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_gu_player_BasePlayer_doFFRest(JNIEnv *env, jobject instance) {
    player->ff_rest(env);
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_gu_player_BasePlayer_doFFPlay(JNIEnv *env, jobject instance, jstring playUrl_) {
    const char *playUrl = env->GetStringUTFChars(playUrl_, 0);

    jlong  duration = player->ff_set_data_source(env,playUrl);
    player->ff_start();

    env->ReleaseStringUTFChars(playUrl_, playUrl);
    return 0;
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_gu_player_BasePlayer_setFFDataSource(JNIEnv *env, jobject instance, jstring playUrl_) {
    const char *playUrl = env->GetStringUTFChars(playUrl_, 0);

    jlong  duration = player->ff_set_data_source(env,playUrl);

    env->ReleaseStringUTFChars(playUrl_, playUrl);
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_gu_player_BasePlayer_attachFFView(JNIEnv *env, jobject instance, jobject surface) {

    player->ff_attach_window(env,surface);

}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_gu_player_BasePlayer_getFFPos(JNIEnv *env, jobject instance) {

    jlong  duration = player->ff_get_current_pos(env);
    return 0;
}