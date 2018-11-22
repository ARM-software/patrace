package com.arm.pa.paretrace.Util;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.io.StringWriter;
import java.io.Writer;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.app.Activity;
import android.os.Environment;

public class Util {
	private static final String IPADDRESS_PATTERN = 
			"^([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\." +
			"([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\." +
			"([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\." +
			"([01]?\\d\\d?|2[0-4]\\d|25[0-5])$";
	 
    public static boolean validateIPAdress(final String ip){		  
		Pattern pattern = Pattern.compile(IPADDRESS_PATTERN);
		Matcher matcher = pattern.matcher(ip);
		return matcher.matches();	    	    
    }
    
    public static boolean validatePort(final String port){
    	try {
    		Integer test = Integer.parseInt(port);
    		return true;
    	}
    	catch(NumberFormatException e){
    		
    	}
    	return false;
    }
    
	public static String getTraceFilePath(){
		return Environment.getExternalStorageDirectory().getAbsolutePath()+"/apitrace/trace_repo";
	}
	
    public static String getStringFromRawResource(Activity act, int resource){
    	InputStream is = act.getResources().openRawResource(resource);
    	Writer writer = new StringWriter();
    	char[] buffer = new char[1024];
    	try {
    	    Reader reader = new BufferedReader(new InputStreamReader(is, "UTF-8"));
    	    int n;
    	    while ((n = reader.read(buffer)) != -1) {
    	        writer.write(buffer, 0, n);
    	    }
    	}
    	catch (Exception e){}
    	finally {
    	    try { is.close(); }
    	    catch (IOException e) {}
    	}

    	return writer.toString();

    }
}
