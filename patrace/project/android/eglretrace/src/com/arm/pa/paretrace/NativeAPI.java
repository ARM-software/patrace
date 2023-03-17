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
    private static int mViewSize;
    private static final String TAG = "paretrace NativeAPI";

    public static void setMsgHandler(Handler handler){
        msgHandler  = handler;
    }

    public static void requestNewSurfaceAndWait(int width, int height, int format, int win) {
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
            Bundle data = new Bundle();
            data.putInt("win", win);
            msg.setData(data);
            msg.what = 1;
            msgHandler.sendMessage(msg);
            while (!mHasSurface || surfHolder == null) {
                try {
                    msgHandler.wait();
                } catch (InterruptedException ex) {
                    Thread.currentThread().interrupt();
                }
            }
            setSurface(surfHolder, mViewSize);
        }
    }

    public static void resizeNewSurfaceAndWait(int width, int height, int format, int win) {
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
            data.putInt("win", win);
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
            setSurface(surfHolder, mViewSize);
        }
    }

    public static void destroySurfaceAndWait(int win) {
        Log.i(TAG, "destroySurfaceAndWait");
        synchronized(msgHandler) {
            mHasSurface = true;
            Message msg = new Message();
            msg.arg1 = 0;
            msg.arg2 = 0;
            Bundle data = new Bundle();
            data.putInt("win", win);
            msg.setData(data);
            msg.what = 3;
            msgHandler.sendMessage(msg);
            while (mHasSurface && surfHolder != null) {
                try {
                    msgHandler.wait();
                } catch (InterruptedException ex) {
                    Thread.currentThread().interrupt();
                }
            }
        }
    }

    public static void onSurfaceChanged(Surface holder, int width, int height, int viewSize) {
        synchronized(msgHandler) {
            Log.i(TAG, "onSurfaceChanged " + holder + " " + width + " " + height);
            surfHolder = holder;
            mHasSurface= true;
            mViewSize = viewSize;
            msgHandler.notifyAll();
        }
    }

    public static void onSurfaceDestroyed(Surface holder, int viewSize) {
        synchronized(msgHandler) {
            Log.i(TAG, "onSurfaceDestroyed " + holder);
            setSurface(holder, 0);
            surfHolder = null;
            mHasSurface= false;
            msgHandler.notifyAll();
        }
    }

    public static native boolean launchFastforward(String[] argArray);
    public static native void init(boolean registerEntries);
    public static native boolean initFromJson(String jsonData, String traceDir, String resultFile);
    public static native void setSurface(Surface surface, int viewSize);
    public static native boolean step();
    public static native void stop(String result);
    public static native boolean opt_getIsPortraitMode();
    public static native void stepframe1();
    public static native void stepframe10();
    public static native void stepframe100();
    public static native void stepdraw1();
    public static native void stepmodefinish();

    static {
        System.loadLibrary("eglretrace");
    }
}
