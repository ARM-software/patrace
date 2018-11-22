package com.arm.pa.paretrace.Types;

import java.io.File;
import java.io.Serializable;
import java.util.Comparator;

public class Trace implements Serializable {

	private static final long serialVersionUID = -3047905631096791578L;

	private File mFile;
	private String mHash;
	private int mHeight;
	private int mWidth;
	private boolean mOverrideRes = false;
	private int mOverrideHeight;
	private int mOverrideWidth;
	private int mFrameStart;
	private int mFrameEnd;
	private String mFrames = "";
	private int mThreadId;
	private boolean mOffscreen = true;
	private boolean mPreload = false;
	private boolean m24BitColorEnabled = false;
	private boolean m24BitDepthEnabled = false;
	private boolean mAntiAliasEnabled = false;
	private boolean mMeasurePerFrame = false;

	public File getFile() {
		return mFile;
	}

	public void setFile(File file) {
		this.mFile = file;
	}

	public String getFileName(){
		return mFile.getName();
	}

	public String getFileDir(){
		return mFile.getParent();
	}

	public String getFileSize(){
		double length = 0;
		if (mFile.exists()){
			length = mFile.length();
		}
		return bytesToReadableString((long)length);
	}

	private static String bytesToReadableString(long num_bytes){
		int unit = 1024;
		if (num_bytes < unit) return num_bytes + " B";
		int exponent = (int) (Math.log(num_bytes)/ Math.log(unit));
		String prefix = ("KMGTPE").charAt(exponent-1) + "i";
		return String.format("%.1f %sB", num_bytes / Math.pow(unit, exponent), prefix);
	}

	public String getHash() {
		return mHash;
	}

	public int getFrameStart() {
		return mFrameStart;
	}

	public void setFrameStart(int mFrameStart) {
		this.mFrameStart = mFrameStart;
	}

	public int getFrameEnd() {
		return mFrameEnd;
	}

	public void setFrameEnd(int mFrameEnd) {
		this.mFrameEnd = mFrameEnd;
	}

	public void setHash(String hash) {
		this.mHash = hash;
	}

	public int getHeight() {
		return mHeight;
	}

	public void setHeight(int mHeight) {
		this.mHeight = mHeight;
	}

	public int getWidth() {
		return mWidth;
	}

	public void setWidth(int mWidth) {
		this.mWidth = mWidth;
	}

	public boolean isOverrideRes() {
		return mOverrideRes;
	}

	public void setOverrideRes(boolean mOverrideRes) {
		this.mOverrideRes = mOverrideRes;
	}

	public int getOverrideHeight() {
		return mOverrideHeight;
	}

	public void setOverrideHeight(int mOverrideHeight) {
		this.mOverrideHeight = mOverrideHeight;
	}

	public int getOverrideWidth() {
		return mOverrideWidth;
	}

	public void setOverrideWidth(int mOverrideWidth) {
		this.mOverrideWidth = mOverrideWidth;
	}

	public String getFrames() {
		return mFrames;
	}

	public void setFrames(String mFrames) {
		this.mFrames = mFrames;
	}

	public void setPreload(boolean preload){
		mPreload = preload;
	}

	public boolean getPreload(){
		return mPreload;
	}

	public int getThreadId() {
		return mThreadId;
	}

	public void setThreadId(int mThreadId) {
		this.mThreadId = mThreadId;
	}

	public boolean isOffscreen() {
		return mOffscreen;
	}

	public void setOffscreen(boolean mOffscreen) {
		this.mOffscreen = mOffscreen;
	}

	public boolean delete(){
		return mFile.delete();
	}

	public void set24BitColor(boolean is24BitColor){
		m24BitColorEnabled = is24BitColor;
	}

	public void set24BitDepth(boolean is24BitDepth){
		m24BitDepthEnabled = is24BitDepth;
	}

	public boolean get24BitColorEnabled(){
		return m24BitColorEnabled;
	}

	public boolean get24BitDepthEnabled(){
		return m24BitDepthEnabled;
	}

	public void setAntiAliasEnabled(boolean antiAliasEnabled){
		mAntiAliasEnabled = antiAliasEnabled;
	}

	public boolean getAntiAliasEnabled(){
		return mAntiAliasEnabled;
	}

	public boolean getMeasurePerFrameEnabled() {
		return mMeasurePerFrame;
	}

	public void setMeasurePerFrameEnabled(boolean measurePerFrame) {
		this.mMeasurePerFrame = measurePerFrame;
	}

    /* Comparator for sorting the list by fileName */
    public static Comparator<Trace> fileNameComparator = new Comparator<Trace>() {
        public int compare(Trace t1, Trace t2) {
           String fileName1 = t1.getFileName().toUpperCase();
           String fileName2 = t2.getFileName().toUpperCase();

           return fileName1.compareTo(fileName2);       //ascending order
        }
    };
}
