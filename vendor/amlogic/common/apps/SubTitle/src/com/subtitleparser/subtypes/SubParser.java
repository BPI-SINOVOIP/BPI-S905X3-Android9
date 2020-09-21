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
//import java.util.StringTokenizer;
//import java.util.regex.Matcher;
//import java.util.regex.Pattern;
//
//import com.subtitleparser.MalformedSubException;
//import com.subtitleparser.SubtitleFile;
//import com.subtitleparser.SubtitleLine;
//import com.subtitleparser.SubtitleParser;
//import com.subtitleparser.SubtitleTime;
//
//
///**
//* a .SUB subtitle parser.
//*
//* @author
//*/
//public class SubParser implements SubtitleParser{
//
//  public SubtitleFile parse(String inputString) throws MalformedSubException{
//      try{
//          String n="\\"+System.getProperty("line.separator");
//          String tmpText="";
//          SubtitleFile sf=new SubtitleFile();
//          SubtitleLine sl=null;
//
//          //SUB regexp
//          Pattern p = Pattern.compile(
//          "\\{(\\d+)\\}\\s*\\{(\\d+)\\}(.*?)"+n
//          );
//          Matcher m = p.matcher(inputString);
//          int occ=0;
//          while(m.find()){
//              int i=0;
//              StringTokenizer st = new StringTokenizer(m.group(3),"|");
//              while (st.hasMoreTokens()) {
//                  i++;
//                  if(i>1){
//                      tmpText=tmpText+"\n"+st.nextToken();
//                  }else{tmpText=st.nextToken();}
//              }
//              st=null;
//
//              occ++;
//              sl=new SubtitleLine(occ,
//              new SubtitleTime(Integer.parseInt(m.group(1))),//start
//              new SubtitleTime(Integer.parseInt(m.group(2))),//end
//              tmpText //text
//              );
//              tmpText="";
//              sf.add(sl);
//          }
//          return sf;
//      }catch(Exception e){throw new MalformedSubException(e.toString());}
//  };
//  public SubtitleFile parse(String inputString,String st2) throws MalformedSubException{
//      return null;
//  };
//
//}
