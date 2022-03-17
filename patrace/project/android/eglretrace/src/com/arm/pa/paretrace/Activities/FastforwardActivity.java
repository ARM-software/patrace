package com.arm.pa.paretrace.Activities;

import android.app.Activity;
import android.content.Intent;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

import java.io.FileReader;
import java.io.BufferedReader;
import org.json.JSONObject;

import com.arm.pa.paretrace.R;

public class FastforwardActivity extends Activity {
    private static final String TAG = "paretrace FastforwardActivity";
    private static boolean isStarted = false;

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        Log.d(TAG, "FastforwardActivity: onCreate()");
    }

    @Override
    protected void onStart() {
        super.onStart();
        Log.d(TAG, "FastforwardActivity: onStart()");
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.d(TAG, "FastforwardActivity: onResume()");

        if (!isStarted) {
            Intent receivedIntent = this.getIntent();

            // intent forward to retrace activity
            String input = receivedIntent.getStringExtra("input");
            String output = receivedIntent.getStringExtra("output");
            int targetFrame = receivedIntent.getIntExtra("targetFrame", -1);
            int endFrame = receivedIntent.getIntExtra("endFrame", -1);
            boolean multithread = receivedIntent.getBooleanExtra("multithread", false);
            boolean offscreen = receivedIntent.getBooleanExtra("offscreen", false);
            boolean noscreen = receivedIntent.getBooleanExtra("noscreen", false);
            boolean norestoretex = receivedIntent.getBooleanExtra("norestoretex", false);
            boolean version = receivedIntent.getBooleanExtra("version", false);
            int restorefbo0 = receivedIntent.getIntExtra("restorefbo0", -1);
            boolean txu = receivedIntent.getBooleanExtra("txu", false);

            // init with json parameters
            if (receivedIntent.hasExtra("jsonParam")) {
                String json_path = receivedIntent.getStringExtra("jsonParam");
                Log.i(TAG, "json_path: " + json_path);
                String json_data = new String();

                // check if json_extra is a file path, and if so, read JSON data from the file
                if (json_path.charAt(0) == '/' || Character.isLetter(json_path.charAt(0))) {
                    try {
                        BufferedReader r = new BufferedReader(new FileReader(json_path));
                        StringBuilder total = new StringBuilder();
                        String line;
                        while ((line = r.readLine()) != null) {
                            total.append(line);
                        }
                        r.close();
                        json_data = total.toString();
                        Log.i(TAG, "json_data: " + json_data);
                        JSONObject json_Param = new JSONObject(json_data);
                        // intent has higher priority than jsonParam
                        if (!receivedIntent.hasExtra("input") && json_Param.has("input")) {
                            input = json_Param.getString("input");
                        }
                        if (!receivedIntent.hasExtra("output") && json_Param.has("output")) {
                            output = json_Param.getString("output");
                        }
                        if (!receivedIntent.hasExtra("targetFrame") && json_Param.has("targetFrame")) {
                            targetFrame = json_Param.getInt("targetFrame");
                        }
                        if (!receivedIntent.hasExtra("endFrame") && json_Param.has("endFrame")) {
                            endFrame = json_Param.getInt("endFrame");
                        }
                        if (!receivedIntent.hasExtra("multithread") && json_Param.has("multithread")) {
                            multithread = json_Param.getBoolean("multithread");
                        }
                        if (!receivedIntent.hasExtra("offscreen") && json_Param.has("offscreen")) {
                            offscreen = json_Param.getBoolean("offscreen");
                        }
                        if (!receivedIntent.hasExtra("norestoretex") && json_Param.has("norestoretex")) {
                            norestoretex = json_Param.getBoolean("norestoretex");
                        }
                        if (!receivedIntent.hasExtra("version") && json_Param.has("version")) {
                            version = json_Param.getBoolean("version");
                        }
                        if (!receivedIntent.hasExtra("restorefbo0") && json_Param.has("restorefbo0")) {
                            restorefbo0 = json_Param.getInt("restorefbo0");
                        }
                        if (!receivedIntent.hasExtra("txu") && json_Param.has("txu")) {
                            txu = json_Param.getBoolean("txu");
                        }
                    } catch (Exception e) {
                        json_data = ""; // shut up the compiler
                        e.printStackTrace();
                        System.exit(1);
                    }
                }
            }

            // combine the parameter into a string intent and forward to RetraceActivity
            String args = new String();
            if (input != null) {
                args += ("--input " + input + " ");
            }
            if (output != null) {
                args += ("--output " + output + " ");
            }
            if (targetFrame != -1) {
                args += ("--targetFrame " + targetFrame + " ");
            }
            if (endFrame != -1) {
                args += ("--endFrame " + endFrame + " ");
            }
            if (multithread == true) {
                args += "--multithread ";
            }
            if (offscreen == true) {
                args += "--offscreen ";
            }
            if (noscreen == true) {
                args += "--noscreen ";
            }
            if (norestoretex == true) {
                args += "--norestoretex ";
            }
            if (version == true) {
                args += "--version ";
            }
            if (restorefbo0 != -1) {
                args += ("--restorefbo0 " + restorefbo0 + " ");
            }
            if (txu == true) {
                args += "--txu ";
            }
            Log.i(TAG, "forward args: " + args);

            Intent forwardIntent = new Intent(this, RetraceActivity.class);
            forwardIntent.putExtra("fastforward", args);
            this.startActivity(forwardIntent);
            isStarted = true;
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.d(TAG, "FastforwardActivity: onPause()");
    }

    @Override
    protected void onStop() {
        super.onStop();
        Log.d(TAG, "FastforwardActivity: onStop()");
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        isStarted = false;
        Thread suicideThread = new Thread() {
            public void run() {
                try {
                    sleep(500);
                } catch (InterruptedException ie) {
                }
                System.exit(0);
            }
        };
        Log.d(TAG, "FastforwardActivity: onDestroy()");
        suicideThread.start();
    }
}
