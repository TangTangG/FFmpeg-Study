package com.player.ffmpegdemo;

import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.view.ViewGroup;

//import com.gu.ffmpeg_surface.FFSurfacePlayer;
import com.gu.player.SimplePlayerIface;
import com.player.ffmpeg_opensles_surface.FFSurfaceOpenslESPlayer;

import java.io.File;

public class MainActivity extends AppCompatActivity {


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
        final String path = new File(Environment.getExternalStorageDirectory(),"demo.mp4").getAbsolutePath();

//        SimplePlayerIface ffPlayer = new FFDecodePlayer();
        SimplePlayerIface ffPlayer = new FFSurfaceOpenslESPlayer(MainActivity.this);
        View view = ffPlayer.init(null);
        ViewGroup container = findViewById(R.id.surface_container);
        container.addView(view);
        ffPlayer.play(path);
    }


}
