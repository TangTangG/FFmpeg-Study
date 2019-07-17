# FFMpeg-libswscale主要函数分析

libswscale是一个主要用于处理图片像素数据的类库。可以完成图片像素格式的转换，图片的拉伸等工作。

libswscale常用的函数数量很少，一般情况下就3个：

sws_getContext()：初始化一个SwsContext。（可以用sws_getCachedContext()取代）

sws_scale()：处理图像数据。

sws_freeContext()：释放一个SwsContext。



<u>*YUV：“Y”表示明亮度（Luminance或Luma），也就是灰阶值，“U”和“V”表示的则是色度（Chrominance或Chroma），作用是描述影像色彩及饱和度*</u>

### 函数调用关系如下

![img](./20150317145505134)

### sws_scale()

![img](/20150317194542751)

libswscale处理数据有两条最主要的方式：unscaled和scaled。

unscaled用于处理不需要拉伸的像素数据（属于比较特殊的情况），

scaled用于处理需要拉伸的像素数据。Unscaled只需要对图像像素格式进行转换；而Scaled则除了对像素格式进行转换之外，还需要对图像进行缩放。

##### Scaled方式可以分成以下几个步骤：

XXX to YUV Converter：首相将数据像素数据转换为8bitYUV格式；

Horizontal scaler：水平拉伸图像，并且转换为15bitYUV；

Vertical scaler：垂直拉伸图像；

Output converter：转换为输出像素格式。