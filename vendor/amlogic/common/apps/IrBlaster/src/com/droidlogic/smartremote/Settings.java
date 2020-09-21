/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC
 */

package com.droidlogic.smartremote;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.StringWriter;
import java.util.ArrayList;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xmlpull.v1.XmlPullParserFactory;
import org.xmlpull.v1.XmlSerializer;

import android.content.Context;
import android.content.SharedPreferences;

public class Settings {
	
	private static Settings sInstance = null;
	
	public static final String CURR_MODEL_ID = "CURR_MODEL_ID";
	public static final String MAX_ID = "MAX_ID";
	
	
	private ArrayList<SettingsChangedListener> mListeners = null;
	private Context mContext = null;
	
	private Model mCurrModel = null;
	private SharedPreferences mPref = null;
	
	private ArrayList<Model> mModelsList = null;
	//private boolean mInitOk = false;
	
	private int mCurrMaxId = 0;
	
	public class Model
	{
		public int mId = -1;
		public int mPolicyId = 0;
		public String name = null;
	}
	
	public interface SettingsChangedListener
	{
		public void onCurrModelChanged(Model currModel);
		public void onModelsChanged(ArrayList<Model> list);
	}
	
	private Settings(Context c)
	{
		this.mContext = c;
		mListeners = new ArrayList<SettingsChangedListener>();
		mModelsList = new ArrayList<Model>();
		
		mPref = mContext.getSharedPreferences("Settings", Context.MODE_PRIVATE);
	}
	
	private void doInit()
	{
		mCurrMaxId = mPref.getInt(MAX_ID, 0);
		
		File modelFile = new File(mContext.getFilesDir(),"models.xml");
		InputStream input = null;
		try {
			if(modelFile.exists()) {
				input = new FileInputStream(modelFile);
			} else {
				input = mContext.getAssets().open("models.xml");
			}
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			input = null;
		} 

		if(input == null) {
			Log.e("Settings init failed , init file exception");
		} else if(!getModels(input)) {
			Log.e("Settings init failed , get models failed");
		} else {
			//mInitOk = true;
			Log.d("Settings init Success");		
		}
		
		try {
			if(input != null) input.close();
		} catch (Exception e) {}
		
		mListeners.clear();
		
		int model_id = mPref.getInt(CURR_MODEL_ID, 0);
		mCurrModel = findModelById(model_id);
		
		if(mCurrModel == null) {
			mCurrModel = mModelsList.get(0);
			mPref.edit().putInt(CURR_MODEL_ID, mCurrModel.mId).commit();
		}
	}
	public static void init()
	{
		sInstance.doInit();
	}
	public static void instance(Context c)
	{
		if(sInstance == null) {
			sInstance = new Settings(c);
		}
	}
	
	public static void registerListener(SettingsChangedListener listener)
	{
		sInstance.mListeners.add(listener);
	}
	public static void unregisterListener(SettingsChangedListener listener)
	{
		sInstance.mListeners.remove(listener);
	}
	public static Model getCurrModel ()
	{
		return sInstance.mCurrModel;
	}
	public static ArrayList<Model> getModeslList ()
	{
		return sInstance.mModelsList;
	}
	
	public static void setCurrModel(int modelId)
	{
		sInstance.setCurrModelInternel(modelId);
	}
	
	public static void addModel(String name)
	{
		sInstance.addNewModel(name);
	}
	
	public static boolean deriveData(String basePath)
	{
		return sInstance.deriveDataInternal(basePath);
	}
	
	public static void delModel(int id)
	{
		sInstance.delModelInternal(id);
	}
	public static void renameModel(int id , String name)
	{
		sInstance.renameModelInternal(id , name);
	}
	public static void notifySettingsInited()
	{
		sInstance.notifyInited();
	}
	
	public static void updateModelProtocol(int model_id , int policy_id)
	{
		sInstance.updateModelProtocolInternel(model_id, policy_id);
	}
	
	private void updateModelProtocolInternel(int model_id , int policy_id)
	{
		Model m = findModelById(model_id);
		
		if(m == null) {
			Log.w("updateModelProtocolInternel failed");
			return ;
		}
		
		m.mPolicyId = policy_id;
		if(!saveModels(mContext.getFilesDir()+"/models.xml")) {
			Log.e("models save failed");
		}
	}
	private void setCurrModelInternel(int id) 
	{
		Model m = findModelById(id);
		
		if(m == null) {
			Log.w("set currModel failed");
			return ;
		}
		sInstance.mCurrModel = m;
		
		sInstance.mPref.edit().putInt(CURR_MODEL_ID, id).commit();
		for(SettingsChangedListener temp:sInstance.mListeners) {
			temp.onCurrModelChanged(sInstance.mCurrModel);
		}
	}
	
