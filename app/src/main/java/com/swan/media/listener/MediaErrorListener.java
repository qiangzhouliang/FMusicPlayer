package com.swan.media.listener;

/**
 * @ClassName MediaErrorListener
 * @Description 错误回调
 * @Author swan
 * @Date 2023/10/10 16:47
 **/
public interface MediaErrorListener {
    void onError(int code, String msg);
}
