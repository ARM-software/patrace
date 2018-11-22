package com.arm.pa.paretrace;

import android.app.Application;
import android.util.Log;

public class RetraceApplication extends Application {

	private static final String TAG = "patrace";

    @Override
    public void onCreate() {

    }

    @Override
    public void onTerminate() {

    }

	@Override
	public void onLowMemory() {
		Log.w(TAG, "Received low memory warning!");
		super.onLowMemory();
	}
}
