/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
package com.subtitleparser.subtypes;

import java.io.*;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.util.Arrays;

import android.graphics.Bitmap;
import android.graphics.Bitmap.*;
import android.graphics.Bitmap.Config;
import android.util.Log;

import com.subtitleparser.MalformedSubException;
import com.subtitleparser.SubData;
import com.subtitleparser.Subtitle;
import com.subtitleparser.SubtitleApi;
import com.subtitleparser.SubtitleParser;

class InSubApi extends SubtitleApi {
        native  RawData getrawdata (int millisec);
        native int setInSubtitleNumberByJni (int  ms, String filename);
        native  void closeInSub();
        native  int getInSubType();

        private final int SUBTITLE_VOB = 1;
        private final int SUBTITLE_PGS = 2;
        private final int SUBTITLE_MKV_STR = 3;
        private final int SUBTITLE_MKV_VOB = 4;
        private final int SUBTITLE_SSA = 5;
        private final int SUBTITLE_DVB = 6;
        private final int SUBTITLE_TMD_TXT = 7;
        private final int SUBTITLE_DVB_TELETEXT = 9;
        private static final String SUBTITLE_FILE = "/data/subtitle.db";
        private static final String WRITE_SUBTITLE_FILE = "/data/subtitle_img.jpeg";
        private static int subtitle_packet_size = (4 + 1 + 4 + 4 + 2 + 2 + 2 + 2 + 2 + 4 + 720 * 576 / 4);
        public  static int subtitle_file_position = 0;
        private static int sync_byte = 0;
        private static int packet_size = 0;
        private static int read_bytes = 0;
        private static int subtitle_width = 0;
        private static int subtitle_height = 0;
        private static int subtitle_alpha = 0;
        private static int subtitle_pts = 0;
        public  static int subtitle_delay = 0;
        private static RawData inter_data = null;
        private static Bitmap bf_show = null;

        private Bitmap bitmap = null;
        private String filename = null;
        int index = 0;
        InSubApi (String file1, int id) {
            filename = file1;
            index = id;
            //setInSubtitleNumberByJni (index, filename);
        };
        public int getSubTypeDetial() {
            Log.i ("InSubApi",   "getInSubType():" + getInSubType());
            return getInSubType();
        }
        public void closeSubtitle() {
            Log.i ("InSubApi",   "closeSubtitle");
            closeInSub();
        }
        public Subtitle.SUBTYPE type() {
            return Subtitle.SUBTYPE.INSUB;
        }
        public SubData getdata (int millisec) {
            //add  value to bitmap
            //add  value to begingtime,endtime
            Log.i ("InSubApi",   "[getdata]millisec:"+millisec);
            inter_data = getrawdata (millisec);
            //cause delay time always is zero for pgs format, so we should hadle it with special method
            /*if(getInSubType() == 0) {
                return null;
            }
            else */if(getInSubType() == SUBTITLE_PGS || getInSubType() == SUBTITLE_DVB || getInSubType() == SUBTITLE_TMD_TXT || getInSubType() == SUBTITLE_DVB_TELETEXT) {
                if (inter_data != null) {
                    if (inter_data.sub_start > 0) {
                        if (inter_data.sub_size > 0) {
                            if (inter_data.type == 1) { //graphic subtitle)
                                bf_show = Bitmap.createBitmap (inter_data.rawdata, inter_data.width,
                                                               inter_data.height, Config.ARGB_8888);
                                Log.i ("getdata", "[Bitmap]start time:" + millisec + ",delay time:" + inter_data.sub_delay);
                                return new SubData (bf_show, inter_data.sub_start, inter_data.sub_delay, inter_data.sub_size, inter_data.origin_width, inter_data.origin_height);
                            }
                            else {
                                Log.i ("getdata", "[text]start time:" + inter_data.sub_start + ",delay time:" + inter_data.sub_delay);
                                return new SubData (inter_data.subtitlestring, inter_data.sub_start, inter_data.sub_delay, inter_data.sub_size);
                            }
                        }
                        else if (inter_data.sub_size == 0) {
                            if (inter_data.type == 1) { //graphic subtitle)
                                Log.i ("getdata", "sub_size=0,[Bitmap]start time:" + millisec + ",delay time:" + inter_data.sub_delay);
                                //bf_show = null;
                                bf_show = Bitmap.createBitmap (1, 1, Config.ARGB_8888);
                                return new SubData (bf_show, inter_data.sub_start, inter_data.sub_delay, inter_data.sub_size, 0, 0);
                            }
                            else {
                                Log.i ("getdata", "sub_size=0,[text]start time:" + inter_data.sub_start + ",delay time:" + inter_data.sub_delay);
                                return new SubData ("", inter_data.sub_start, inter_data.sub_delay, inter_data.sub_size);
                            }
                        }
                        else {
                            return null;
                        }
                    }
                    else {
                        return null;
                    }
                }
                else {
                    return null;
                }
            }
            else {
                if (inter_data != null) {
                    if (inter_data.type == 1) {
                        bf_show = Bitmap.createBitmap (inter_data.rawdata, inter_data.width,
                                                       inter_data.height, Config.ARGB_8888);
                        Log.i ("SubData",    "time b: " + millisec + "  e:" + inter_data.sub_delay);
                        if (inter_data.sub_delay > millisec) {
                            return new SubData (bf_show, millisec, inter_data.sub_delay);
                        }
                        else {
                            return new SubData (bf_show, millisec, millisec + 1500);
                        }
                    }
                    else {
                        Log.i ("InSubApi",   "get SubData by string  delay time :" + inter_data.sub_delay);
                        if (inter_data.sub_delay > millisec) {
                            return new SubData (inter_data.subtitlestring, millisec, inter_data.sub_delay);
                        }
                        else {
                            return new SubData (inter_data.subtitlestring, millisec, millisec + 1500);
                        }
                    }
                }
                else {
                    Log.i ("InSubApi",   "get subdata return null");
                    //          int[] data = new int[1];
                    //          Arrays.fill(data, 0x00000000);
                    //          bitmap= Bitmap.createBitmap( data,  1,  1, Config.ARGB_8888  ) ;
                    //          return new SubData( bitmap, millisec, millisec+300);
                    return null;
                }
            }
        };

