/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
//package com.subtitleparser.subtypes;
//
//import java.util.regex.Matcher;
//import java.util.regex.Pattern;
//
//import android.util.Log;
//
//import com.subtitleparser.MalformedSubException;
//import com.subtitleparser.SubtitleFile;
//import com.subtitleparser.SubtitleLine;
//import com.subtitleparser.SubtitleParser;
//import com.subtitleparser.SubtitleTime;
//
///**
//* a .SUB subtitle parser.
//*
//* @author jeff.yang
//*/
//public class SamiParser implements SubtitleParser{
//
//  public SubtitleFile parse(String inputString) throws MalformedSubException{
//      try{
//          String n="\\"+System.getProperty("line.separator");
//          String tmpText="";
//          SubtitleFile sf=new SubtitleFile();
//          SubtitleLine sl=null;
//
//          //Sami regexp
//          Pattern p = Pattern.compile(
//                  "<SYNC START=(\\d+)>.*"+n+".*<P .*>(.*?)"+n
//                 +".*<SYNC START=(\\d+)>.*"+n+".*<P .*>&nbsp;"+n,
//                  Pattern.CASE_INSENSITIVE
//          );
//
//          Matcher m = p.matcher(inputString);
//          int occ=0;
//          int s_msec=0, s_sec=0, s_min=0;
//          int e_msec=0, e_sec=0, e_min=0;
//          while(m.find()){
//              occ++;
//              s_msec =Integer.parseInt(m.group(1));
//              s_sec = s_msec/1000;
//              s_min = s_sec/60;
//              e_msec = Integer.parseInt(m.group(3));
//              e_sec = e_msec/1000;
//              e_min = e_sec/60;
//
//              sl=new SubtitleLine(occ,
//                      new SubtitleTime(s_min/60, //start time
//                              s_min%60,
//                              s_sec%60,
//                              s_msec%1000),
//                      new SubtitleTime(s_min/60, //end time
//                              e_min%60,
//                              e_sec%60,
//                              e_msec%1000),
//              m.group(2) //text
//              );
//              tmpText="";
//              sf.add(sl);
//          }
//          Log.i("SamiParser", "find"+sf.size());
//          return sf;
//      }catch(Exception e)
//      {
//          Log.i("SamiParser", "------------!!!!!!!parse file err!!!!!!!!");
//          throw new MalformedSubException(e.toString());
//      }
//  };
//  public SubtitleFile parse(String inputString,String st2) throws MalformedSubException{
//      return null;
//  };
//
//}
