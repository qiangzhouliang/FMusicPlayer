package com.swan.media;

import android.text.TextUtils;

import com.swan.media.listener.MediaErrorListener;

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

    public void setOnErrorListener(MediaErrorListener mErrorListener) {
        this.mErrorListener = mErrorListener;
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

    public void setDataSource(String url){
        this.url = url;
    }

    public void play(){
        if (TextUtils.isEmpty(url)){
            throw new NullPointerException("url is null, please call method serDataSource");
        }
        nPlay(url);
    }

    private native void nPlay(String url);

    public native String stringFromJNI();
}
