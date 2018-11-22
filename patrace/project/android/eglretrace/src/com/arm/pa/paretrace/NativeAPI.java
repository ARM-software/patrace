package com.arm.pa.paretrace;

import android.util.Log;
import android.view.Surface;
import android.os.Handler;
import android.os.Message;
import android.os.Bundle;

public class NativeAPI
{
    private static int mWidth;
    private static int mHeight;
    private static int mFormat;
    private static Handler msgHandler;
    private static boolean mHasSurface;
    private static Surface surfHolder;
    private static int mTextureViewSize;
    private static final String TAG = "paretrace NativeAPI";

    public static void setMsgHandler(Handler handler){
        msgHandler  = handler;
    }

    public static void requestNewSurfaceAndWait(int width, int height, int format) {
        Log.i(TAG, "requestNewSurfaceAndWait");
        synchronized(msgHandler) {
            mWidth = width;
            mHeight = height;
            mFormat = format;
            mHasSurface = false;
            surfHolder  = null;
            Message msg = new Message();
            msg.arg1 = width;
            msg.arg2 = height;
            msg.what = 1;
            msgHandler.sendMessage(msg);
            while (!mHasSurface || surfHolder == null) {
                try {
                    msgHandler.wait();
                } catch (InterruptedException ex) {
                    Thread.currentThread().interrupt();
                }
            }
            setSurface(surfHolder, mTextureViewSize);
        }
    }

    public static void resizeNewSurfaceAndWait(int width, int height, int format, int textureViewId) {
        Log.i(TAG, "resizeNewSurfaceAndWait");
        synchronized(msgHandler) {
            mWidth = width;
            mHeight = height;
            mFormat = format;
            mHasSurface = false;
            surfHolder  = null;
            Message msg = new Message();
            msg.arg1 = width;
            msg.arg2 = height;
            Bundle data = new Bundle();
            data.putInt("textureViewId", textureViewId);
            msg.setData(data);
            msg.what = 2;
            msgHandler.sendMessage(msg);
            while (!mHasSurface || surfHolder == null) {
                try {
                    msgHandler.wait();
                } catch (InterruptedException ex) {
                    Thread.currentThread().interrupt();
                }
            }
            setSurface(surfHolder, mTextureViewSize);
        }
    }

    public static void onSurfaceChanged(Surface holder, int width, int height, int textureViewSize) {
        synchronized(msgHandler) {
            Log.i(TAG, "onSurfaceChanged " + holder + " " + width + " " + height);
            surfHolder = holder;
            mHasSurface= true;
            mTextureViewSize = textureViewSize;
            msgHandler.notifyAll();
        }
    }

    public static native void init(boolean registerEntries);
    public static native boolean initFromJson(String jsonData, String traceDir, String resultFile);
    public static native void setSurface(Surface surface, int textureViewSize);
    public static native boolean step();
    public static native void stop();
    public static native boolean opt_getIsPortraitMode();

    static {
        System.loadLibrary("eglretrace");
    }
}
