package com.arm.pa.paretrace.Activities;

import java.util.HashMap;
import java.lang.Integer;

import android.app.Activity;
import android.content.Intent;
import android.content.Context;
import android.os.Bundle;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.HorizontalScrollView;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.TextureView;
import android.graphics.SurfaceTexture;
import android.view.WindowManager;
import android.view.KeyEvent;
import android.os.Handler;
import android.os.Message;

import com.arm.pa.paretrace.NativeAPI;
import com.arm.pa.paretrace.R;
import com.arm.pa.paretrace.Retracer.GLThread;

public class RetraceActivity extends Activity
{
    private static final String TAG = "paretrace RetraceActivity";
    private Handler msgHandler = new Handler() {
                @Override
                public void handleMessage(Message msg) {
                    super.handleMessage(msg);
                    switch (msg.what) {
                        case 1:
                            addNewView(msg.arg1, msg.arg2, msg.getData().getInt("win"));
                            break;
                        case 2:
                            resizeView(msg.arg1, msg.arg2, msg.getData().getInt("win"));
                            break;
                        case 3:
                            removeOneView(msg.getData().getInt("win"));
                            break;
                        default:
                            break;
                    }
                }
            };
    private NativeAPI NativeAPI;
    private HashMap<Integer, View> viewIdToViewMap = new HashMap<Integer, View>();
    private GLThread mGLThread = null;
    private GLViewContainer mViewContainer = null;
    private HorizontalScrollView scroll = null;

    private boolean force_single_window = true;
    private boolean enOverlay = true;
    private int transparent = 100;
    private boolean enFullScreen = false;
    private int oldVisibility = View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN;
    WakeLock mWl = null;

    @Override protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        Intent parentIntent = getIntent();
        force_single_window = parentIntent.getBooleanExtra("force_single_window", true);
        enOverlay = force_single_window? false : parentIntent.getBooleanExtra("enOverlay", true);
        transparent = parentIntent.getIntExtra("transparent", 100);
        enFullScreen = parentIntent.getBooleanExtra("enFullScreen", false);

        if (enFullScreen)
        {
            oldVisibility = getVisibility();
            hideSystemUI();
        }

        Log.i(TAG, "PARetrace Version: " + getString(R.string.version));

        // We use TextureView to handle multiview cases
        // TextureView can only be used with hardware acceleration enabled
        if (!force_single_window)
        {
            getWindow().setFlags(WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED,
                                 WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED);
        }

