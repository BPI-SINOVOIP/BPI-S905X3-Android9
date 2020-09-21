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
//
///**
//* a .SRT subtitle parser.
//*
//* @author
//*/
//public class SrtParser implements SubtitleParser{
//
//  public SubtitleFile parse(String inputString) throws MalformedSubException{
//      try{
//          Log.d("SrtParser","----------begin  parse ---------");
//
//          String n="\\"+System.getProperty("line.separator");
//          String l=".*?"+n+""; //regex for a text line
//          SubtitleFile sf=new SubtitleFile();
//          SubtitleLine sl=null;
//          //SRT regexp
//          //(?s) DOTALL MODE
//          Pattern p = Pattern.compile(
//          "(?s)(\\d+)\\s*"+n+"(\\d\\d):(\\d\\d):(\\d\\d),(\\d\\d\\d)\\s+-->"+
//          "\\s+(\\d\\d):(\\d\\d):(\\d\\d),(\\d\\d\\d)"+n+"(.*?)"+n+n
//          );
//          Matcher m = p.matcher(inputString);
//          while(m.find()){
//
//              sl=new SubtitleLine(Integer.parseInt(m.group(1)), //nLine
//              new SubtitleTime(Integer.parseInt(m.group(2)), //start time
//                  Integer.parseInt(m.group(3)),
//                  Integer.parseInt(m.group(4)),
//                  Integer.parseInt(m.group(5))),
//              new SubtitleTime(Integer.parseInt(m.group(6)), //end time
//                  Integer.parseInt(m.group(7)),
//                  Integer.parseInt(m.group(8)),
//                  Integer.parseInt(m.group(9))),
//              m.group(10) //text
//              );
//
//              sf.add(sl);
//          }
//          return sf;
//      }catch(Exception e)
//      {
//          Log.d("SrtParser","----------parse err---------");
//          throw new MalformedSubException(e.toString());
//      }
//  };
//  public SubtitleFile parse(String inputString,String st2) throws MalformedSubException{
//      return null;
//  };
//}
