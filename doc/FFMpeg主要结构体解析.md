# FFMpeg主要结构体解析

AVCodec是存储编解码器信息的结构体，主要字段注释如下：

- const char *name; //编解码器名字
- const char *long_name; //编解码器全名
- enum AVMediaType type; //编解码器类型
- enum AVCodecID id; //编解码器ID
- const AVRational *supported_framerates; //支持帧率数组（视频）
- const enum AVPixelFormat *pix_fmts; //支持像素格式数组(视频）
- const int *supported_samplerates; //支持音频采样率数组(音频）
- const enum AVSampleFormat *sample_fmts; //支持采样格式数组(音频)
- const uint64_t *channel_layouts; //支持声道数（音频）
- const AVClass *priv_class; //私有数据