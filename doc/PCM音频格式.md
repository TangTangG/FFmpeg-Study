# PCM音频格式

脉冲编码调制(Pulse Code Modulation,*PCM*)

PCM：其数据排列格式为左右声道每个样本点数据交错排列![20180710192426957](.\20180710192426957.png)

16bit的PCM数据是有负数的，至于负数的意义，我估计是代表电压的正负的，0值代表无声。

如下为FFmpeg中所定义的音频格式：

enum AVSampleFormat {
AV_SAMPLE_FMT_NONE = -1,
AV_SAMPLE_FMT_U8, ///< unsigned 8 bits
AV_SAMPLE_FMT_S16, ///< signed 16 bits
AV_SAMPLE_FMT_S32, ///< signed 32 bits
AV_SAMPLE_FMT_FLT, ///< float
AV_SAMPLE_FMT_DBL, ///< double

// 以下都是带平面格式
AV_SAMPLE_FMT_U8P, ///< unsigned 8 bits, planar
AV_SAMPLE_FMT_S16P, ///< signed 16 bits, planar
AV_SAMPLE_FMT_S32P, ///< signed 32 bits, planar
AV_SAMPLE_FMT_FLTP, ///< float, planar
AV_SAMPLE_FMT_DBLP, ///< double, planar

AV_SAMPLE_FMT_NB ///< Number of sample formats. DO NOT USE if linking dynamically
};
其中，

AV_SAMPLE_FMT_FLT 为float类型（4字节）
AV_SAMPLE_FMT_FLTP 为平面排列float类型（4字节）

带P和不带P的数据类型的区别：
P表示Planar（平面），其数据格式排列方式为 :
LLLLLLRRRRRRLLLLLLRRRRRRLLLLLLRRRRRRL...（每个LLLLLLRRRRRR为一个音频帧）
而不带P的数据格式（即交错排列）排列方式为：
LRLRLRLRLRLRLRLRLRLRLRLRLRLRLRLRLRLRL...（每个LR为一个音频样本）

注：L表示左声道一个采样点数据，R表示右声道一个采样点数据

之前由于没有透彻了解音频的数据结构，使用Adobe Audition软件播放PCM音频数据总是出现问题，如下图所示，此图的原始数据为左声道为0而右声道为正常声音采集，由于是Planar的数据排列方式而非交错排列方式，导致最终音频失真。![20180710192858995](.\20180710192858995.png)
