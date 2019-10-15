package com.arm.pa.paretrace.Activities;

import java.io.File;
import java.io.FilenameFilter;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Collections;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.RadioButton;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.arm.pa.paretrace.R;
import com.arm.pa.paretrace.Types.Trace;
import com.arm.pa.paretrace.Util.Util;

public class SelectActivity extends Activity
{

	private static final String TAG = "PATRACE_UI";
	public static final String CONNECTION_UPDATE_ACTION = "com.arm.pa.paretrace.CONNECTION_STATUS_UPDATE";

	private static LinkedHashMap<String, String> mTraceOptionNames;
	private static LinkedHashMap<String, Boolean> mTraceOptionValues;

	private ArrayList<Trace> mTraceList;
	private ListView mListView;
	private View root_view;
	private BroadcastReceiver mConnectionStatusReceiver;

	@Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.select_activity);

        setupTraceOptions();
        setupLayout();
        loadSettings();

        //Set receiver for status updates
    	if (mConnectionStatusReceiver == null){
    		final IntentFilter filter = new IntentFilter();
            filter.addAction(CONNECTION_UPDATE_ACTION);

            mConnectionStatusReceiver = new BroadcastReceiver() {

                @Override
                public void onReceive(Context context, Intent intent) {
                	String status = intent.getStringExtra("status");
                	int progress = intent.getIntExtra("progress", 0);
                }
            };
            registerReceiver(mConnectionStatusReceiver, filter);
    	}
    }

    @Override
    protected void onPause() {
        super.onPause();
        saveSettings();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        unregisterReceiver(mConnectionStatusReceiver);
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

    private void setupTraceOptions(){
    	mTraceOptionNames = new LinkedHashMap<String, String>();
    	mTraceOptionNames.put("Offscreen", "forceOffscreen");
    	mTraceOptionNames.put("24bit Color", "use24BitColor");
    	mTraceOptionNames.put("Alpha", "useAlpha");
    	mTraceOptionNames.put("24bit Depth", "use24BitDepth");
    	mTraceOptionNames.put("AntiAlias (EGL)", "antialiasing");
    	mTraceOptionNames.put("Preload", "preload");
        mTraceOptionNames.put("Store Program Info", "storeProgramInfo");
        mTraceOptionNames.put("Remove Unused Attributes", "removeUnusedAttributes");
        mTraceOptionNames.put("Debug", "debug");
        mTraceOptionNames.put("Draw log", "drawlog");

    	mTraceOptionValues = new LinkedHashMap<String, Boolean>();
    	mTraceOptionValues.put("forceOffscreen", false);
    	mTraceOptionValues.put("use24BitColor", true);
    	mTraceOptionValues.put("useAlpha", true);
    	mTraceOptionValues.put("use24BitDepth", true);
    	mTraceOptionValues.put("antialiasing", false);
    	mTraceOptionValues.put("preload", false);
        mTraceOptionValues.put("storeProgramInfo", false);
        mTraceOptionValues.put("removeUnusedAttributes", false);
        mTraceOptionValues.put("debug", false);
        mTraceOptionValues.put("drawlog", false);
    }

    private void setupTraceList(){
    	mTraceList.clear();
        appendTracesInDir(new File("/data/apitrace"), true, mTraceList);
        appendTracesInDir(new File(Util.getTraceFilePath()), true, mTraceList);
        Collections.sort(mTraceList, Trace.fileNameComparator);
    }

	private void setupLayout(){

		SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(this);
		for (Map.Entry<String, String> entry: mTraceOptionNames.entrySet()){
			mTraceOptionValues.put(entry.getValue(), pref.getBoolean("option_"+entry.getValue(), mTraceOptionValues.get(entry.getValue())));
		}

		/*** Retracer interface ***/

		// Trace files
		mTraceList = new ArrayList<Trace>();
		setupTraceList();
        TraceListAdapter trace_list_adapter = new TraceListAdapter(mTraceList);
        mListView = (ListView) findViewById(R.id.list);
        registerForContextMenu(mListView);
        mListView.setAdapter(trace_list_adapter);
        mListView.setOnItemClickListener(new OnItemClickListener() {

			@Override
			public void onItemClick(AdapterView<?> arg0, View arg1, int arg2,
					long arg3) {
				startRetrace((Trace)mListView.getAdapter().getItem(arg2));
			}
		});
        root_view = findViewById(R.id.root);

		// Options
 		LinearLayout options_root = (LinearLayout) findViewById(R.id.retrace_options);
 		for (Map.Entry<String, String> entry: mTraceOptionNames.entrySet()){
 			addOption(options_root, entry.getKey());
 		}

	}

	private void addOption(LinearLayout v, final String option_name){
		LayoutInflater li = getLayoutInflater();
		View option_view = li.inflate(R.layout.option_item, null);
		((TextView)option_view.findViewById(R.id.option_title)).setText(option_name);
		((CheckBox)option_view.findViewById(R.id.option_checkbox)).setChecked(mTraceOptionValues.get(mTraceOptionNames.get(option_name)));
		((CheckBox)option_view.findViewById(R.id.option_checkbox)).setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View arg0) {
				mTraceOptionValues.put(mTraceOptionNames.get(option_name), ((CheckBox)arg0).isChecked());
			}
		});

		v.addView(option_view);
	}

	private void saveSettings(){
		SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(this).edit();

		for (Map.Entry<String, Boolean> entry: mTraceOptionValues.entrySet()){
			editor.putBoolean("option_"+entry.getKey(), entry.getValue());
		}
		try {
    		int thread = Integer.parseInt(((EditText)findViewById(R.id.option_thread_id)).getText().toString());
    		int alpha = Integer.parseInt(((EditText)findViewById(R.id.option_alpha)).getText().toString());
            boolean enable_res = ((CheckBox)findViewById(R.id.option_enableres)).isChecked();
            boolean enable_multithread = ((RadioButton)findViewById(R.id.option_enable_multithread)).isChecked();
            boolean force_single_window = ((RadioButton)findViewById(R.id.option_force_single_window)).isChecked();
            boolean enable_overlay = ((RadioButton)findViewById(R.id.option_enable_overlay)).isChecked();
            boolean enable_seltid = ((CheckBox)findViewById(R.id.option_enable_seltid)).isChecked();
            boolean enable_fullscreen = ((CheckBox)findViewById(R.id.option_enable_fullscreen)).isChecked();
    		int xres = Integer.parseInt(((EditText)findViewById(R.id.option_xres)).getText().toString());
    		int yres = Integer.parseInt(((EditText)findViewById(R.id.option_yres)).getText().toString());

			editor.putInt("settings_thread_id", thread);
			editor.putInt("settings_xres", xres);
			editor.putInt("settings_yres", yres);
			editor.putBoolean("settings_enable_res", enable_res);
			editor.putBoolean("settings_enable_seltid", enable_seltid);
			editor.putBoolean("settings_enable_multithread", enable_multithread);
			editor.putBoolean("settings_force_single_window", force_single_window);
			editor.putBoolean("settings_enable_overlay", enable_overlay);
			editor.putBoolean("settings_enable_fullscreen", enable_fullscreen);
			editor.putInt("settings_alpha", alpha);
		}
		catch (Exception e){
			Log.e(TAG, "Couldn't save settings");
		}

		editor.commit();
	}

	private void loadSettings(){
		SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(this);
		/*
		for (int i = 0; i < TRACE_OPTIONS.length; i++){
			mTraceOptionsValues[i] = pref.getBoolean("settings_"+TRACE_OPTIONS[i], mTraceOptionsValues[i]);
		}
		*/

		int thread = pref.getInt("settings_thread_id", 0);
		int alpha = pref.getInt("settings_alpha", 100);
		int xres = pref.getInt("settings_xres", 1280);
		int yres = pref.getInt("settings_yres", 720);
        boolean enable_res = pref.getBoolean("settings_enable_res", false);
        boolean enable_seltid = pref.getBoolean("settings_enable_seltid", false);
		boolean enable_multithread = pref.getBoolean("settings_enable_multithread", false);
		boolean force_single_window = pref.getBoolean("settings_force_single_window", true);
		boolean enable_overlay = force_single_window? false : pref.getBoolean("settings_enable_overlay", true);
		boolean enable_split   = force_single_window? false : !enable_overlay;
		boolean enable_fullcreen = pref.getBoolean("settings_enable_fullcreen", false);

        ((RadioButton)root_view.findViewById(R.id.option_enable_singlethread)).setChecked(!enable_multithread);
		((EditText)root_view.findViewById(R.id.option_thread_id)).setText(""+thread);
        ((RadioButton)root_view.findViewById(R.id.option_enable_multithread)).setChecked(enable_multithread);
        ((RadioButton)root_view.findViewById(R.id.option_enable_overlay)).setChecked(enable_overlay);
        ((RadioButton)root_view.findViewById(R.id.option_enable_split)).setChecked(enable_split);
        ((RadioButton)root_view.findViewById(R.id.option_force_single_window)).setChecked(force_single_window);
		((EditText)root_view.findViewById(R.id.option_alpha)).setText(""+alpha);
        ((CheckBox)root_view.findViewById(R.id.option_enable_seltid)).setChecked(enable_seltid);
        ((EditText)root_view.findViewById(R.id.option_xres)).setText(""+xres);
        ((EditText)root_view.findViewById(R.id.option_yres)).setText(""+yres);
        ((CheckBox)root_view.findViewById(R.id.option_enableres)).setChecked(enable_res);
        ((CheckBox)root_view.findViewById(R.id.option_enable_fullscreen)).setChecked(enable_fullcreen);
	}

    private ArrayList<Trace> getTracesFromPath(String path){
    	File folder = new File(path);
        File[] listOfFilesData = folder.listFiles(new FilenameFilter() {
            public boolean accept(File dir, String name) {
                return name.toLowerCase().endsWith(".pat");
            }
        });
        ArrayList<Trace> trace_list = new ArrayList<Trace>();
        if(listOfFilesData != null){
        	for (int i = 0; i < listOfFilesData.length; ++i) {
            	File filepath = listOfFilesData[i];
            	Trace t = new Trace();
            	t.setFile(filepath);
            	trace_list.add(t);
            }
        }

        return trace_list;
    }

    private void appendTracesInDir(File file, boolean isRecursive, ArrayList<Trace> traceList) {
        if (!file.isDirectory())
            return;

        if (isRecursive) {
            String[] dirs = file.list(new FilenameFilter() {
                public boolean accept(File dir, String name) {
                    return new File(dir, name).isDirectory();
                }
            });

            if (dirs != null) {
                for (String name : dirs)
                    appendTracesInDir(new File(file, name), isRecursive, traceList);
            }
        }

        File[] pats = file.listFiles(new FilenameFilter() {
            public boolean accept(File dir, String name) {
                return name.toLowerCase().endsWith(".pat");
            }
        });
        if (pats != null) {
            for (File curPat : pats) {
                Trace t = new Trace();
                t.setFile(curPat);
                traceList.add(t);
            }
        }
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v,
        ContextMenuInfo menuInfo) {
      if (v.getId()==R.id.list) {
        menu.setHeaderTitle("Trace file options");
        String[] menuItems = { "Delete" };
        for (int i = 0; i<menuItems.length; i++) {
          menu.add(Menu.NONE, i, i, menuItems[i]);
        }
      }
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
      AdapterView.AdapterContextMenuInfo info = (AdapterView.AdapterContextMenuInfo)item.getMenuInfo();
      if (item.getTitle().equals("Delete")){
    	  if (info.position < mTraceList.size() ){
    		  if (mTraceList.get(info.position).delete()){
    		    Toast.makeText(SelectActivity.this, "File deleted successfully.", Toast.LENGTH_LONG).show();
    			  mTraceList.remove(info.position);
    			  ((TraceListAdapter)mListView.getAdapter()).notifyDataSetChanged();
    		  }
    		  else {
    			  Toast.makeText(SelectActivity.this, "Couldn't delete file.", Toast.LENGTH_LONG).show();
    		  }
    	  }
      }

      return true;
    }

    private class TraceListAdapter extends BaseAdapter {

    	private ArrayList<Trace> mTraceList;
    	private LayoutInflater mInflater;

    	public TraceListAdapter(ArrayList<Trace> list) {
    		mTraceList = list;
    		mInflater = (LayoutInflater)getSystemService(Context.LAYOUT_INFLATER_SERVICE);

		}

		@Override
		public int getCount() {
			return mTraceList.size();
		}

		@Override
		public Trace getItem(int position) {
			return mTraceList.get(position);
		}

		@Override
		public long getItemId(int position) {
			return position;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			if (convertView == null){
				convertView = mInflater.inflate(R.layout.trace_list_item, null);
			}
			Trace trace = mTraceList.get(position);
			TextView title = (TextView) convertView.findViewById(R.id.title);
			title.setText(trace.getFileName());
			TextView subtitle = (TextView) convertView.findViewById(R.id.subtitle);
			subtitle.setText("("+trace.getFileSize() + ")  " + trace.getFileDir());

			return convertView;
		}
    }


    void startRetrace(Trace trace)
    {
    	Intent intent = new Intent(this, RetraceActivity.class);
        intent.putExtra("fileName", trace.getFile().getAbsolutePath());
        intent.putExtra("resultFile", trace.getFile().getAbsolutePath()+".result");
        intent.putExtra("isGui", true);

        boolean enable_snapshot = ((CheckBox)findViewById(R.id.option_enablesnapshot)).isChecked();
        if (enable_snapshot) {
            intent.putExtra("callset", ((EditText)findViewById(R.id.option_snapshotcallset)).getText().toString());
            intent.putExtra("callsetprefix", ((EditText)findViewById(R.id.option_snapshotcallsetprefix)).getText().toString());
        }

        boolean enable_framerange = ((CheckBox)findViewById(R.id.option_enableframerange)).isChecked();
        if (enable_framerange) {
            int fstart = Integer.parseInt(((EditText)findViewById(R.id.option_framestart)).getText().toString());
            int fend = Integer.parseInt(((EditText)findViewById(R.id.option_frameend)).getText().toString());
            intent.putExtra("frame_start", fstart);
            intent.putExtra("frame_end", fend);
        }

        boolean enable_multithread = false;
    	try {
            enable_multithread = ((RadioButton)findViewById(R.id.option_enable_multithread)).isChecked();
            if (enable_multithread) {
                intent.putExtra("multithread", true);
            }
            else {
                intent.putExtra("multithread", false);
                boolean seltid = ((CheckBox)findViewById(R.id.option_enable_seltid)).isChecked();
                int thread = Integer.parseInt(((EditText)findViewById(R.id.option_thread_id)).getText().toString());
                if (seltid) {
                    intent.putExtra("tid", thread);
                }
            }
    	}
        catch (NumberFormatException e){
            Toast.makeText(this, "Invalid thread id, frame start or frame end number." , Toast.LENGTH_SHORT).show();
            return;
        }

        boolean force_single_window = ((RadioButton)findViewById(R.id.option_force_single_window)).isChecked();
        if (force_single_window && enable_multithread){
            Toast.makeText(this, "Force single window is not valid when multithread enabled!" , Toast.LENGTH_SHORT).show();
            return;
        }
        if (force_single_window)
        {
            intent.putExtra("force_single_window", true);
            intent.putExtra("enOverlay", false);
        }
        else
        {
            boolean enable_overlay = ((RadioButton)findViewById(R.id.option_enable_overlay)).isChecked();
            intent.putExtra("force_single_window", false);
            if (enable_overlay)
            {
                intent.putExtra("enOverlay", true);
                int alpha = Integer.parseInt(((EditText)findViewById(R.id.option_alpha)).getText().toString());
                intent.putExtra("transparent", alpha);
            }
            else
            {
                intent.putExtra("enOverlay", false);
            }
        }

        boolean enable_FullScreen = ((CheckBox)findViewById(R.id.option_enable_fullscreen)).isChecked();
        if (enable_FullScreen)
        {
            intent.putExtra("enFullScreen", true);
        }
        else
        {
            intent.putExtra("enFullScreen", false);
        }

        boolean enable_res = ((CheckBox)findViewById(R.id.option_enableres)).isChecked();
        if (enable_res) {
            try {
                int xres = Integer.parseInt(((EditText)findViewById(R.id.option_xres)).getText().toString());
                int yres = Integer.parseInt(((EditText)findViewById(R.id.option_yres)).getText().toString());
                intent.putExtra("oresW", xres);
                intent.putExtra("oresH", yres);
            }
            catch (NumberFormatException e){
                Toast.makeText(this, "Invalid resolution. Please enter an integer" , Toast.LENGTH_SHORT).show();
                return;
            }
        }

    	for (Map.Entry<String, Boolean> entry: mTraceOptionValues.entrySet()){
			intent.putExtra(entry.getKey(), entry.getValue());
    	}

        this.startActivity(intent);
    }

}
