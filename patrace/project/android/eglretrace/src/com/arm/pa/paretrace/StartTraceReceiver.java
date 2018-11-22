package com.arm.pa.paretrace;

import com.arm.pa.paretrace.Activities.RetraceActivity;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class StartTraceReceiver extends BroadcastReceiver {

	@Override
	public void onReceive(Context ctx, Intent intent) {

		Intent startTraceIntent = new Intent(ctx, RetraceActivity.class);

		startTraceIntent.putExtras(intent);
		startTraceIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

		ctx.startActivity(startTraceIntent);
	}
}
