# Android Mediaplayer源码解析

##### 播放流程必要函数：

```java
setDataSource();//设置数据源，支持多种格式
setDisplay(SurfaceHolder sh);//设置用于显示的surface
setSurface(Surface surface);//
prepare();//准备
start();//开始播放
pause();//暂停
stop();//停止
release();//释放
reset();//重置
```

##### native层

在初始化MediaPlayer.class的时候加载C代码。

```java
static {
    System.loadLibrary("media_jni");
    native_init();
}
```

主要代码都在**/framework/base/media**/jni下。

*注：实际上关于media的大部分代码都在/framework/base/media下，MediaPlayer对应的JNI实现在**frameworks/base/media/jni/android_media_MediaPlayer.cpp**

主要从以下几个方面对MediaPlayer进行解析：

1、编解码

2、渲染

3、控制逻辑的实现

## 总结

