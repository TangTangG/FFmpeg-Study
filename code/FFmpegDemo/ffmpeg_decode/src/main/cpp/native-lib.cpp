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
Java_com_gu_ffmpeg_1decode_FFDecodePlayer_decode(JNIEnv *env, jobject instance, jstring url_) {
    const char *url = env->GetStringUTFChars(url_, 0);

    av_log_set_callback(custom_log);

    env->ReleaseStringUTFChars(url_, url);
}