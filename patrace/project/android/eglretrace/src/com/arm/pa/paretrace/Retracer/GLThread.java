package com.arm.pa.paretrace.Retracer;

import java.io.BufferedReader;
import java.io.FileReader;
import java.util.HashMap;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.graphics.SurfaceTexture;
import android.view.Surface;
import android.util.Log;
import android.os.Handler;

import com.arm.pa.paretrace.NativeAPI;
import com.arm.pa.paretrace.R;
import com.arm.pa.paretrace.Util.Util;

import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.os.BatteryManager;
import android.os.SystemClock;

public class GLThread extends Thread {

    private static final String TAG = "paretrace GLThread";
    private JSONArray voltageinfo;
    private Intent powerintent;
    private Activity mRetraceActivity;

    class CheckBatteryStatusReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent != null){
                BatteryManager mBatteryManager = (BatteryManager)mRetraceActivity.getSystemService(Context.BATTERY_SERVICE);
                double level = intent.getIntExtra(BatteryManager.EXTRA_VOLTAGE, -1);
                double timestamp = (double)SystemClock.elapsedRealtime() / 1000.0;
                double current_now = mBatteryManager.getLongProperty(BatteryManager.BATTERY_PROPERTY_CURRENT_NOW);
                double current_avg = mBatteryManager.getLongProperty(BatteryManager.BATTERY_PROPERTY_CURRENT_AVERAGE);
                try {
                    JSONArray pair = new JSONArray();
                    pair.put(timestamp);
                    pair.put(level);
                    pair.put(current_now);
                    pair.put(current_avg);
                    voltageinfo.put(pair);
                } catch (JSONException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    public static boolean isRunning;
    // properties
    private NativeAPI NativeAPI;

    // states
    private boolean mShouldExit;
    private boolean mExited;

    private int mViewSize = 0;
    private HashMap<SurfaceTexture, Surface> surfaceTexToSurfaceMap = new HashMap<SurfaceTexture, Surface>();

    private CheckBatteryStatusReceiver batteryStatusReceiver;

    public GLThread(Context ctx, Handler handler) {
        super();
        voltageinfo = new JSONArray();
        mRetraceActivity = (Activity) ctx;
        NativeAPI.setMsgHandler(handler);
        IntentFilter powerfilter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
        powerintent = mRetraceActivity.registerReceiver(null, powerfilter);

        batteryStatusReceiver = new CheckBatteryStatusReceiver();
        mRetraceActivity.registerReceiver(batteryStatusReceiver, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
    }


    public void requestExitAndWait() {
        Log.i(TAG, "requestExitAndWait");
        synchronized(this) {
            mShouldExit = true;
            notifyAll();
            while (!mExited) {
                try {
                    wait();
                } catch (InterruptedException ex) {
                    Thread.currentThread().interrupt();
                }
            }
            mRetraceActivity.unregisterReceiver(batteryStatusReceiver);
        }
    }

    public void setViewSize(int size) {
        mViewSize = size;
    }

    ////////////////////////////////////////////////////////////
    // Event handlers
    public void onWindowResize(SurfaceTexture surface, int w, int h) {
        synchronized(this) {
            Log.i(TAG, "onWindowResize " + surface + " " + w + " " + h);
            Surface result;
            if (surfaceTexToSurfaceMap.containsKey(surface)) {
                result = surfaceTexToSurfaceMap.get(surface);
            }
            else {
                result = new Surface(surface);
                surfaceTexToSurfaceMap.put(surface, result);
            }
            NativeAPI.onSurfaceChanged(result, w, h, mViewSize);
            notifyAll();
        }
    }

    public void onWindowResize(Surface surface, int w, int h) {
        synchronized(this) {
            Log.i(TAG, "onWindowResize " + surface + " " + w + " " + h);
            NativeAPI.onSurfaceChanged(surface, w, h, mViewSize);
            notifyAll();
        }
    }

    public void onWindowDestroyed(SurfaceTexture surface) {
        synchronized(this) {
            Log.i(TAG, "onWindowDestroyed " + surface);
            Surface result;
            if (surfaceTexToSurfaceMap.containsKey(surface)) {
                result = surfaceTexToSurfaceMap.get(surface);
            }
            else {
                result = new Surface(surface);
                surfaceTexToSurfaceMap.put(surface, result);
            }
            NativeAPI.onSurfaceDestroyed(result, mViewSize);
            notifyAll();
        }
    }

    public void onWindowDestroyed(Surface surface) {
        synchronized(this) {
            Log.i(TAG, "onWindowDestroyed " + surface);
            NativeAPI.onSurfaceDestroyed(surface, 0);
            notifyAll();
        }
    }

    private JSONObject getdata() {
        JSONObject js = new JSONObject();
        try {
            js.put("phone_manufacturer", android.os.Build.MANUFACTURER);
            js.put("phone_model", android.os.Build.MODEL);
            js.put("android_version", android.os.Build.VERSION.SDK_INT);
            js.put("android_release", android.os.Build.VERSION.RELEASE);
        }
        catch (Exception e)
        {
            Log.i(TAG, "Failed to generate JSON data!");
            e.printStackTrace();
        }
        return js;
    }

    private JSONObject batterydata() {
        JSONObject js = new JSONObject();
        int battery_voltage = powerintent.getIntExtra(BatteryManager.EXTRA_VOLTAGE, -1);
        int battery_plugged = powerintent.getIntExtra(BatteryManager.EXTRA_PLUGGED, -1);
        int battery_temp = powerintent.getIntExtra(BatteryManager.EXTRA_TEMPERATURE, -1);
        BatteryManager mBatteryManager = (BatteryManager)mRetraceActivity.getSystemService(Context.BATTERY_SERVICE);
        long energy = mBatteryManager.getLongProperty(BatteryManager.BATTERY_PROPERTY_ENERGY_COUNTER); // Remaining energy in nanowatt-hours
        long current_now = mBatteryManager.getLongProperty(BatteryManager.BATTERY_PROPERTY_CURRENT_NOW); // Instantaneous battery current in microamperes
        long current_avg = mBatteryManager.getLongProperty(BatteryManager.BATTERY_PROPERTY_CURRENT_AVERAGE); // Average battery current in microamperes
        long capacity = mBatteryManager.getLongProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY); // Remaining battery capacity as an integer percentage
        long charge = mBatteryManager.getLongProperty(BatteryManager.BATTERY_PROPERTY_CHARGE_COUNTER); // Remaining battery capacity in microampere-hours
        if (battery_voltage < 1000) battery_voltage *= 1000;
        try {
            js.put("battery_remaining", capacity);
            js.put("battery_plugged", battery_plugged);
            js.put("battery_temperature", battery_temp);
            js.put("current_average", current_avg);
            js.put("current_now", current_now);
            js.put("voltage_now", battery_voltage);
            js.put("battery_charge", charge);
            js.put("battery_capacity", capacity);
        }
        catch (Exception e)
        {
            Log.i(TAG, "Failed to generate JSON data!");
            e.printStackTrace();
        }
        return js;
    }

    ////////////////////////////////////////////////////////////
    // Thread main
    @Override
    public void run() {
        setName("GLThread " + getId());
        Log.i(TAG, "starting tid=" + getId());

        try {
            guardedRun();
        } catch (InterruptedException e) {
            // fall through and exit normally
            Log.w(TAG, "exception in GLThead.run", e);
        } finally {
            synchronized(this) {
                Log.d(TAG, "About to terminate...");
                //mEglHelper.terminate();

                mExited = true;
                notifyAll();
            }
        }

        isRunning = false;
        mRetraceActivity.finish();
    }

    /**
     * @throws InterruptedException
     */
    public void guardedRun() throws InterruptedException {
        synchronized(this) {
            if (mShouldExit) {
                return;
            }
        }

        isRunning = true;

        Intent parentIntent = mRetraceActivity.getIntent();

        String resultFile = parentIntent.getStringExtra("resultFile");

        String json_data = new String();
        String trace_file_path = new String();

        JSONObject battery_before = batterydata();

        if (parentIntent.hasExtra("jsonData")) { // JSON format
            trace_file_path = Util.getTraceFilePath();
            if (parentIntent.hasExtra("traceFilePath")){
                trace_file_path = parentIntent.getStringExtra("traceFilePath");
            }

            String json_extra = parentIntent.getStringExtra("jsonData");

            // check if json_extra is a file path, and if so, read JSON data from the file
            if(json_extra.charAt(0) == '/' || Character.isLetter(json_extra.charAt(0))) {
                try {
                    BufferedReader r = new BufferedReader(new FileReader(json_extra));
                    StringBuilder total = new StringBuilder();
                    String line;
                    while ((line = r.readLine()) != null) {
                        total.append(line);
                    }
                    r.close();
                    json_data = total.toString();
                } catch (Exception e) {
                    json_data = ""; // shut up the compiler
                    e.printStackTrace();
                    System.exit(1);
                }
            } else {
                json_data = json_extra;
            }
        } else {
            JSONObject js = new JSONObject();
            trace_file_path = parentIntent.getStringExtra("fileName");
            try {
                if (parentIntent.hasExtra("callset")) {
                    js.put("snapshotCallset", parentIntent.getStringExtra("callset"));
                    js.put("snapshotPrefix", parentIntent.getStringExtra("callsetprefix"));
                }
                js.put("callStats", parentIntent.getBooleanExtra("callstats", false));
                js.put("statelog", parentIntent.getBooleanExtra("statelog", false));
                if (parentIntent.hasExtra("cpumask")) {
                    js.put("cpumask", parentIntent.getStringExtra("cpumask"));
                }
                js.put("drawlog", parentIntent.getBooleanExtra("drawlog", false));
                if (parentIntent.hasExtra("multithread")) {
                    js.put("multithread", parentIntent.getBooleanExtra("multithread", false));
                }
                if (parentIntent.hasExtra("force_single_window")) {
                    js.put("forceSingleWindow", parentIntent.getBooleanExtra("force_single_window", false));
                }
                js.put("file", parentIntent.getStringExtra("fileName") );
                js.put("threadId", parentIntent.getIntExtra("tid", -1) ); // default in hdr
                js.put("width", parentIntent.getIntExtra("winW", -1) ); // default in hdr
                js.put("height", parentIntent.getIntExtra("winH", -1) ); // default in hdr
                js.put("overrideWidth", parentIntent.getIntExtra("oresW", -1) ); // default off
                js.put("overrideHeight", parentIntent.getIntExtra("oresH", -1) ); // default off
                if ( parentIntent.getIntExtra("oresW", -1) > 0 && parentIntent.getIntExtra("oresH", -1) > 0) {
                    js.put("overrideResolution", true ); // default off
                }

                int frame_start = parentIntent.getIntExtra("frame_start", 1); // default off
                int frame_end = parentIntent.getIntExtra("frame_end", 9999999); // default off
                String frames = ""+frame_start+"-"+frame_end;
                js.put("frames", frames);

                if (parentIntent.hasExtra("snapshotPrefix")) {
                    js.put("snapshotPrefix", parentIntent.getStringExtra("snapshotPrefix") ); // default off
                }

                if (parentIntent.hasExtra("snapshotCallset")) {
                    js.put("snapshotCallset", parentIntent.getStringExtra("snapshotCallset") ); // default off
                }

                if (parentIntent.hasExtra("perfstart") && parentIntent.hasExtra("perfend")) {
                    int perf_start = parentIntent.getIntExtra("perfstart", -1);
                    int perf_end = parentIntent.getIntExtra("perfend", -1);
                    String perfrange = ""+perf_start+"-"+perf_end;
                    js.put("perfrange",perfrange);
                }
                if(parentIntent.hasExtra("perfpath")){
                    js.put("perfpath", parentIntent.getStringExtra("perfpath"));
                }
                if(parentIntent.hasExtra("perffreq")){
                    js.put("perffreq", parentIntent.getIntExtra("perffreq", 1000));
                }
                if(parentIntent.hasExtra("perfout")){
                    js.put("perfout", parentIntent.getStringExtra("perfout"));
                }

                js.put("preload", parentIntent.getBooleanExtra("preload", false));
                js.put("singlesurface", parentIntent.getIntExtra("singlesurface", -1));
                js.put("noscreen", parentIntent.getBooleanExtra("noscreen", false));
                js.put("offscreen", parentIntent.getBooleanExtra("forceOffscreen", false));
                js.put("offscreenSingleTile", parentIntent.getBooleanExtra("offscreenSingleTile", false)); // equal to "-singleframe"
                js.put("measurePerFrame", parentIntent.getBooleanExtra("measurePerFrame", false));
                js.put("colorBitsRed", parentIntent.getIntExtra("colorBitsRed", -1));
                js.put("colorBitsGreen", parentIntent.getIntExtra("colorBitsGreen", -1));
                js.put("colorBitsBlue", parentIntent.getIntExtra("colorBitsBlue", -1));
                js.put("colorBitsAlpha", parentIntent.getIntExtra("colorBitsAlpha", -1));
                js.put("depthBits", parentIntent.getIntExtra("depthBits", -1));
                js.put("stencilBits", parentIntent.getIntExtra("stencilBits", -1));

                if (parentIntent.getBooleanExtra("antialiasing", false)) {
                    js.put("msaaSamples", 4);
                }

                // GUI specific
                if (parentIntent.hasExtra("use24BitColor")) {
                    js.put("colorBitsRed", 8);
                    js.put("colorBitsGreen",8);
                    js.put("colorBitsBlue", 8);
                }
                if (parentIntent.hasExtra("useAlpha")) {
                    js.put("colorBitsAlpha", 8);
                }
                if (parentIntent.hasExtra("use24BitDepth")) {
                    js.put("depthBits", 24);
                }

                js.put("removeUnusedVertexAttributes",
                       parentIntent.getBooleanExtra("removeUnusedAttributes", false));

                js.put("storeProgramInformation",
                       parentIntent.getBooleanExtra("storeProgramInfo", false));

                if (parentIntent.getBooleanExtra("debug", false))
                {
                    js.put("debug", true);
                    js.put("measurePerFrame", true); // will not start debug collector otherwise
                    JSONArray ja = new JSONArray();
                    ja.put("debug");
                    js.put("instrumentation", ja);
                }

            } catch (JSONException e) {
                e.printStackTrace();
            }

            json_data = js.toString();
        }

        if (resultFile == null) {
            Log.e(TAG, "result file is null.");
            resultFile = "/sdcard/results.json";
        }

        if (!NativeAPI.initFromJson(json_data, trace_file_path, resultFile)) {
            Log.e(TAG, "Could not initialise the retracer from JSON!");
            return;
        }

        Log.i(TAG, "Initialised the retracer from JSON: " + json_data);

        boolean isPortrait = NativeAPI.opt_getIsPortraitMode();
        Log.i(TAG, "Portrait mode: " + isPortrait);
        int screenOrientation = isPortrait ?
            ActivityInfo.SCREEN_ORIENTATION_PORTRAIT :
            ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
        int oldScreenOrientation = mRetraceActivity.getRequestedOrientation();
        mRetraceActivity.setRequestedOrientation(screenOrientation);

        NativeAPI.init(false);
        Log.i(TAG, "EGL initialisation finished.");

        try {
            NativeAPI.step();
        }
        catch (Exception e) {}

        JSONObject battery_after = batterydata();
        JSONObject js = getdata();
        try {
            js.put("battery_changes", voltageinfo);
            js.put("battery_before", battery_before);
            js.put("battery_after", battery_after);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        Log.i(TAG, "Generated JSON: " + js.toString());
        NativeAPI.stop(js.toString());
        Log.i(TAG, "Restore the orientation mode: " + oldScreenOrientation);
        mRetraceActivity.setRequestedOrientation(oldScreenOrientation);
    }
}
