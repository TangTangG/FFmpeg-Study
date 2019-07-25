package com.player.ffmpegdemo;

import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

//import com.gu.ffmpeg_surface.FFSurfacePlayer;
import com.gu.player.SimplePlayerIface;
import com.player.ffmpeg_opensles_surface.FFSurfaceOpenslESPlayer;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    private SimplePlayerIface ffPlayer = null;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        findViewById(R.id.play_demo)
                .setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        play();
                    }
                });
    }

    private void play() {
        final String path = new File(Environment.getExternalStorageDirectory(), "demo.mp4").getAbsolutePath();

//        SimplePlayerIface ffPlayer = new FFDecodePlayer();
        if (ffPlayer == null) {
            ffPlayer = new FFSurfaceOpenslESPlayer(MainActivity.this);
            View view = ffPlayer.init(null);
            ViewGroup container = findViewById(R.id.surface_container);
            int height = container.getMeasuredHeight();
            int width = container.getMeasuredWidth();
            if (width >= height) {
                width = 16 * height / 9;
            } else {
                height = width * 9 / 16;
            }
            FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(width, height);
            view.setLayoutParams(params);
            container.addView(view);
        }
        ffPlayer.play(path);
    }


}
