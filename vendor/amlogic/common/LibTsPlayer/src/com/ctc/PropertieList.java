package com.ctc;

import android.util.Log;

import java.util.List;
import java.util.ArrayList;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileReader;
import java.util.Properties;

public class PropertieList {
	private static final String TAG = "PropertieList";
	private static final String PathName = "ctc_test.properties";
	private static final String KEY_PLAYNAME = "PlayName";
	private static final String KEY_PLAYBUFFERSIZE = "PlayBufferSize";
	private static Properties mProperties = null;
	private static String mRootPath = null;
	
	public PropertieList() {
		File file = loadFile();
		mProperties = null;
		if(file != null) {
			mProperties = loadProperties(file);
		}
    }
	
	private List<String> loadDir() {
		File mounts = new File("/proc/mounts");
		List<String> paths = new ArrayList<String>();
		
		if(mounts.exists()) {
			try {
				BufferedReader reader = new BufferedReader(
						new FileReader(mounts));
				try {
					String text = null;
					while((text = reader.readLine()) != null) {
						Log.d(TAG, text);
						if(text.startsWith("/dev/block/vold/")) {
							String[] splits = text.split(" ");
							Log.d(TAG, "len= " + splits.length);
							if(splits.length > 2) {
								Log.d(TAG, splits[1]);
								paths.add(splits[1]);
							}
						}
					}
				}
				finally{
					reader.close();
				}
			}
			catch(Exception e) {
				e.printStackTrace();
			}
		}
		else {
			Log.w(TAG, "File</proc/mounts> is not exists");
		}
		return paths;
	}
	
	private File loadFile() {
		/*
		File dir = new File("/mnt/");
		if(dir.exists()) {
			File[] childs = dir.listFiles();
			int count = childs.length;
			Log.d(TAG, "Path</mnt/> child count = " + count);
			for(int i=0; i<count; i++) {
				File child = childs[i];
				if(child != null) {
					if(child.isDirectory()) {
						String path = child.getAbsolutePath();
						Log.d(TAG, path + ", index is " + i);
						File file = new File(path + "/" + PathName);
						if(file.exists()) {
							return file;
						}
					}
				}
				else {
					Log.w(TAG, "Child is null, index is " + i);
				}
			}
		}
		else {
			Log.w(TAG, "Path</mnt/> is not exists");
		}
		return null;
		*/
		List<String> paths = loadDir();
		int count = paths.size();
		for(int i=0; i<count; i++) {
			String path = paths.get(i);
			File file = new File(path + "/" + PathName);
			if(file.exists()) {
				mRootPath = new String(path);
				return file;
			}
		}
		Log.w(TAG, "Not found " + PathName);
		return null;
	}
	
	private Properties loadProperties(File file) {
		Properties properties = new Properties();
		
		try {
			FileInputStream input = new FileInputStream(file);
			properties.load(input);
			input.close();
		} 
		catch (Exception e) {
			e.printStackTrace();
		}
		return properties;
	}
	
	public String getPlayUrl() {
		String defUrl = "/storage/external_storage/sdcard1/iptv_test.ts";
		if(mProperties == null) {
			Log.w(TAG, "getPlayUrl, mProperties is null!");
			return defUrl;
		}
		if(mRootPath == null) {
			Log.w(TAG, "getPlayUrl, mRootPath is null!");
			return defUrl;
		}
		String fileName = mProperties.getProperty(KEY_PLAYNAME, null);
		if(fileName == null) {
			Log.w(TAG, "getPlayUrl, fileName is null!");
			return defUrl;
		}
		if(fileName.startsWith("/")) {
		  return mRootPath + fileName;
		}
		else {
		  return fileName;
		} 
	}
	
	public int getPlayBufferSize(int defSize) {//Unit is KB
		if(mProperties == null) {
			Log.w(TAG, "getPlayBuffer, mProperties is null!");
			return defSize;
		}
		String bufStr = mProperties.getProperty(KEY_PLAYBUFFERSIZE, null);
		if(bufStr == null) {
			Log.w(TAG, "getPlayBuffer, bufStr is null!");
			return defSize;
		}
		try {
			int ret = Integer.parseInt(bufStr);
			Log.d(TAG, "buffer size is " + ret + "KB");
			return defSize;
		} catch(Exception e) {
			e.printStackTrace();
		}
		return defSize;
	}
}
