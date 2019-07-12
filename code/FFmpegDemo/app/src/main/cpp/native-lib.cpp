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

extern "C"
JNIEXPORT jint JNICALL
Java_com_player_ffmpegdemo_FFPlayer_doFFplay(JNIEnv *env, jobject instance, jobject surface,
                                             jstring url_) {
    int result;

    const char *url = env->GetStringUTFChars(url_, 0);
    char buf[] = {0};

    //we can omit this function call in ffmpeg 4.0 and later.
    av_register_all();
//    av_log_set_callback(ffmpeg_log);

    //初始化 码流参数 上下文
    AVFormatContext *formatContext = avformat_alloc_context();
    //读取视频文件
    result = avformat_open_input(&formatContext, url, NULL, NULL);
    if (result < 0) {
        av_strerror(result, buf, 1024);
        // LOGE("%s" ,inputPath)
        LOGE("Couldn't open file %s: %d(%s)", url, result, buf);
        LOGE("FFMPEG Player Error: Can not open video file")
        return -1;
    }

    //查看文件视频流信息
    result = avformat_find_stream_info(formatContext, NULL);
    if (result < 0) {
        LOGE("FFMPEG Player Error: Can not find video file stream info")
        return -1;
    }
    //查找视频流对应解码器
    int video_stream_index = -1;
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }

    if (video_stream_index == -1) {
        LOGD("FFMPEG Player : Can not find video codec")
        return -1;
    }
    //初始化视频流编码器上下文
    AVCodecContext *avCodecContext = formatContext->streams[video_stream_index]->codec;;
    //初始化对应流的编码器
    AVCodec *avCodec = avcodec_find_decoder(avCodecContext->codec_id);
    result = avcodec_open2(avCodecContext, avCodec, NULL);
    if (result < 0) {
        LOGE("FFMPEG Player Error: Can not open video file");
        return -1;
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
    //与缓存区相关联，设置rgb_frame
    av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, (uint8_t *) out_buffer,
                         AV_PIX_FMT_RGBA,
                         width, height, 1);

    SwsContext *swsContext = sws_getContext(width, height,
                                            avCodecContext->pix_fmt,
                                            width, height,
                                            AV_PIX_FMT_RGBA,
                                            SWS_BICUBIC, NULL, NULL, NULL);
    //根据surface创建一个用于展示的window
    ANativeWindow *pNativeWindow = ANativeWindow_fromSurface(env, surface);
    if (pNativeWindow == 0) {
        LOGE("FFMPEG Player Error: ANativeWindow get failed");
        return -1;
    }
    if (ANativeWindow_setBuffersGeometry(pNativeWindow, width, height, WINDOW_FORMAT_RGBA_8888) <
        0) {
        LOGE("FFMPEG Player Error: Couldn't set buffers geometry.");
        return -1;
    }

    ANativeWindow_Buffer nativeWindow_buffer;

    int ret;

    LOGD("FFMPEG Player: DECODE -------- BEGIN");

    while (av_read_frame(formatContext, avPacket) >= 0) {
        LOGD("FFMPEG Player: DECODE -------- stream_index %d", avPacket->stream_index);
        LOGD("FFMPEG Player: DECODE -------- codec_index %d", video_stream_index);
        if (avPacket->stream_index == video_stream_index) {
//            LOGD("FFMPEG Player: DECODE -------- tag ");
            //解码
            ret = avcodec_send_packet(avCodecContext, avPacket);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                return -1;
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
                LOGD("FFMPEG Player: can not lock window. ")
            } else {
                uint8_t *dst = (uint8_t *) nativeWindow_buffer.bits;
                //一行包含多少RGBA ---> 多少个像素
                int destStride = nativeWindow_buffer.stride * 4;
                //实际一行包含的像素点内存量
                int srcStride = rgb_frame->linesize[0];
                for (int i = 0; i < avCodecContext->height; ++i) {
                    memcpy(dst + i * destStride, out_buffer + i * srcStride, srcStride);
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
    return 0;

    /*
     ANativeWindow *nativeWindow;
    ANativeWindow_Buffer windowBuffer;
     AVFormatContext *pFormatCtx;
    int i, videoindex;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    struct SwsContext *img_convert_ctx;
    char input_str[500] = {0};
    sprintf(input_str, "%s", url);
    av_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();
    if (avformat_open_input(&pFormatCtx, input_str, NULL, NULL) != 0) {
        LOGE("Couldn't open input stream.\n");
        return -1;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("Couldn't find stream information.\n");
        return -1;
    }
    videoindex = -1;
    for (i = 0; i < pFormatCtx->nb_streams; i++)
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex = i;
            break;
        }
    if (videoindex == -1) {
        LOGE("Couldn't find a video stream.\n");
        return -1;
    }
    pCodecCtx = pFormatCtx->streams[videoindex]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        LOGE("Couldn't find Codec.\n");
        return -1;
    }
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGE("Couldn't open codec.\n");
        return -1;
    }
    //获取界面传下来的surface
    nativeWindow = ANativeWindow_fromSurface(env, surface);
    if (0 == nativeWindow) {
        LOGE("Couldn't get native window from surface.\n");
        return -1;
    }
    int width = pCodecCtx->width;
    int height = pCodecCtx->height;
    //分配一个帧指针，指向解码后的原始帧
    AVFrame *vFrame = av_frame_alloc();
    AVPacket *vPacket = (AVPacket *) av_malloc(sizeof(AVPacket));
    AVFrame *pFrameRGBA = av_frame_alloc();
    img_convert_ctx = sws_getContext(width, height, pCodecCtx->pix_fmt,
                                     width, height, AV_PIX_FMT_RGBA, SWS_BICUBIC, NULL, NULL, NULL);
    if (0 >
        ANativeWindow_setBuffersGeometry(nativeWindow, width, height, WINDOW_FORMAT_RGBA_8888)) {
        LOGE("Couldn't set buffers geometry.\n");
        ANativeWindow_release(nativeWindow);
        return -1;
    }
    //读取帧
    while (av_read_frame(pFormatCtx, vPacket) >= 0) {
        if (vPacket->stream_index == videoindex) {
            //视频解码
            int ret = avcodec_send_packet(pCodecCtx, vPacket);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                LOGE("video avcodec_send_packet error %d", ret);
                return -1;
            }
            ret = avcodec_receive_frame(pCodecCtx, vFrame);
            if (ret < 0 && ret != AVERROR_EOF) {
                LOGE("video avcodec_receive_frame error %d", ret);
                av_packet_unref(vPacket);
                continue;
            }
            //转化格式
            sws_scale(img_convert_ctx, (const uint8_t *const *) vFrame->data, vFrame->linesize, 0,
                      pCodecCtx->height,
                      pFrameRGBA->data, pFrameRGBA->linesize);
            if (ANativeWindow_lock(nativeWindow, &windowBuffer, NULL) < 0) {
                LOGE("cannot lock window");
            } else {
                av_image_fill_arrays(pFrameRGBA->data, pFrameRGBA->linesize,
                                     (const uint8_t *) windowBuffer.bits, AV_PIX_FMT_RGBA,
                                     width, height, 1);
                ANativeWindow_unlockAndPost(nativeWindow);
            }
        }
        av_packet_unref(vPacket);
    }
    //释放内存
    sws_freeContext(img_convert_ctx);
    av_free(vPacket);
    av_free(pFrameRGBA);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    return 0;*/
}