	private void notifyInited()
	{
		for(SettingsChangedListener listener : mListeners) {
			listener.onCurrModelChanged(mCurrModel);
			listener.onModelsChanged(mModelsList);
		}
	}
	private void renameModelInternal(int id , String name)
	{
		Model m = findModelById(id);
		if(m != null) {
			m.name = name;
			updateModles();
		}
	}
	private void delModelInternal(int id)
	{
		Model m = findModelById(id);
		if(m != null) {
			mModelsList.remove(m);
			updateModles();
		}
	}
	private void addNewModel(String name)
	{
		int id = mCurrMaxId;
		
		do {
			id += 1;
		}while(mModelsList.contains(findModelById(id)));
		
		mCurrMaxId = id;
		mPref.edit().putInt("MAX_ID", mCurrMaxId).commit();
		
		Model m = new Model();
		m.mId = id;
		m.mPolicyId = 0;
		m.name = name;
		mModelsList.add(m);
		updateModles();
	}
	
	private void updateModles()
	{
		if(!saveModels(mContext.getFilesDir()+"/models.xml")) {
			Log.e("models save failed");
		}
		
		for(SettingsChangedListener listener : mListeners) {
			listener.onModelsChanged(mModelsList);
		}
	}
	
	private boolean deriveDataInternal(String basePath)
	{
		String path = basePath+"/"+"models.xml";
		return saveModels(path);
	}
	
	private Model findModelById(int id)
	{
		Model m = null;
		for(Model temp:mModelsList) {
			if(temp.mId == id) {
				m = temp;
				break;
			}
		}
		
		return m;
	}
	
	private boolean getModels(InputStream input)
	{
		boolean ret = true;
		mModelsList.clear();
		try {
			DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();  
			DocumentBuilder builder = factory.newDocumentBuilder();  
			Document document = builder.parse(input);  
			Element root = document.getDocumentElement();  
          
			NodeList modelNodes = root.getElementsByTagName("model");  
			for(int i=0;i<modelNodes.getLength();i++){  
				Element modelElement = (Element) modelNodes.item(i);
				int id = Integer.parseInt(modelElement.getAttribute("id"));
				int policy_id = Integer.parseInt(modelElement.getAttribute("policy_id"));
				String name = modelElement.getAttribute("name");
				if(id > mCurrMaxId) mCurrMaxId = id;
				Model m = new Model();
				m.mId = id;
				m.mPolicyId = policy_id;
				m.name = name;
				mModelsList.add(m);
				Log.d("add model id="+id+" policy_id="+policy_id+" name="+name);
				NodeList childNodes = modelElement.getChildNodes();  

				for(int j=0;j<childNodes.getLength();j++){  
					if(childNodes.item(j).getNodeType()==Node.ELEMENT_NODE){  
						if("data".equals(childNodes.item(j).getNodeName())){  
					}
                }  
            }//end for j  
        }//end for i
			mPref.edit().putInt(MAX_ID, mCurrMaxId).commit();
	}catch(Exception e){
		e.printStackTrace();
		ret = false;
	}
	return ret;
}
	
	private boolean  saveModels(String filePath)
	{
		boolean ret = true;
		File outFile = new File(filePath);
		if(!outFile.exists()) {
			try {
				outFile.createNewFile();
			} catch (IOException e1) {
				// TODO Auto-generated catch block
				e1.printStackTrace();
				return false;
			}
		}
		
		StringWriter xmlWriter = new StringWriter();
        OutputStream outStream = null;
        OutputStreamWriter outStreamWriter = null;
		
        try {  
        	
            outStream = new FileOutputStream(outFile);  
            outStreamWriter = new OutputStreamWriter(outStream);  
            
            XmlPullParserFactory pullFactory;  
            pullFactory = XmlPullParserFactory.newInstance();  
            XmlSerializer xmlSerializer = pullFactory.newSerializer();  
            //XmlSerializer xmlSerializer = Xml.newSerializer();    
            xmlSerializer.setOutput(xmlWriter);  
            //<?xml version='1.0' encoding='UTF-8' standalone='yes' ?>  
            xmlSerializer.startDocument("UTF-8", true);  
            //<models version="1">  
            xmlSerializer.startTag("", "models");  
            xmlSerializer.attribute("", "version", "1");  
            for(Model model : mModelsList)  
            {  
                //<model id="*" name="*"/>  
                xmlSerializer.startTag("", "model");  
                xmlSerializer.attribute("", "id", ""+model.mId);
                xmlSerializer.attribute("", "policy_id", ""+model.mPolicyId);
                xmlSerializer.attribute("", "name", model.name);
                xmlSerializer.endTag("", "model");  
            }  
            //</models>  
            xmlSerializer.endTag("", "models");  
            xmlSerializer.endDocument();  
            
            outStreamWriter.write(xmlWriter.toString());  
            outStreamWriter.close();  
            outStream.close();  
        } catch (Exception e) {  
            // TODO Auto-generated catch block  
            e.printStackTrace();  
            ret = false;
        }finally {
			try {
				if(outStreamWriter != null) outStreamWriter.close();
				if(outStream != null) outStream.close(); 
			} catch (IOException e){}  
        }
        return ret;
	}
	
}
