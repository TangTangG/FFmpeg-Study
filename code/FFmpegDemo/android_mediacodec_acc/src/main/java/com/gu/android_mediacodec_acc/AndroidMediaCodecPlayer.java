package com.gu.android_mediacodec_acc;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.res.AssetFileDescriptor;
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

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

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
        try {
            initial();
        } catch (IOException e) {
            e.printStackTrace();
        }
        SimpleThreadPoolExecutor.getPool().submit(new Runnable() {
            @Override
            public void run() {
                receiveFrame();
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
//            FileOutputStream file = new FileOutputStream(playUrl);
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

    private void receiveFrame() {
        if (mediaCodec == null) {
            return;
        }
        ByteBuffer[] inputBuffers = mediaCodec.getInputBuffers();
        ByteBuffer[] outputBuffers = mediaCodec.getOutputBuffers();
        MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
        while (!Thread.interrupted()) {
            int inputIndex = mediaCodec.dequeueInputBuffer(0);
            if (inputIndex >= 0) {
                ByteBuffer buffer = inputBuffers[inputIndex];
                int sampleSize = extractor.readSampleData(buffer, 0);
                if (sampleSize < 0) {
                    Log.i(TAG, "receiveFrame: mark input buffer stream end.");
                    mediaCodec.queueInputBuffer(inputIndex, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                } else {
                    mediaCodec.queueInputBuffer(inputIndex, 0, sampleSize, extractor.getSampleTime(), 0);
                    extractor.advance();
                }
            }
            int outIndex = mediaCodec.dequeueOutputBuffer(bufferInfo, 10);
            switch (outIndex) {
                case MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED:
                    outputBuffers = mediaCodec.getOutputBuffers();
                    break;
                case MediaCodec.INFO_OUTPUT_FORMAT_CHANGED:
                    Log.d(TAG, "New format " + mediaCodec.getOutputFormat());
                    break;
                case MediaCodec.INFO_TRY_AGAIN_LATER:
                    Log.d(TAG, "dequeueOutputBuffer timed out!");
                    break;
                default:
                    ByteBuffer buffer = outputBuffers[outIndex];
                    Log.v(TAG, "We can't use this buffer but render it due to the API limit, " + buffer);
                    mediaCodec.releaseOutputBuffer(outIndex, true);
                    break;
            }
            if ((bufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                Log.i(TAG, "receiveFrame: catch end of the stream.");
                break;
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
        }

        @Override
        public void onOutputBufferAvailable(MediaCodec codec, int index, MediaCodec.BufferInfo info) {
            Log.d(TAG, "onOutputBufferAvailable: " + index + " info:" + info.toString());
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
