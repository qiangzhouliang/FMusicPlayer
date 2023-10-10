package com.swan.fmusicplayer;

import android.os.Bundle;
import android.util.Log;

import androidx.appcompat.app.AppCompatActivity;

import com.swan.fmusicplayer.databinding.ActivityMainBinding;
import com.swan.media.SwanPlayer;

import java.io.File;

public class MainActivity extends AppCompatActivity {
    private String TAG = "SWAN";
    private ActivityMainBinding binding;
    private SwanPlayer swanPlayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        swanPlayer = new SwanPlayer();
        // 输出 FFmpeg 版本信息
        Log.i(TAG, "FFmpeg Version："+swanPlayer.stringFromJNI());

        playMusic();
    }

    private void playMusic(){
        File mMusicFile = new File(getApplication().getFilesDir().getPath()+"/test1.mp3");
        swanPlayer.setDataSource(mMusicFile.getAbsolutePath());
        swanPlayer.setOnErrorListener((code, msg) -> {
            Log.e(TAG, "error code:"+code+"  ==>  error msg:"+msg);
            // java的逻辑代码
        });
        swanPlayer.play();
    }

}