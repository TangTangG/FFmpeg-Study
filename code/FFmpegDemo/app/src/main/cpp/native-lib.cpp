#include <jni.h>
#include <string>
#include <android/log.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES.h>

#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR, "player", FORMAT, ##__VA_ARGS__);
#define LOGD(FORMAT, ...) __android_log_print(ANDROID_LOG_DEBUG, "player", FORMAT, ##__VA_ARGS__);

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include <android/native_window_jni.h>
#include <unistd.h>
}

extern "C"
JNIEXPORT void JNICALL
Java_com_player_ffmpegdemo_FFPlayer_doFFplay(JNIEnv *env, jobject instance, jobject surface,
                                             jstring url_) {
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
        return;
    }

    //查看文件视频流信息
    result = avformat_find_stream_info(formatContext, NULL);
    if (result < 0) {
        LOGE("FFMPEG Player Error: Can not find video file stream info")
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
        //可能传入的是一个单纯的音频流
        LOGD("FFMPEG Player : Can not find video codec")
        return;
    }
    //初始化视频流编码器上下文
    AVCodecContext *avCodecContext = formatContext->streams[video_stream_index]->codec;;
    //初始化对应流的编码器
    AVCodec *avCodec = avcodec_find_decoder(avCodecContext->codec_id);
    result = avcodec_open2(avCodecContext, avCodec, NULL);
    if (result < 0) {
        LOGE("FFMPEG Player Error: Can not open video file");
        return;
    }
    //对于视频（Video）来说，AVPacket通常包含一个压缩的Frame；而音频（Audio）则有可能包含多个压缩的Frame
    //关于这些数据的一些附加的信息，如显示时间戳（pts），解码时间戳（dts）,数据时长（duration）
    AVPacket *avPacket = static_cast<AVPacket *>(av_malloc(sizeof(AVPacket)));
    av_init_packet(avPacket);

    AVFrame *avFrame = av_frame_alloc();
    AVFrame *rgb_frame = av_frame_alloc();

    //建立缓存区
    uint8_t *out_buffer = static_cast<uint8_t *>(av_malloc(
            static_cast<size_t>(av_image_get_buffer_size(AV_PIX_FMT_RGBA, avCodecContext->width,
                                                         avCodecContext->height, 0))));
    //与缓存区相关联，设置rgb_frame
    av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, out_buffer, AV_PIX_FMT_RGBA,
                         avCodecContext->width, avCodecContext->height, 0);

    SwsContext *swsContext = sws_getContext(avCodecContext->width, avCodecContext->height,
                                            avCodecContext->pix_fmt,
                                            avCodecContext->width, avCodecContext->height,
                                            AV_PIX_FMT_RGBA,
                                            SWS_BICUBIC, NULL, NULL, NULL);
    //根据surface创建一个用于展示的window
    ANativeWindow *pNativeWindow = ANativeWindow_fromSurface(env, surface);
    if (pNativeWindow == 0) {
        LOGE("FFMPEG Player Error: ANativeWindow get failed");
    }

    ANativeWindow_Buffer nativeWindow_buffer;

    int frameCount;
    int h = 0;
    LOGD("FFMPEG Player: DECODE -------- BEGIN");

    while (av_read_frame(formatContext, avPacket) >= 0) {
        LOGD("FFMPEG Player: DECODE -------- stream_index %d", avPacket->stream_index);
        LOGD("FFMPEG Player: DECODE -------- codec_index %d", video_stream_index);
        if (avPacket->stream_index == video_stream_index) {
            LOGD("FFMPEG Player: DECODE -------- tag ");
            //解码
            avcodec_send_packet(avCodecContext, avPacket);
            frameCount = avcodec_receive_frame(avCodecContext, avFrame);
            LOGD("FFMPEG Player: DECODE ..... %d", frameCount)
            if (frameCount) {
                LOGD("FFMPEG Player: DECODE ..... transfrom")
                ANativeWindow_setBuffersGeometry(pNativeWindow, avCodecContext->width,
                                                 avCodecContext->height, WINDOW_FORMAT_RGBA_8888);
                ANativeWindow_lock(pNativeWindow, &nativeWindow_buffer, NULL);
                //对frame所有像素点进行缩放
                sws_scale(swsContext, (const uint8_t *const *)avFrame->data, avFrame->linesize, 0, avFrame->height,
                          rgb_frame->data, rgb_frame->linesize);
                //获取缩放之后的像素数据
                uint8_t *dst = static_cast<uint8_t *>(nativeWindow_buffer.bits);
                //一行包含多少RGBA ---> 多少个像素
                int destStride = nativeWindow_buffer.stride *4;
                //像素数据的首地址
                uint8_t *src = rgb_frame->data[0];
                //实际一行包含的像素点内存量
                int srcStride = rgb_frame->linesize[0];
                for (int i = 0; i < avCodecContext->height; ++i) {
                    memcpy(dst + i * destStride,src + i * srcStride, static_cast<size_t>(srcStride));
                }
                ANativeWindow_unlockAndPost(pNativeWindow);
                //这里应该引入生产消费模型，暂时阻塞
                usleep(1000*16);

            }
        }
        av_packet_unref(avPacket);
    }
    //释放资源
    ANativeWindow_release(pNativeWindow);
    av_frame_free(&avFrame);
    av_frame_free(&rgb_frame);
    avcodec_close(avCodecContext);
    avformat_free_context(formatContext);

    env->ReleaseStringUTFChars(url_, url);
    return;
}