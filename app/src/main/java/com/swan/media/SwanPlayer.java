package com.swan.media;

import android.text.TextUtils;

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