        // 2-byte number
        private static int SHORT_little_endian_TO_big_endian (int i) {
            return ( (i >> 8) & 0xff) + ( (i << 8) & 0xff00);
        }

        // 4-byte number
        private static int INT_little_endian_TO_big_endian (int i) {
            return ( (i & 0xff) << 24) + ( (i & 0xff00) << 8) + ( (i & 0xff0000) >> 8) + ( (i >> 24) & 0xff);
        }
        public static Bitmap getBitmap (int tick)  {
            try {
                File f = new File (SUBTITLE_FILE);
                RandomAccessFile raf = new RandomAccessFile (f, "r");
                //seek to subtitle packet
                raf.seek (subtitle_file_position * subtitle_packet_size);
                FileChannel fc = raf.getChannel();
                sync_byte = raf.readInt();
                //Log.i(LOG_TAG,"Subtitle file first four bytes are: " + sync_byte);
                sync_byte = INT_little_endian_TO_big_endian (sync_byte);
                //Log.i(LOG_TAG,"Subtitle file first four bytes reverse are: " + sync_byte);
                raf.skipBytes (1);
                subtitle_pts = INT_little_endian_TO_big_endian (raf.readInt());
                //Log.i(LOG_TAG,"millisec is : " + millisec);
                //Log.i(LOG_TAG,"subtitle_pts is : " + subtitle_pts);
                if (tick * 90 < subtitle_pts) {
                    return null;
                }
                subtitle_delay = INT_little_endian_TO_big_endian (raf.readInt());
                raf.skipBytes (4);
                subtitle_width = SHORT_little_endian_TO_big_endian (raf.readShort());
                subtitle_height = SHORT_little_endian_TO_big_endian (raf.readShort());
                subtitle_alpha = SHORT_little_endian_TO_big_endian (raf.readShort());
                int RGBA_Pal[] = new int[4];
                RGBA_Pal[0] = RGBA_Pal[1] = RGBA_Pal[2] = RGBA_Pal[3] = 0;
                if ( (subtitle_alpha & 0xff0) != 0) {
                    RGBA_Pal[2] = 0xffffffff;
                    RGBA_Pal[1] = 0xff;
                }
                else if ( (subtitle_alpha & 0xfff0) != 0) {
                    RGBA_Pal[1] = 0xff;
                    RGBA_Pal[2] = 0xffffffff;
                    RGBA_Pal[3] = 0xff;
                }
                else if ( (subtitle_alpha & 0xf0f0) != 0) {
                    RGBA_Pal[1] = 0xffffffff;
                    RGBA_Pal[3] = 0xff;
                }
                else if ( (subtitle_alpha & 0xff00) != 0) {
                    RGBA_Pal[2] = 0xffffffff;
                    RGBA_Pal[3] = 0xff;
                }
                else {
                    RGBA_Pal[1] = 0xffffffff;
                    RGBA_Pal[3] = 0xff;
                }
                packet_size = raf.readInt();
                //Log.i(LOG_TAG,    "packet size is : " + packet_size);
                packet_size = INT_little_endian_TO_big_endian (packet_size);
                //Log.i(LOG_TAG,    "packet size reverse is : " + packet_size);
                //allocate equal size buffer
                ByteBuffer bf = ByteBuffer.allocate (packet_size);
                read_bytes = fc.read (bf, subtitle_file_position * subtitle_packet_size + 23);
                //Log.i(LOG_TAG,    "read subtitle packet size is : " + read_bytes);
                int i = 0, j = 0, n = 0, index = 0, index1 = 0, data_byte = 0, buffer_width = 0;
                buffer_width = (subtitle_width + 63) & 0xffffffc0;
                data_byte = ( ( (subtitle_width * 2) + 15) >> 4) << 1;
                bf_show = Bitmap.createBitmap (buffer_width, subtitle_height, Config.ARGB_8888);
                //ByteBuffer bf_show = ByteBuffer.allocate(buffer_width*subtitle_height*4);
                byte[] data = new byte[200];
                //Log.i(LOG_TAG,    "subtitle_width  is : " + subtitle_width);
                //Log.i(LOG_TAG,    "subtitle_height  is : " + subtitle_height);
                //Log.i(LOG_TAG,    "biffer_width  is : " + buffer_width);
                //Log.i(LOG_TAG,    "data_byte  is : " + data_byte);
                for (i = 0; i < subtitle_height; i++) {
                    if ( (i & 1) != 0) {
                        bf.position ( (i >> 1) *data_byte + (720 * 576 / 8));
                        bf.get (data, 0, data_byte);
                    }
                    else {
                        bf.position ( (i >> 1) *data_byte);
                        bf.get (data, 0, data_byte);
                    }
                    index = 0;
                    for (j = 0; j < subtitle_width; j++) {
                        index1 = ( (index % 2) > 0) ? (index - 1) : (index + 1);
                        n = data[index1];
                        index++;
                        if (n != 0) {
                            bf_show.setPixel (j, i, RGBA_Pal[ (n >> 4) & 0x3]);
                            //result_buf[i*(buffer_width)+j] = RGBA_Pal[(n>>4)&0x3];
                            if (++j >= subtitle_width) { break; }
                            bf_show.setPixel (j, i, RGBA_Pal[ (n >> 6) & 0x3]);
                            //result_buf[i*(buffer_width)+j] = RGBA_Pal[n>>6];
                            if (++j >= subtitle_width) { break; }
                            bf_show.setPixel (j, i, RGBA_Pal[n & 0x3]);
                            //result_buf[i*(buffer_width)+j] = RGBA_Pal[n&0x3];
                            if (++j >= subtitle_width) { break; }
                            bf_show.setPixel (j, i, RGBA_Pal[ (n >> 2) & 0x3]);
                            //result_buf[i*(buffer_width)+j] = RGBA_Pal[(n>>2)&0x3];
                        }
                        else {
                            j += 3;
                        }
                    }
                }
                raf.close();
                subtitle_file_position++;
                if (subtitle_file_position >= 100) {
                    subtitle_file_position = 0;
                }
                //Log.i(LOG_TAG,    "end draw bitmap ");
                //invalidate();
                return bf_show;
            }
            catch (FileNotFoundException ex) {
                ex.printStackTrace();
            }
            catch (IllegalArgumentException ex) {
                ex.printStackTrace();
            }
            catch (IOException ex) {
                ex.printStackTrace();
            }
            return null;
        }
}




public class InSubParser implements SubtitleParser {

        public SubtitleApi parse (String filename, int index) throws MalformedSubException {
            Log.i ("SubtitleApi", "------------SubtitleApi-----------");
            return new InSubApi (filename, index);
        };


        public SubtitleApi parse (String inputString, String st2) throws MalformedSubException {
            return null;
        };
}