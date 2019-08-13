package com.gu.android_mediacodec_acc;

import android.annotation.TargetApi;
import android.content.Context;
import android.graphics.PixelFormat;
import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.os.Build;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

import com.gu.player.BasePlayer;
import com.gu.player.SimpleThreadPoolExecutor;

import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

/**
 * MediaExtractor:辅助视频流中提取多轨道媒体数据
 * MediaSync: 同步音频
 * MediaMuxer: 音频混合器
 * MediaCrypto: 解码加密流的辅助类    Crypto(加密)
 * MediaDrm:DRM加密
 * Image:图像缓冲区 --->  MediaCodec or a CameraDevice
 * Surface:展示
 * AudioTrack:音轨信息
 *
 * MediaFormat:设置输出样式，包括大小，缩放，码率等等
 */
@TargetApi(Build.VERSION_CODES.LOLLIPOP)
public class AndroidMediaCodecPlayer extends BasePlayer {

    private static final String TAG = "media-codec";

    private final Context context;
    private SurfaceView surfaceView;
    private SurfaceHolder holder;
    private MediaCodec mediaCodec;
    private MediaExtractor extractor;

    private String playUrl;

    public AndroidMediaCodecPlayer(Context context) {
        this.context = context;
    }

    @Override
    public View init(final SurfaceView surfaceView) {
        this.surfaceView = new SurfaceView(context);
        this.surfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(final SurfaceHolder holder) {
                if (AndroidMediaCodecPlayer.this.holder != null) {
                    return;
                }
                AndroidMediaCodecPlayer.this.holder = holder;
                holder.setFormat(PixelFormat.RGBA_8888);

                if (TextUtils.isEmpty(playUrl)) {
                    return;
                }
                doPlay();
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {

            }
        });
        return this.surfaceView;
    }

    private void doPlay() {
        SimpleThreadPoolExecutor.getPool().submit(new Runnable() {
            @Override
            public void run() {
                try {
                    initial();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        });
    }

    private void initial() throws IOException {
        final Surface surface = holder.getSurface();
        if (surface == null || !surface.isValid()) {
            Log.d(TAG, "initial: surface is invalid");
            return;
        }
        extractor = new MediaExtractor();
        //local path
        if (playUrl.startsWith("/")) {
            FileInputStream stream = new FileInputStream(playUrl);
            FileDescriptor fd = stream.getFD();
            extractor.setDataSource(fd);
        } else {
            extractor.setDataSource(playUrl);
        }
        String mimeType = null;
        for (int i = 0; i < extractor.getTrackCount(); i++) {
            MediaFormat format = extractor.getTrackFormat(i);
            mimeType = format.getString(MediaFormat.KEY_MIME);
            if (mimeType.startsWith("video/")) {
                //找到了视频流
                extractor.selectTrack(i);
                mediaCodec = MediaCodec.createDecoderByType(mimeType);
                mediaCodec.configure(format, surface, null, 0);
                break;
            }
        }
        if (mediaCodec != null) {
            mediaCodec.setCallback(new MediaCallback());
            mediaCodec.start();
        } else {
            Log.e(TAG, "initial: get media codec error!");
            for (int i = 0; i < extractor.getTrackCount(); i++) {
                MediaFormat format = extractor.getTrackFormat(i);
                mimeType = format.getString(MediaFormat.KEY_MIME);
                Log.i(TAG, "initial: " + mimeType);
            }
        }
    }

    @Override
    public boolean play(String url) {
        this.playUrl = url;
        if (holder != null) {
            doPlay();
        }
        return true;
    }

    private void complete(){
        stop();
    }

    @Override
    public boolean stop() {
        if (mediaCodec != null) {
            mediaCodec.stop();
        }
        if (extractor != null) {
            extractor.release();
        }
        return false;
    }

    private class MediaCallback extends MediaCodec.Callback {

        @Override
        public void onInputBufferAvailable(MediaCodec codec, int index) {
            Log.d(TAG, "onInputBufferAvailable: " + index);
//            bufferIdxList.add(index);
            ByteBuffer buffer = mediaCodec.getInputBuffer(index);
            int sampleSize = extractor.readSampleData(buffer, 0);
            if (sampleSize < 0) {
                Log.i(TAG, "receiveFrame: mark input buffer stream end.");
                mediaCodec.queueInputBuffer(index, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
            } else {
                mediaCodec.queueInputBuffer(index, 0, sampleSize, extractor.getSampleTime(), 0);
                extractor.advance();
            }
        }

        @Override
        public void onOutputBufferAvailable(MediaCodec codec, int index, MediaCodec.BufferInfo info) {
            Log.d(TAG, "onOutputBufferAvailable: " + index + " info:" + info.toString());
            ByteBuffer outputBuffer = codec.getOutputBuffer(index);
            MediaFormat outputFormat = codec.getOutputFormat(index);
            if ((info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) {
                outputFormat.setByteBuffer("csd-0", outputBuffer);
                info.size = 0;
            }

            mediaCodec.releaseOutputBuffer(index, true);
            if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                Log.i(TAG, "receiveFrame: catch end of the stream.");
                //mark end of stream.
                complete();
            }
        }

        @Override
        public void onError(MediaCodec codec, MediaCodec.CodecException e) {
            Log.d(TAG, "error: " + e);
        }

        @Override
        public void onOutputFormatChanged(MediaCodec codec, MediaFormat format) {
            Log.d(TAG, "onOutputFormatChanged: " + format.toString());
        }
    }
}
