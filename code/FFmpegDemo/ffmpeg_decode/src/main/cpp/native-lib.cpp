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
#include <android/native_window_jni.h>
#include <unistd.h>
}


void custom_log(void *ptr, int level, const char *fmt, va_list vl) {
    /*r 打开只读文件，该文件必须存在。 
    *r+ 打开可读写的文件，该文件必须存在。 
    *w 打开只写文件，若文件存在则文件长度清为0，即该文件内容会消失。若文件不存在则建立该文件。 
    *w+ 打开可读写文件，若文件存在则文件长度清为零，即该文件内容会消失。若文件不存在则建立该文件。 
    *a 以附加的方式打开只写文件。若文件不存在，则会建立该文件，如果文件存在，写入的数据会被加到文件尾，即文件原先的内容会被保留。 
    *a+ 以附加方式打开可读写的文件。若文件不存在，则会建立该文件，如果文件存在，写入的数据会被加到文件尾后，即文件原先的内容会被保留。 
    *上述的形态字符串都可以再加一个b字符，如rb、w+b或ab+等组合，
     加入b 字符用来告诉函数库打开的文件为二进制文件，而非纯文字文件*/
    FILE *fp = fopen("/storage/emulated/0/av_log.txt", "a+");
    if (fp) {
        vfprintf(fp, fmt, vl);
        fflush(fp);
        fclose(fp);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_gu_ffmpeg_1decode_FFDecodePlayer_decode(JNIEnv *env, jobject instance, jstring url_,
                                                 jstring out_) {
    const char *url = env->GetStringUTFChars(url_, 0);
    const char *out = env->GetStringUTFChars(out_, 0);

    char info[1000]={0};

    av_log_set_callback(custom_log);

    avformat_network_init();

    AVFormatContext *pFormatContext = avformat_alloc_context();
    if (avformat_open_input(&pFormatContext, url, NULL, NULL) != 0) {
        LOGE("Couldn`t open input stream.\n");
        return;
    }
    if (avformat_find_stream_info(pFormatContext, NULL) < 0) {
        LOGE("Couldn`t find stream information.\n");
        return;
    }
    int videoIndex = -1;

    for (int i = 0; i < pFormatContext->nb_streams; ++i) {
        if (pFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
            break;
        }
    }

    if (videoIndex == -1) {
        LOGE("Couldn`t find a video code.\n");
        return;
    }

    AVCodecParameters *codecpar = pFormatContext->streams[videoIndex]->codecpar;
    AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
    AVCodecContext *pCodecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(pCodecContext, codecpar);
    if (avcodec_open2(pCodecContext, codec, NULL) < 0) {
        LOGE("Couldn`t open codec.\n");
        return;
    }
    int width = pCodecContext->width;
    int height = pCodecContext->height;

    AVFrame *avFrame = av_frame_alloc();
    AVFrame *yuvFrame = av_frame_alloc();

    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 1);
    uint8_t *out_buffer = (uint8_t *) (av_malloc(bufferSize));

    av_image_fill_arrays(yuvFrame->data, yuvFrame->linesize, out_buffer, AV_PIX_FMT_YUV420P, width,
                         height, 1);

    AVPacket *avPacket = (AVPacket *) av_malloc((sizeof(AVPacket)));

    SwsContext *swsContext = sws_getContext(width, height, pCodecContext->pix_fmt, width, height,
                                            AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    FILE *fp_yuv = fopen(out, "wb+");
    if (fp_yuv == NULL){
        LOGE("Couldn`t open/create output file %s.\n",out);
        return;
    }
    int ret;

    int frame_count = 0;
    int y_count = 0;
    clock_t startTime = clock();

    LOGD("FFMPEG Player: DECODE -------- BEGIN");

    while (av_read_frame(pFormatContext, avPacket) >= 0) {
        if (avPacket->stream_index == videoIndex) {
            //解码
            ret = avcodec_send_packet(pCodecContext, avPacket);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                return;
            }
            ret = avcodec_receive_frame(pCodecContext, avFrame);
            if (ret < 0 && ret != AVERROR_EOF) {
                LOGE("video avcodec_receive_frame error %d", ret);
                av_packet_unref(avPacket);
                continue;
            }

            sws_scale(swsContext, (const uint8_t *const *) avFrame->data, avFrame->linesize, 0,
                      height, yuvFrame->data, yuvFrame->linesize);
            y_count = width *height;
            fwrite(yuvFrame->data[0],1,y_count,fp_yuv);//y
            fwrite(yuvFrame->data[1],1,y_count >> 1,fp_yuv);//u
            fwrite(yuvFrame->data[2],1,y_count >> 1,fp_yuv);//v

            //Output info
            char pictype_str[10]={0};
            switch(avFrame->pict_type){
                case AV_PICTURE_TYPE_I:sprintf(pictype_str,"I");break;
                case AV_PICTURE_TYPE_P:sprintf(pictype_str,"P");break;
                case AV_PICTURE_TYPE_B:sprintf(pictype_str,"B");break;
                default:sprintf(pictype_str,"Other");break;
            }
            LOGD("Frame Index: %5d. Type:%s",frame_count,pictype_str);
            frame_count++;
        }
        av_packet_unref(avPacket);
    }
    //flush decoder
    while(1){
        ret = avcodec_send_packet(pCodecContext, avPacket);
        if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            break;
        }
        ret = avcodec_receive_frame(pCodecContext, avFrame);
        if (ret < 0 && ret != AVERROR_EOF) {
            LOGE("video avcodec_receive_frame error %d", ret);
            av_packet_unref(avPacket);
            continue;
        }
        sws_scale(swsContext, (const uint8_t *const *) avFrame->data, avFrame->linesize, 0,
                  height, yuvFrame->data, yuvFrame->linesize);
        y_count = width *height;
        fwrite(yuvFrame->data[0],1,y_count,fp_yuv);//y
        fwrite(yuvFrame->data[1],1,y_count >> 1,fp_yuv);//u
        fwrite(yuvFrame->data[2],1,y_count >> 1,fp_yuv);//v

        //Output info
        char pictype_str[10]={0};
        switch(avFrame->pict_type){
            case AV_PICTURE_TYPE_I:sprintf(pictype_str,"I");break;
            case AV_PICTURE_TYPE_P:sprintf(pictype_str,"P");break;
            case AV_PICTURE_TYPE_B:sprintf(pictype_str,"B");break;
            default:sprintf(pictype_str,"Other");break;
        }
        LOGD("Frame Index: %5d. Type:%s",frame_count,pictype_str);
        frame_count++;
    }

    clock_t endTime = clock();

    sprintf(info, "%s[Time      ]%fms\n",info,(double)(endTime - startTime));
    sprintf(info, "%s[Count     ]%d\n",info,frame_count);

    fclose(fp_yuv);

    sws_freeContext(swsContext);
    av_frame_free(&avFrame);
    av_frame_free(&yuvFrame);
    avcodec_close(pCodecContext);
    avformat_close_input(&pFormatContext);
    env->ReleaseStringUTFChars(url_, url);
    env->ReleaseStringUTFChars(out_, out);
}