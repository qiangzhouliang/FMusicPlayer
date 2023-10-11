package com.swan.media;

import android.text.TextUtils;

import com.swan.media.listener.MediaErrorListener;
import com.swan.media.listener.MediaPreparedListener;

/**
 * @ClassName SwanPlayer
 * @Description 音频播放器逻辑处理类
 * @Author swan
 * @Date 2023/10/10 10:14
 **/
public class SwanPlayer {
    // Used to load the 'fmusicplayer' library on application startup.
    static {
        System.loadLibrary("fmusicplayer");

    }

    /**
     * url 可以是本地文件路径，也可以是 http链接
     */
    private String url;
    private MediaErrorListener mErrorListener;
    private MediaPreparedListener mPreparedListener;

    public void setOnErrorListener(MediaErrorListener mErrorListener) {
        this.mErrorListener = mErrorListener;
    }

    public void setOnPreparedListener(MediaPreparedListener mPreparedListener) {
        this.mPreparedListener = mPreparedListener;
    }

    /**
     * called from jni
     * @param code
     * @param msg
     */
    private void onError(int code, String msg){
        if (mErrorListener != null){
            mErrorListener.onError(code, msg);
        }
    }

    private void onPrepared(){
        if (mPreparedListener != null){
            mPreparedListener.onPrepared();
        }
    }

    public void setDataSource(String url){
        this.url = url;
    }

    public void play(){
        nPlay();
    }

    private native void nPlay();

    public native String stringFromJNI();

    public void prepare(){
        if (TextUtils.isEmpty(url)){
            throw new NullPointerException("url is null, please call method serDataSource");
        }
        nPrepare(url);
    }

    public void prepareAsync(){
        if (TextUtils.isEmpty(url)){
            throw new NullPointerException("url is null, please call method serDataSource");
        }
        nPrepareAsync(url);
    }

    private native void nPrepare(String url);
    private native void nPrepareAsync(String url);
}
