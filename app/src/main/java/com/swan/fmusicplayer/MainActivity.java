package com.swan.fmusicplayer;

import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.View;

import androidx.appcompat.app.AppCompatActivity;

import com.swan.fmusicplayer.databinding.ActivityMainBinding;
import com.swan.media.SwanPlayer;
import com.swan.media.SwanVideoView;

import java.io.File;

public class MainActivity extends AppCompatActivity {
    private String TAG = "SWAN";
    private ActivityMainBinding binding;
    private SwanPlayer swanPlayer;
    private SwanVideoView mVideoView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());
        mVideoView = binding.videoView;

        //playMusic();
    }

    private void playMusic(){
        swanPlayer = new SwanPlayer();
        // 输出 FFmpeg 版本信息
        Log.i(TAG, "FFmpeg Version："+swanPlayer.stringFromJNI());
        File mMusicFile = new File(getApplication().getFilesDir().getPath()+"/test.mp3");
        swanPlayer.setDataSource(mMusicFile.getAbsolutePath());
        swanPlayer.setOnErrorListener((code, msg) -> {
            Log.e(TAG, "error code:"+code+"  ==>  error msg:"+msg);
            // java的逻辑代码
        });
        swanPlayer.setOnPreparedListener(() -> {
            Log.e(TAG, "准备完毕");
            swanPlayer.play();
        });
        swanPlayer.prepareAsync();
    }

    public void play(View view) {
        File mVideoFile = new File(getApplication().getFilesDir().getPath()+"/Forrest_Gump_IMAX.mp4");
        //File mMusicFile = new File(getApplication().getFilesDir().getPath()+"/test.mp3");
        mVideoView.player(mVideoFile.getPath());
        decodeVideo(mVideoFile.getPath(), mVideoView.getHolder().getSurface());
    }

    private native void decodeVideo(String uri, Surface surface);
}