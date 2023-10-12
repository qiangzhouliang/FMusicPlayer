package com.swan.media;

import android.content.Context;
import android.graphics.PixelFormat;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.swan.media.listener.MediaPreparedListener;

/**
 * @ClassName SwanVideoView
 * @Description
 * @Author swan
 * @Date 2023/10/12 15:17
 **/
public class SwanVideoView extends SurfaceView implements MediaPreparedListener {
    private SwanPlayer mPlayer;
    public SwanVideoView(Context context) {
        this(context,null);
    }

    public SwanVideoView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public SwanVideoView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        SurfaceHolder holder = getHolder();
        // 设置显示的像素格式
        holder.setFormat(PixelFormat.RGBA_8888);

        mPlayer = new SwanPlayer();
        mPlayer.setOnPreparedListener(this);
    }

    public void player(String uri){
        stop();
        mPlayer.setDataSource(uri);
        mPlayer.prepareAsync();
    }

    /**
     * 停止方法
     */
    private void stop() {

    }

    @Override
    public void onPrepared() {
        mPlayer.play();
    }
}
