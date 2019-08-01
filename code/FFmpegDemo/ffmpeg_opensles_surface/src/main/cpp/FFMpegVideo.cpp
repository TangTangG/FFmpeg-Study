//
// Created by tangcaigao on 2019/7/18.
//
#include "util/FFThreadPool.h"
#include "FFMpegVideo.h"

#define MIN_VIDEO_CLOCK_DIFF_MS 25
#define MAX_VIDEO_CLOCK_DIFF_MS 1500


//error define ----begin
char errorBuf[] = {0};
int errorState;
//error define ----end

bool pop(FFMpegVideo *pVideo, AVPacket *pPacket) {

    FFLockedQueue<AVPacket> *queue = pVideo->queue;
    AVPacket *avPacket = queue->pop();
    if (avPacket == NULL) {
        return false;
    }
    if (!av_packet_ref(pPacket, avPacket)) {
        av_packet_unref(avPacket);
        AVPacket *remove = queue->releaseHead();
        av_free(remove);
    }
    return true;
}

void FFMpegVideo::push(AVPacket *pPacket) {
    queue->push(pPacket);
}

static double videoTimeRedress(FFMpegVideo *pVideo, int64_t pts) {
    NativePlayerContext *ctx = pVideo->pCtx;
    double audio_clock = ctx->audio_clock;
    double play  = av_q2d(pVideo->time_base) * pts;

    if (play != 0)
        ctx->video_clock=play;
    else //pst为0 则先把pts设为上一帧时间
        play = ctx->video_clock;
    //可能有pts为0 则主动增加clock
    //frame->repeat_pict = 当解码时，这张图片需要要延迟多少
    //需要求出扩展延时：
    //extra_delay = repeat_pict / (2*fps) 显示这样图片需要延迟这么久来显示
    double repeat_pict = pVideo->avFrame->repeat_pict;
    //使用AvCodecContext的而不是stream的
    double frame_delay = av_q2d(pVideo->time_base);
    //如果time_base是1,25 把1s分成25份，则fps为25
    //fps = 1/(1/25)
    double fps = 1 / frame_delay;
    //pts 加上 这个延迟 是显示时间
    double extra_delay = repeat_pict / (2 * fps);
    double delay = extra_delay + frame_delay;
//    LOGI("extra_delay:%f",extra_delay);
    ctx->video_clock += delay;
    LOGD("video_clock info: %f",ctx->video_clock);
    return 0;
}

static void start_render_notify(void *pVideo, void *out) {
    FFMpegVideo *video = static_cast<FFMpegVideo *>(pVideo);
    AVPacket *avPacket = (AVPacket *) av_malloc(sizeof(AVPacket));
    int result;
    int height;
    ANativeWindow_Buffer nativeWindow_buffer;
    AVFrame *avFrame = video->avFrame;
    while (video->pCtx->play_state > 0) {
        if (!pop(video, avPacket)) {
            usleep(16000);
            continue;
        }
        if (avPacket->pts != AV_NOPTS_VALUE) {
            //qts --> double 校正时间
            video->pCtx->video_clock = av_q2d(video->time_base) * avPacket->pts;
        }
        //解码
        result = avcodec_send_packet(video->avCodecCtx, avPacket);
        if (result != 0) {
            if (result != AVERROR(EAGAIN) && result != AVERROR_EOF) {
                LOGE("video avcodec_send_packet error %d", result);
                av_packet_unref(avPacket);
                continue;
            }
        }

        result = avcodec_receive_frame(video->avCodecCtx, avFrame);
        if (result != 0) {
            if (result != AVERROR_EOF) {
                LOGE("video avcodec_receive_frame error %d", result);
                continue;
            }/* else {
                LOGD("video down ------ ");
                video->pCtx->video_down = true;
                break;
            }*/
        }

        height = video->avCodecCtx->height;
        sws_scale(video->swsContext, (const uint8_t *const *) avFrame->data, avFrame->linesize, 0,
                  height, video->rgb_frame->data, video->rgb_frame->linesize);

        double diff = videoTimeRedress(video, avFrame->best_effort_timestamp);
        if (diff < -MIN_VIDEO_CLOCK_DIFF_MS || diff > MAX_VIDEO_CLOCK_DIFF_MS) {
            av_packet_unref(avPacket);
            //超出两个阈值的都认为是异常帧，直接丢弃
            continue;
        } else if (diff > MIN_VIDEO_CLOCK_DIFF_MS && diff <= MAX_VIDEO_CLOCK_DIFF_MS) {
            usleep((diff - MIN_VIDEO_CLOCK_DIFF_MS) * 1000);
        }

        if (ANativeWindow_lock(video->pNativeWindow, &nativeWindow_buffer, NULL) >= 0) {
            uint8_t *dst = (uint8_t *) nativeWindow_buffer.bits;
            //像素数据的首地址
            uint8_t *src = video->rgb_frame->data[0];
            //一行包含多少RGBA ---> 多少个像素
            int destStride = nativeWindow_buffer.stride * 4;
            //实际一行包含的像素点内存量
            int srcStride = video->rgb_frame->linesize[0];
            for (int i = 0; i < height; ++i) {
                memcpy(dst + i * destStride, src + i * srcStride, srcStride);
            }
            ANativeWindow_unlockAndPost(video->pNativeWindow);
        }
        av_packet_unref(avPacket);
    }
    av_free(avPacket);
    av_frame_free(&avFrame);
    video->release();
}