        mViewContainer = new GLViewContainer(this);
        scroll = new HorizontalScrollView(this);
        scroll.addView(mViewContainer);
        setContentView(scroll);
        super.onLowMemory();
    }

    private int getVisibility() {
        return getWindow().getDecorView().getSystemUiVisibility();
    }

    private void hideSystemUI() {
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION // hide nav bar
            | View.SYSTEM_UI_FLAG_FULLSCREEN // hide status bar
            | View.SYSTEM_UI_FLAG_IMMERSIVE);
    }

    private void showSystemUI() {
        getWindow().getDecorView().setSystemUiVisibility(oldVisibility);
    }

    private void addNewView(int width, int height, int win) {
        if (force_single_window) {
            GLSurfaceView view = new GLSurfaceView(this, width, height);
            Log.i(TAG, "Create a new surface view:" + " width:" + width + " height:" + height);
            mViewContainer.addView(view, view.getLayout());
            viewIdToViewMap.put(win, view);
        }
        else if (mViewContainer != null) {
            GLTextureView view = new GLTextureView(this, width, height);
            Log.i(TAG, "Create a new texture view:" + " width:" + width + " height:" + height);
            mViewContainer.addView(view, view.getLayout());
            viewIdToViewMap.put(win, view);
        }
        else {
            Log.w(TAG, "View container has been destroyed!");
        }
    }

    private void resizeView(int width, int height, int win) {
        if (mViewContainer != null) {
            View view = viewIdToViewMap.get(win);
            LayoutParams params = view.getLayoutParams();
            params.width = width;
            params.height = height;
            view.setLayoutParams(params);
            Log.i(TAG, "Resize a view:" + " width:" + width + " height:" + height);
        }
        else {
            Log.w(TAG, "View container has been destroyed!");
        }
    }

    private void removeOneView(int win) {
        if (mViewContainer != null)
            {
                Log.i(TAG, "Remove one view");
                View view = viewIdToViewMap.get(win);
                mViewContainer.removeView(view);
                viewIdToViewMap.remove(win);
            }
        else
            Log.w(TAG, "View container has been destroyed!");
    }

    public int viewContainerSize() {
        return mViewContainer.getChildCount();
    }

    @Override public void onLowMemory() {
        Log.w(TAG, "Received low memory warning!");
        super.onLowMemory();
    }

    @Override protected void onStart() {
        super.onStart();
        Log.i(TAG, "RetraceActivity: onStart()");
    }

    @Override protected void onResume() {
        super.onResume();

        Log.i(TAG, "RetraceActivity: onResume()");

        // release old wakelock if not released
        if (mWl != null && mWl.isHeld()) {
            mWl.release();
            mWl = null;
        }

        // Turn on screen / keep screen awake
        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        mWl = pm.newWakeLock((PowerManager.SCREEN_BRIGHT_WAKE_LOCK | PowerManager.FULL_WAKE_LOCK | PowerManager.ACQUIRE_CAUSES_WAKEUP), TAG);
        mWl.acquire();

        if (mViewContainer.getChildCount() > 0) {
            mViewContainer.removeAllViews();
        }

        mGLThread = new GLThread(this, msgHandler);
        mGLThread.start();
    }

    @Override protected void onPause() {
        super.onPause();
        Log.i(TAG, "RetraceActivity: onPause()");

        if (mGLThread != null) {
            mGLThread.requestExitAndWait();
        }

        // Release wakelock
        if (mWl != null && mWl.isHeld()) {
            mWl.release();
        }
    }

    @Override protected void onDestroy() {
        super.onDestroy();
        Log.i(TAG, "RetraceActivity: onDestroy()");
        boolean isGui = getIntent().getBooleanExtra("isGui", false);
        if(!isGui) {
            /* Create a thread which will pause and then call exit */
            Thread suicideThread = new Thread() {
                public void run() {
                    try {
                        sleep(500);
                    }
                    catch (InterruptedException ie){
                    }
                    System.exit(0);
                }
            };
            suicideThread.start();
        }
        if (enFullScreen)
        {
            showSystemUI();
        }

		mGLThread = null;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_N:
                NativeAPI.stepframe1();
                Log.i(TAG,"step frame 1");
                return true;
            case KeyEvent.KEYCODE_M:
                NativeAPI.stepframe10();
                Log.i(TAG,"step frame 10");
                return true;
            case KeyEvent.KEYCODE_L:
                NativeAPI.stepframe100();
                Log.i(TAG,"step frame 100");
                return true;
            case KeyEvent.KEYCODE_D:
                NativeAPI.stepdraw1();
                Log.i(TAG,"step draw 1");
                return true;
            case KeyEvent.KEYCODE_Q:
                NativeAPI.stepmodefinish();
                Log.i(TAG,"step finish");
                return true;
            default:
                Log.i(TAG,"step invalid keyevent");
                return super.onKeyUp(keyCode, event);
        }
    }

    private class GLSurfaceView extends SurfaceView implements SurfaceHolder.Callback {
        private int width;
        private int height;

        public GLSurfaceView(Context context, int w, int h) {
            super(context);

            width  = w;
            height = h;

            SurfaceHolder holder = getHolder();
            holder.addCallback(this);
        }

        public ViewGroup.LayoutParams getLayout() {
            return new ViewGroup.LayoutParams(width, height);
        }

        @Override public void surfaceCreated(SurfaceHolder holder) {
            Surface surface = holder.getSurface();
            Log.i(TAG, "SurfaceHolder.Callback: surfaceCreated:" + surface);
            mGLThread.setViewSize(mViewContainer.getChildCount());
        }

        @Override public void surfaceDestroyed(SurfaceHolder holder) {
            Surface surface = holder.getSurface();
            Log.i(TAG, "SurfaceHolder.Callback: surfaceDestroyed:" + surface);
            if (mGLThread != null) {
                mGLThread.onWindowDestroyed(surface);
            }
        }

        @Override public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
            Surface surface = holder.getSurface();
            Log.i(TAG, "SurfaceHolder.Callback: surfaceChanged:" + surface + " " + w + " " + h);
            if (mGLThread != null) {
                mGLThread.setViewSize(mViewContainer.getChildCount());
                mGLThread.onWindowResize(surface, w, h);
            }
        }
    }

    private class GLTextureView extends TextureView implements TextureView.SurfaceTextureListener {
        private int width;
        private int height;

        public GLTextureView(Context context, int w, int h) {
            super(context);

            width  = w;
            height = h;

            if (enOverlay)
            {
                if (transparent > 100)
                    transparent = 100;
                float alpha = (float)transparent/100.0f;
                setAlpha(alpha);
            }
            else
            {
                setAlpha(1.0f);
            }

            setSurfaceTextureListener(this);
        }

        public ViewGroup.LayoutParams getLayout() {
            return new ViewGroup.LayoutParams(width, height);
        }

        public void onSurfaceTextureAvailable(SurfaceTexture surface, int w, int h) {
            Log.i(TAG, "SurfaceTextureListener: surfaceCreated:" + surface);
            if (mGLThread != null) {
                mGLThread.setViewSize(mViewContainer.getChildCount());
                mGLThread.onWindowResize(surface, w, h);
            }
        }

        public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
            Log.i(TAG, "SurfaceTextureListener: surfaceDestroyed:" + surface);
            if (mGLThread != null) {
                mGLThread.onWindowDestroyed(surface);
            }
            return true;
        }

        public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int w, int h) {
            Log.i(TAG, "SurfaceTextureListener: surfaceSizeChanged:" + surface + " " + w + " " + h);
            if (mGLThread != null) {
                mGLThread.setViewSize(mViewContainer.getChildCount());
                mGLThread.onWindowResize(surface, w, h);
            }
            width = w;
            height = h;
        }

        public void onSurfaceTextureUpdated(SurfaceTexture surface) {
            if (enOverlay) {
                bringToFront();
            }
        }
    }

    private class GLViewContainer extends ViewGroup {
        View maxView = null;
        int maxWidth  =0;
        int maxHeight = 0;
        int maxArea = 0;

        public GLViewContainer(Context context) {
            super(context);
        }

        @Override protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            super.onMeasure(widthMeasureSpec, heightMeasureSpec);
            maxWidth  =0;
            maxHeight = 0;
            maxArea = 0;

            measureChildren(widthMeasureSpec, heightMeasureSpec);
            if (getChildCount() == 0) {
                setMeasuredDimension(0 ,0);
            }
            else {
                for (int i = 0; i < getChildCount(); i++) {
                    View child = getChildAt(i);
                    int width = child.getMeasuredWidth();
                    int height = child.getMeasuredHeight();
                    int area = width * height;
                    if (enOverlay || force_single_window)
                    {
                        if (width > maxWidth) {
                            maxWidth = width;
                        }
                    }
                    else
                    {
                        maxWidth += width;
                    }
                    if (height > maxHeight) {
                        maxHeight = height;
                    }
                    if (area >= maxArea) {
                        maxArea = area;
                        maxView = child;
                    }
                }
                setMeasuredDimension(maxWidth, maxHeight);
            }
        }

        @Override protected void onLayout(boolean changed, int l, int t, int r, int b) {
            int totalWidth = 0;
            for (int i = 0; i < getChildCount(); i++) {
                View child = getChildAt(i);
                if (child.getVisibility() == GONE) continue;
                int width = child.getMeasuredWidth();
                int height = child.getMeasuredHeight();
                child.layout(totalWidth, 0, totalWidth + width, height);
                if (!enOverlay && !force_single_window)
                {
                    totalWidth += width;
                }
            }
        }
    }
}