void FFMpegVideo::create(NativePlayerContext *ctx) {
    avFrame = av_frame_alloc();
    rgb_frame = av_frame_alloc();
    pCtx = ctx;
    queue = new FFLockedQueue<AVPacket>();
    queue->init();
}

jlong FFMpegVideo::decode(const char *url) {
    AVFormatContext *formatCtx = pCtx->formatCtx;

    //查找视频流对应解码器
    video_stream_index = av_find_best_stream(formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL,
                                             0);
    if (video_stream_index == -1) {
        LOGD("FFMPEG Player : Can not find video stream");
        return 0L;
    }

    //初始化视频流编码器上下文
    AVCodecParameters *codecpar = formatCtx->streams[video_stream_index]->codecpar;
    avCodec = avcodec_find_decoder(codecpar->codec_id);
    avCodecCtx = avcodec_alloc_context3(avCodec);
    time_base = pCtx->formatCtx->streams[video_stream_index]->time_base;

    if (pCtx->debug) {
        LOGD("---------- dumping video stream info begin ----------");

        LOGD("input format: %s", formatCtx->iformat->name);
        LOGD("nb_streams: %d", formatCtx->nb_streams);

        int64_t start_time = formatCtx->start_time / AV_TIME_BASE;
        LOGD("start_time: %lld", start_time);

        int64_t duration = formatCtx->duration / AV_TIME_BASE;
        LOGD("duration: %lld s", duration);

        int video_stream_idx = av_find_best_stream(formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL,
                                                   0);
        if (video_stream_idx >= 0) {
            AVStream *video_stream = formatCtx->streams[video_stream_idx];
            LOGD("video nb_frames: %lld", video_stream->nb_frames);
            LOGD("video codec_id: %d", codecpar->codec_id);
            LOGD("video codec_name: %s", avcodec_get_name(codecpar->codec_id));
            LOGD("video width x height: %d x %d", codecpar->width, codecpar->height);
            LOGD("video pix_fmt: %d", avCodecCtx->pix_fmt);
            LOGD("video bitrate %lld kb/s", (int64_t) codecpar->bit_rate / 1000);
            LOGD("video avg_frame_rate: %d fps",
                 video_stream->avg_frame_rate.num / video_stream->avg_frame_rate.den);
        }
        LOGD("---------- dumping video stream info end ----------");
    }
    avcodec_parameters_to_context(avCodecCtx, codecpar);
    ff_threadpool_add(pCtx->threadPoolCtx, start_render_notify, this, NULL);
    return formatCtx->duration / AV_TIME_BASE;
}

bool FFMpegVideo::renderInit() {
    if (pNativeWindow == 0) {
        pNativeWindow = static_cast<ANativeWindow *>(pCtx->display);
    }
    if (pNativeWindow == 0) {
        LOGE("FFMPEG Player Error: ANativeWindow get failed");
        return false;
    }
    errorState = avcodec_open2(avCodecCtx, avCodec, NULL);
    if (errorState < 0) {
        LOGE("FFMPEG Player Error: Can not open video file");
    }

    //分配缓存区域，取帧 渲染
    int width = avCodecCtx->width;
    int height = avCodecCtx->height;
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, width, height, 1);
    //建立缓存区
    uint8_t *out_buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    //填充rgb_frame
    av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, out_buffer,
                         AV_PIX_FMT_RGBA,
                         width, height, 1);

    swsContext = sws_getContext(width, height,
                                avCodecCtx->pix_fmt,
                                width, height,
                                AV_PIX_FMT_RGBA,
                                SWS_BICUBIC, NULL, NULL, NULL);
    if (ANativeWindow_setBuffersGeometry(pNativeWindow, width, height, WINDOW_FORMAT_RGBA_8888) <
        0) {
        queue->stop();
        LOGE("FFMPEG Player Error: Couldn't set buffers geometry.");
        return false;
    }
    return true;
}

void FFMpegVideo::release() {
    ANativeWindow_release(pNativeWindow);
    sws_freeContext(swsContext);
    av_frame_free(&avFrame);
    av_frame_free(&rgb_frame);
    avcodec_close(avCodecCtx);
    avformat_close_input(&(pCtx->formatCtx));
}

void FFMpegVideo::reset() {

}

void FFMpegVideo::destroy() {
    queue->free();
    delete queue;
    queue = NULL;
}


