/******************************************************************
*
*Copyright (C) 2012  Amlogic, Inc.
*
*Licensed under the Apache License, Version 2.0 (the "License");
*you may not use this file except in compliance with the License.
*You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
*Unless required by applicable law or agreed to in writing, software
*distributed under the License is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*See the License for the specific language governing permissions and
*limitations under the License.
******************************************************************/
package com.droidlogic.FileBrower;

import java.io.Closeable;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.List;
import java.util.Collections;
import java.util.Map;
import java.util.Iterator;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import java.io.InputStream;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.os.Message;
import android.os.StatFs;
///import android.os.storage.StorageManager;
//import android.os.storage.VolumeInfo;
import android.widget.TextView;
import android.util.Log;
import android.net.Uri;

import com.droidlogic.FileBrower.FileBrowerDatabase.FileMarkCursor;
import com.droidlogic.app.FileListManager;

public class FileOp {
    public static File copying_file = null;
    public static boolean copy_cancel = false;
    public static boolean switch_mode = false;
    public static boolean IsBusy = false;
    public static String source_path = null;
    public static String target_path = null;
    private static Context mContext;
    public static final String KEY_PATH = "key_path";
    public static void SetMode(boolean value){
        switch_mode = value;
    }
    public static boolean GetMode(){
        return switch_mode;
    }
    public static FileOpTodo file_op_todo = FileOpTodo.TODO_NOTHING;
    public static enum FileOpTodo{
        TODO_NOTHING,
        TODO_CPY,
        TODO_CUT
    }
    public static enum FileOpReturn{
        SUCCESS,
        ERR,
        ERR_NO_FILE,
        ERR_DEL_FAIL,
        ERR_CPY_FAIL,
        ERR_CUT_FAIL,
        ERR_PASTE_FAIL
    }

    /** getFileSizeStr */
    public static String getFileSizeStr(long length) {
        int sub_index = 0;
        String sizeStr = "";
        if (length >= 1073741824) {
            sub_index = (String.valueOf((float)length/1073741824)).indexOf(".");
            sizeStr = ((float)length/1073741824+"000").substring(0,sub_index+3)+" GB";
        } else if (length >= 1048576) {
            sub_index = (String.valueOf((float)length/1048576)).indexOf(".");
            sizeStr =((float)length/1048576+"000").substring(0,sub_index+3)+" MB";
        } else if (length >= 1024) {
            sub_index = (String.valueOf((float)length/1024)).indexOf(".");
            sizeStr = ((float)length/1024+"000").substring(0,sub_index+3)+" KB";
        } else if (length < 1024) {
            sizeStr = String.valueOf(length)+" B";
        }
        return sizeStr;
    }

    /** getFileTypeImg */
    public static Object getFileTypeImg(String filename) {
        if (FileListManager.isMusic(filename)) {
            return R.drawable.item_type_music;
        } else if (FileListManager.isPhoto(filename)) {
            return R.drawable.item_type_photo;
        } else if (FileListManager.isVideo(filename)) {
            return R.drawable.item_type_video;
        } else if (FileListManager.isApk(filename)) {
            return R.drawable.item_type_apk;
        } else
            return R.drawable.item_type_file;
    }
    public static Object getThumbImage(String filename) {
        if (FileListManager.isMusic(filename)) {
            return R.drawable.item_preview_music;
        } else if (FileListManager.isPhoto(filename)) {
            return R.drawable.item_preview_photo;
        } else if (FileListManager.isVideo(filename)) {
            return R.drawable.item_preview_video;
        } else if (FileListManager.isApk(filename)) {
            return R.drawable.item_preview_apk;
        } else
            return R.drawable.item_preview_file;
    }

    public static int getThumbDeviceIcon(Context c,String device_name){
        if (device_name.equals("flash")) {
            return R.drawable.memory_default;
        }
        else if (device_name.equals("sata")) {
            return R.drawable.sata_default;
        }
        else if (device_name.equals("sdcard")) {
            return R.drawable.sdcard_default;
        }
        else if (device_name.equals("usb")) {
            return R.drawable.usb_default;
        }
        return R.drawable.noname_file_default;
    }
    public static boolean deviceExist(String string) {
        // TODO Auto-generated method stub
        if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
            File sdCardDir = Environment.getExternalStorageDirectory();
        }
        return true;
    }
    public static String getShortName(String file){
        File file_path = new File(file);
        String filename = file_path.getName();
        String temp_str = null;
        if (file_path.isDirectory()) {
            if (filename.length()>18) {
                temp_str = String.copyValueOf((filename.toCharArray()), 0, 15);
                temp_str = temp_str +"...";
            }
            else {
                temp_str = filename;
            }
        }
        else {
            if (filename.length()>18) {
                int index = filename.lastIndexOf(".");
                if ((index != -1) && (index < filename.length())) {
                    String suffix = String.copyValueOf((filename.toCharArray()), index, (filename.length()-index));
                    if (index >= 15) {
                        temp_str = String.copyValueOf((filename.toCharArray()), 0, 15);
                    } else {
                        temp_str = String.copyValueOf((filename.toCharArray()), 0, index);
                    }
                    temp_str = temp_str + "~" + suffix;
                } else {
                    temp_str = String.copyValueOf((filename.toCharArray()), 0, 15);
                    temp_str = temp_str + "~";
                }
            }
            else {
                temp_str = filename;
            }
        }
        return temp_str;
    }
    /** check file sel status */
    public static boolean isFileSelected(String file_path,String cur_page) {
        if (cur_page.equals("list")) {
            if (FileBrower.db == null) return false;
            try {
                FileBrower.myCursor = FileBrower.db.getFileMarkByPath(file_path);
                if (FileBrower.myCursor.getCount() > 0) {
                    return true;
                }
            } finally {
                FileBrower.myCursor.close();
            }
        }
        else if (cur_page.equals("thumbnail1")) {
            if (ThumbnailView1.db == null) return false;
            try {
                ThumbnailView1.myCursor = ThumbnailView1.db.getFileMarkByPath(file_path);
                if (ThumbnailView1.myCursor.getCount() > 0) {
                    return true;
                }
            } finally {
                ThumbnailView1.myCursor.close();
            }
        }
        return false;
    }

    public static Bitmap fitSizePic(File f){
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inJustDecodeBounds = true;
        Bitmap bitmap = BitmapFactory.decodeFile(f.getPath(), options);

        int samplesize = (int) (options.outHeight / 96);
        if (samplesize <= 0) samplesize = 1;

        options.inSampleSize = samplesize;
        options.inJustDecodeBounds = false;
        bitmap = BitmapFactory.decodeFile(f.getPath(), options);

        return bitmap;
    }

    /** update file sel status
     * 1: add to mark table 0: remove from mark table
    */
    public static void cleanFileMarks(String cur_page) {
        if (cur_page.equals("list")) {
            if (FileBrower.db != null) {
                FileMarkCursor cc = null;
                try {
                    cc = FileBrower.db.getFileMark();
                    if (cc != null && cc.moveToFirst()) {
                        if (cc.getCount() > 0) {
                            for (int i = 0; i < cc.getCount(); i++) {
                                cc.moveToPosition(i);
                                String file_path = cc.getColFilePath();
                                if (file_path != null) {
                                    if (!new File(file_path).exists()) {
                                        FileBrower.db.deleteFileMark(file_path);
                                    }
                                }
                            }
                        }
                    }
                } finally {
                    if (cc != null) cc.close();
                }
            }
        } else if (cur_page.equals("thumbnail1")) {
            if (ThumbnailView1.db != null) {
                FileMarkCursor cc = null;
                try {
                    cc = ThumbnailView1.db.getFileMark();
                    if (cc != null && cc.moveToFirst()) {
                        if (cc.getCount() > 0) {
                            for (int i = 0; i < cc.getCount(); i++) {
                                cc.moveToPosition(i);
                                String file_path = cc.getColFilePath();
                                if (file_path != null) {
                                    if (!new File(file_path).exists()) {
                                        ThumbnailView1.db.deleteFileMark(file_path);
                                    }
                                }
                            }
                        }
                    }
                } finally {
                    if (cc != null) cc.close();
                }
            }
        }
    }
    public static void updateFileStatus(String file_path, int status, String cur_page) {
        if (cur_page.equals("list")) {
            if (FileBrower.db == null) return;
            if (status == 1) {
                try {
                    FileBrower.myCursor = FileBrower.db.getFileMarkByPath(file_path);
                    if (FileBrower.myCursor.getCount() <= 0) {
                        FileBrower.db.addFileMark(file_path, 1);
                    }
                } finally {
                    FileBrower.myCursor.close();
                }
            } else {
                FileBrower.db.deleteFileMark(file_path);
            }
        } else if (cur_page.equals("thumbnail1")) {
            if (ThumbnailView1.db == null) return;
            if (status == 1) {
                try {
                    ThumbnailView1.myCursor = ThumbnailView1.db.getFileMarkByPath(file_path);
                    if (ThumbnailView1.myCursor.getCount() <= 0) {
                        ThumbnailView1.db.addFileMark(file_path, 1);
                    }
                } finally {
                    ThumbnailView1.myCursor.close();
                }
            } else {
                ThumbnailView1.db.deleteFileMark(file_path);
            }
        }
    }

    /** cut/copy/paste/delete selected files*/
    public static FileOpReturn cutSelectedFile() {
        return FileOpReturn.ERR;
    }
    public static FileOpReturn copySelectedFile() {
        return FileOpReturn.ERR;
    }
    private static void nioTransferCopy(File source, File target) {
        FileChannel in = null;
        FileChannel out = null;
        FileInputStream inStream = null;
        FileOutputStream outStream = null;
        try {
            inStream = new FileInputStream(source);
            outStream = new FileOutputStream(target);
            in = inStream.getChannel();
            out = outStream.getChannel();
            in.transferTo(0, in.size(), out);
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            close(inStream);
            close(in);
            close(outStream);
            close(out);
        }
    }
    private static void nioBufferCopy(File source, File target, String cur_page, int buf_size) {
        if (!source.exists() || !target.exists())
            return;

        FileChannel in = null;
        FileChannel out = null;
        FileInputStream inStream = null;
        FileOutputStream outStream = null;
        try {
            source_path = source.getPath();
            target_path = target.getPath();
            inStream = new FileInputStream(source);
            outStream = new FileOutputStream(target);
            in = inStream.getChannel();
            out = outStream.getChannel();

            ByteBuffer buffer = ByteBuffer.allocate(1024 * buf_size);
            long bytecount = 0;
            int byteread = 0;
            while (!copy_cancel && ((byteread = in.read(buffer)) != -1)) {
                if (copy_cancel) {
                    break;
                }
                buffer.flip();
                out.write(buffer);
                buffer.clear();
                bytecount += byteread;
                if (cur_page.equals("list")) {
                    FileBrower.mProgressHandler.sendMessage(Message.obtain(
                        FileBrower.mProgressHandler, 1, (int)(bytecount * 100 / source.length()), 0));
                } else if (cur_page.equals("thumbnail1")){
                    ThumbnailView1.mProgressHandler.sendMessage(Message.obtain(
                        ThumbnailView1.mProgressHandler, 1, (int)(bytecount * 100 / source.length()), 0));
                }
            }
            source_path = null;
            target_path = null;
            out.force(true);
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            close(inStream);
            close(in);
            close(outStream);
            close(out);
        }
    }
    private static void close(Closeable closable) {
        if (closable != null) {
            try {
                closable.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }
    public static FileOpReturn pasteSelectedFile(String cur_page) {
        ArrayList<String> fileList = new ArrayList<String>();
        copy_cancel = false;
        IsBusy = true;
        if ((file_op_todo != FileOpTodo.TODO_CPY) &&
            (file_op_todo != FileOpTodo.TODO_CUT)) {
            if (cur_page.equals("list")) {
                FileBrower.mProgressHandler.sendMessage(Message.obtain(
                    FileBrower.mProgressHandler, 5));
            } else if (cur_page.equals("thumbnail1")) {
                ThumbnailView1.mProgressHandler.sendMessage(Message.obtain(
                    ThumbnailView1.mProgressHandler, 5));
            }
            IsBusy = false;
            return FileOpReturn.ERR;
        }

        if (cur_page.equals("list")) {
            if (FileBrower.cur_path.startsWith(FileListManager.STORAGE)) {
                if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED_READ_ONLY)) {
                    FileBrower.mProgressHandler.sendMessage(Message.obtain(
                        FileBrower.mProgressHandler, 7));
                    IsBusy = false;
                    return FileOpReturn.ERR;
                }
            }
            if (!checkCanWrite(FileBrower.cur_path)) {
                FileBrower.mProgressHandler.sendMessage(Message.obtain(
                    FileBrower.mProgressHandler, 7));
                IsBusy = false;
                return FileOpReturn.ERR;
            }
        } else if (cur_page.equals("thumbnail1")) {
            if (ThumbnailView1.cur_path.startsWith(FileListManager.STORAGE)) {
                if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED_READ_ONLY)) {
                    ThumbnailView1.mProgressHandler.sendMessage(Message.obtain(
                        ThumbnailView1.mProgressHandler, 7));
                    IsBusy = false;
                    return FileOpReturn.ERR;
                }
            }
            if (!checkCanWrite(ThumbnailView1.cur_path)) {
                ThumbnailView1.mProgressHandler.sendMessage(Message.obtain(
                    ThumbnailView1.mProgressHandler, 7));
                IsBusy = false;
                return FileOpReturn.ERR;
            }
        }
        try {
            if (cur_page.equals("list")) {
                FileBrower.myCursor = FileBrower.db.getFileMark();
                if (FileBrower.myCursor.getCount() > 0) {
                    for (int i=0; i<FileBrower.myCursor.getCount(); i++) {
                        FileBrower.myCursor.moveToPosition(i);
                        fileList.add(FileBrower.myCursor.getColFilePath());
                    }
                }
            } else if (cur_page.equals("thumbnail1")) {
                ThumbnailView1.myCursor = ThumbnailView1.db.getFileMark();
                if (ThumbnailView1.myCursor.getCount() > 0) {
                    for (int i=0; i<ThumbnailView1.myCursor.getCount(); i++) {
                        ThumbnailView1.myCursor.moveToPosition(i);
                        fileList.add(ThumbnailView1.myCursor.getColFilePath());
                    }
                }
            }
        } finally {
            if (cur_page.equals("list")) {
                FileBrower.myCursor.close();
            } else if (cur_page.equals("thumbnail1")) {
                ThumbnailView1.myCursor.close();
            }
        }

        if (!fileList.isEmpty()) {
            if (cur_page.equals("list")) {
                FileBrower.mProgressHandler.sendMessage(Message.obtain(
                    FileBrower.mProgressHandler, 3));
            } else if (cur_page.equals("thumbnail1")) {
                ThumbnailView1.mProgressHandler.sendMessage(Message.obtain(
                    ThumbnailView1.mProgressHandler, 3));
            }

            String curPathBefCopy=null;
            String curPathAftCopy=null;
            if (cur_page.equals("list")) {
                curPathBefCopy=FileBrower.cur_path;
            }
            else if (cur_page.equals("thumbnail1")) {
                curPathBefCopy=ThumbnailView1.cur_path;
            }
            else {
                Log.e("pasteSelectedFile","curPathBefCopy=null error.");
                IsBusy = false;
                return FileOpReturn.ERR;
            }

            for (int i = 0; i < fileList.size(); i++) {
                String name = fileList.get(i);
                File file = new File(name);
                if (copy_cancel) {
                    if (cur_page.equals("list")) {
                        FileBrower.mProgressHandler.sendMessage(Message.obtain(
                            FileBrower.mProgressHandler, 9));
                    } else if (cur_page.equals("thumbnail1")) {
                        ThumbnailView1.mProgressHandler.sendMessage(Message.obtain(
                            ThumbnailView1.mProgressHandler, 9));
                    }
                    IsBusy = false;
                    return FileOpReturn.ERR;
                }

                if (file.exists()) {
                    if (cur_page.equals("list")) {
                        if (file.length() > checkFreeSpace(FileBrower.cur_path)) {
                            FileBrower.mProgressHandler.sendMessage(Message.obtain(
                                FileBrower.mProgressHandler, 8));
                            IsBusy = false;
                            return FileOpReturn.ERR;
                        }
                    } else if (cur_page.equals("thumbnail1")) {
                        if (file.length() > checkFreeSpace(ThumbnailView1.cur_path)) {
                            ThumbnailView1.mProgressHandler.sendMessage(Message.obtain(
                                ThumbnailView1.mProgressHandler, 8));
                            IsBusy = false;
                            return FileOpReturn.ERR;
                        }
                    }

                    if (file.isDirectory()) {
                        try {
                            int idx=-1;
                            idx = (FileBrower.cur_path).indexOf(file.getPath());
                            if (idx != -1) {
                                if (cur_page.equals("list")) {
                                    FileBrower.mProgressHandler.sendMessage(Message.obtain(
                                        FileBrower.mProgressHandler, 11));
                                } else if (cur_page.equals("thumbnail1")) {
                                    ThumbnailView1.mProgressHandler.sendMessage(Message.obtain(
                                        ThumbnailView1.mProgressHandler, 11));
                                }
                                IsBusy = false;
                                return FileOpReturn.ERR;
                            }
                            else {
                                File file_new = null;
                                if (cur_page.equals("list")) {
                                    file_new = new File(FileBrower.cur_path + File.separator + file.getName());
                                } else if (cur_page.equals("thumbnail1")) {
                                    file_new = new File(ThumbnailView1.cur_path + File.separator + file.getName());
                                }

                                if (file_new.exists()) {
                                    String date = new SimpleDateFormat("yyyyMMddHHmmss_")
                                        .format(Calendar.getInstance().getTime());
                                    if (cur_page.equals("list")) {
                                        file_new = new File(FileBrower.cur_path + File.separator + date + file.getName());
                                    } else if (cur_page.equals("thumbnail1")) {
                                        file_new = new File(ThumbnailView1.cur_path + File.separator + date + file.getName());
                                    }
                                }

                                if (!file_new.exists()) {
                                    copying_file = file_new;
                                    FileUtils.setCurPage(cur_page);
                                    FileUtils.copyDirectoryToDirectory(file, new File(FileBrower.cur_path));

                                    if (!copy_cancel) {
                                        if (file_op_todo == FileOpTodo.TODO_CUT)
                                            FileUtils.deleteDirectory(file);
                                    } else {
                                        if (file_new.exists())
                                            FileUtils.deleteDirectory(file_new);

                                        if (cur_page.equals("list")) {
                                            FileBrower.mProgressHandler.sendMessage(Message.obtain(
                                                FileBrower.mProgressHandler, 9));
                                        } else if (cur_page.equals("thumbnail1")) {
                                            ThumbnailView1.mProgressHandler.sendMessage(Message.obtain(
                                                ThumbnailView1.mProgressHandler, 9));
                                        }
                                        IsBusy = false;
                                        return FileOpReturn.ERR;
                                    }
                                }
                            }
                        }
                        catch (Exception e) {
                            Log.e("Exception when copyDirectoryToDirectory",e.toString());
                        }
                    }
                    else {
                        try {
                            File file_new = null;
                            if (cur_page.equals("list")) {
                                file_new = new File(FileBrower.cur_path + File.separator + file.getName());
                            } else if (cur_page.equals("thumbnail1")) {
                                file_new = new File(ThumbnailView1.cur_path + File.separator + file.getName());
                            }

                            if (file_new.exists()) {
                                String date = new SimpleDateFormat("yyyyMMddHHmmss_")
                                    .format(Calendar.getInstance().getTime());
                                if (cur_page.equals("list")) {
                                    file_new = new File(FileBrower.cur_path + File.separator + date + file.getName());
                                } else if (cur_page.equals("thumbnail1")) {
                                    file_new = new File(ThumbnailView1.cur_path + File.separator + date + file.getName());
                                }
                            }

                            if (!file_new.exists()) {
                                file_new.createNewFile();
                                copying_file = file_new;
                                try {
                                    if (file.length() < 1024*1024*10)
                                        nioBufferCopy(file, file_new, cur_page, 4);
                                    else if (file.length() < 1024*1024*100)
                                        nioBufferCopy(file, file_new, cur_page, 1024);
                                    else
                                        nioBufferCopy(file, file_new, cur_page, 1024*10);
                                    if (!copy_cancel) {
                                        if (file_op_todo == FileOpTodo.TODO_CUT)
                                            file.delete();
                                        }
                                    else {
                                        if (file_new.exists())
                                            file_new.delete();

                                        if (cur_page.equals("list")) {
                                            FileBrower.mProgressHandler.sendMessage(Message.obtain(
                                                FileBrower.mProgressHandler, 9));
                                        } else if (cur_page.equals("thumbnail1")) {
                                            ThumbnailView1.mProgressHandler.sendMessage(Message.obtain(
                                                ThumbnailView1.mProgressHandler, 9));
                                        }
                                        IsBusy = false;
                                        return FileOpReturn.ERR;
                                    }
                                } catch (Exception e) {
                                    Log.e("Exception when copy file", e.toString());
                                }
                            }
                        } catch (Exception e) {
                            Log.e("Exception when delete file", e.toString());
                        }

                        if (cur_page.equals("list")) {
                            FileBrower.mProgressHandler.sendMessage(Message.obtain(
                                FileBrower.mProgressHandler, 2, (i+1) * 100 / fileList.size(), 0));
                        } else if (cur_page.equals("thumbnail1")) {
                            ThumbnailView1.mProgressHandler.sendMessage(Message.obtain(
                                ThumbnailView1.mProgressHandler, 2, (i+1) * 100 / fileList.size(), 0));
                        }
                    }
                }
            }

            //make sure current path is the destination path, otherwise indicate copy fail
            if (cur_page.equals("list")) {
                curPathAftCopy=FileBrower.cur_path;
            }
            else if (cur_page.equals("thumbnail1")) {
                curPathAftCopy=ThumbnailView1.cur_path;
            } else {
                Log.e("pasteSelectedFile","curPathAftCopy=null error.");
                IsBusy = false;
                return FileOpReturn.ERR;
            }

            if ((copy_cancel)||!(curPathBefCopy.equals(curPathAftCopy))) {
                if (copying_file.exists()) {
                    try {
                        if (copying_file.isDirectory()) {
                            FileUtils.deleteDirectory(copying_file);
                        } else {
                            copying_file.delete();
                        }
                    }
                    catch (Exception e) {
                        Log.e("Exception when delete",e.toString());
                    }
                }
                if (cur_page.equals("list")) {
                    FileBrower.mProgressHandler.sendMessage(Message.obtain(FileBrower.mProgressHandler, 9));
                } else if (cur_page.equals("thumbnail1")) {
                    ThumbnailView1.mProgressHandler.sendMessage(Message.obtain(ThumbnailView1.mProgressHandler, 9));
                }

                IsBusy = false;
                return FileOpReturn.ERR;
            }

            Bundle data = new Bundle();
            Message msg;
            if (cur_page.equals("list")) {
                msg = Message.obtain(FileBrower.mProgressHandler, 4);
                data.putStringArrayList("file_name_list", fileList);
                msg.setData(data);
                FileBrower.mProgressHandler.sendMessage(msg);
                IsBusy = false;
                return FileOpReturn.SUCCESS;
            } else if (cur_page.equals("thumbnail1")) {
                msg = Message.obtain(ThumbnailView1.mProgressHandler, 4);
                data.putStringArrayList("file_name_list", fileList);
                msg.setData(data);
                ThumbnailView1.mProgressHandler.sendMessage(msg);
                IsBusy = false;
                return FileOpReturn.SUCCESS;
           }
       } else {
            if (cur_page.equals("list")) {
                FileBrower.mProgressHandler.sendMessage(Message.obtain(
                    FileBrower.mProgressHandler, 5));
                IsBusy = false;
                return FileOpReturn.ERR;
            } else if (cur_page.equals("thumbnail1")) {
                ThumbnailView1.mProgressHandler.sendMessage(Message.obtain(
                    ThumbnailView1.mProgressHandler, 5));
                IsBusy = false;
                return FileOpReturn.ERR;
            }
        }
        IsBusy = false;
        return FileOpReturn.ERR;
    }
    public static FileOpReturn deleteSelectedFile(String cur_page) {
        List<String> fileList = new ArrayList<String>();
        boolean IsDelSuccess = true;
        IsBusy = true;
        try {
            if (cur_page.equals("list")) {
                FileBrower.myCursor = FileBrower.db.getFileMark();
                if (FileBrower.myCursor.getCount() > 0) {
                    for (int i=0; i<FileBrower.myCursor.getCount(); i++) {
                        FileBrower.myCursor.moveToPosition(i);
                        fileList.add(FileBrower.myCursor.getColFilePath());
                    }
                }
            } else if (cur_page.equals("thumbnail1")) {
                ThumbnailView1.myCursor = ThumbnailView1.db.getFileMark();
                if (ThumbnailView1.myCursor.getCount() > 0) {
                    for (int i=0; i<ThumbnailView1.myCursor.getCount(); i++) {
                        ThumbnailView1.myCursor.moveToPosition(i);
                        fileList.add(ThumbnailView1.myCursor.getColFilePath());
                    }
                }
            }
        } finally {
            if (cur_page.equals("list")) {
                FileBrower.myCursor.close();
            } else if (cur_page.equals("thumbnail1")) {
                ThumbnailView1.myCursor.close();
            }
        }

        if (!fileList.isEmpty()) {
            for (String name : fileList) {
                File file = new File(name);
                if (file.exists()) {
                    //Log.i(FileBrower.TAG, "delete file: " + name);
                    try {
                        if (file.canWrite()) {
                            if (file.isDirectory()) {
                                FileUtils.deleteDirectory(file);
                            } else {
                                IsDelSuccess = file.delete();
                            }
                        } else {
                            if (name.startsWith(FileListManager.STORAGE)) {
                                if (!(Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED_READ_ONLY))) {
                                    if (file.isDirectory()) {
                                        FileUtils.deleteDirectory(file);
                                    } else {
                                        IsDelSuccess = file.delete();
                                    }
                                }
                            }
                        }
                    } catch (Exception e) {
                        IsDelSuccess = false;
                        Log.e("Exception when delete file", e.toString());
                    }
                }
            }
            IsBusy = false;
            if (IsDelSuccess)
                return FileOpReturn.SUCCESS;
            else
                return FileOpReturn.ERR_DEL_FAIL;
        } else {
            IsBusy = false;
            return FileOpReturn.ERR;
        }
    }

    public static long checkFreeSpace(String path) {
        long nSDFreeSize = 0;
        if (path != null) {
            StatFs statfs = new StatFs(path);
            long nBlocSize = statfs.getBlockSize();
            long nAvailaBlock = statfs.getAvailableBlocks();
            nSDFreeSize = nAvailaBlock * nBlocSize;
        }
        return nSDFreeSize;
    }

    public static boolean checkCanWrite(String path) {
        if (path != null) {
            File file = new File(path);
            if (file.exists() && file.canWrite())
                return true;
        }
        return false;
    }

    /*
    * get mark file name for function paste
    */
    public static String getMarkFileName(String cur_page) {
        String path="\0";
        String name="\0";
        int index = -1;

        if (cur_page.equals("list")) {
            FileBrower.myCursor = FileBrower.db.getFileMark();
            if (FileBrower.myCursor.getCount() > 0) {
                for (int i=0; i<FileBrower.myCursor.getCount(); i++) {
                    FileBrower.myCursor.moveToPosition(i);
                    path=FileBrower.myCursor.getColFilePath();
                    index=path.lastIndexOf("/");
                    if (index >= 0) {
                        name+=path.substring(index+1);
                    }
                    name+="\n";
                }
            }
        } else if (cur_page.equals("thumbnail1")) {
            ThumbnailView1.myCursor = ThumbnailView1.db.getFileMark();
            if (ThumbnailView1.myCursor.getCount() > 0) {
                for (int i=0; i<ThumbnailView1.myCursor.getCount(); i++) {
                    ThumbnailView1.myCursor.moveToPosition(i);
                    path=ThumbnailView1.myCursor.getColFilePath();
                    index=path.lastIndexOf("/");
                    if (index >= 0) {
                        name+=path.substring(index+1);
                    }
                    name+="\n";
                }
            }
        }
        return name;
    }

    public static ArrayList<Uri> getMarkFilePathUri(String cur_page) {
        ArrayList<Uri> uris = new ArrayList<Uri>();
        String path = "\0";

        if (cur_page.equals("list")) {
            FileBrower.myCursor = FileBrower.db.getFileMark();
            if (FileBrower.myCursor.getCount() > 0) {
                for (int i=0; i<FileBrower.myCursor.getCount(); i++) {
                    FileBrower.myCursor.moveToPosition(i);
                    path = FileBrower.myCursor.getColFilePath();
                    File f = new File(path);
                    if (!f.isDirectory()) {
                        uris.add(Uri.fromFile(f));
                    }
                }
            }
        } else if (cur_page.equals("thumbnail1")) {
            ThumbnailView1.myCursor = ThumbnailView1.db.getFileMark();
            if (ThumbnailView1.myCursor.getCount() > 0) {
                for (int i=0; i<ThumbnailView1.myCursor.getCount(); i++) {
                    ThumbnailView1.myCursor.moveToPosition(i);
                    path = ThumbnailView1.myCursor.getColFilePath();
                    File f = new File(path);
                    if (!f.isDirectory()) {
                        uris.add(Uri.fromFile(f));
                    }
                }
            }
        }
        return uris;
    }

    /*
    * get mark file name for function rename
    */
    public static String getMarkFilePath(String cur_page) {
        String path="\0";

        if (cur_page.equals("list")) {
            FileBrower.myCursor = FileBrower.db.getFileMark();
            if (FileBrower.myCursor.getCount() > 0) {
                for (int i=0; i<FileBrower.myCursor.getCount(); i++) {
                    FileBrower.myCursor.moveToPosition(i);
                    path+=FileBrower.myCursor.getColFilePath();
                    path+="\n";
                }
            }
        } else if (cur_page.equals("thumbnail1")) {
            ThumbnailView1.myCursor = ThumbnailView1.db.getFileMark();
            if (ThumbnailView1.myCursor.getCount() > 0) {
                for (int i=0; i<ThumbnailView1.myCursor.getCount(); i++) {
                    ThumbnailView1.myCursor.moveToPosition(i);
                    path+=ThumbnailView1.myCursor.getColFilePath();
                    path+="\n";
                }
            }
        }
        return path;
    }

    /*
    * get single mark file name for function rename
    */
    public static String getSingleMarkFilePath(String cur_page) {
        String path = null;

        if (cur_page.equals("list")) {
            FileBrower.myCursor = FileBrower.db.getFileMark();
            if (FileBrower.myCursor.getCount() > 0) {
                if (FileBrower.myCursor.getCount() != 1) {
                    Log.e(FileBrower.TAG,"current FileBrower Cursor outof range, count:"+FileBrower.myCursor.getCount());
                } else {
                    FileBrower.myCursor.moveToFirst();
                    path=FileBrower.myCursor.getColFilePath();
                }
            }
        } else if (cur_page.equals("thumbnail1")) {
            ThumbnailView1.myCursor = ThumbnailView1.db.getFileMark();
            if (ThumbnailView1.myCursor.getCount() > 0) {
                if (ThumbnailView1.myCursor.getCount() != 1) {
                    Log.e(FileBrower.TAG,"current ThumbnailView1 Cursor outof range, count:"+FileBrower.myCursor.getCount());
                } else {
                    ThumbnailView1.myCursor.moveToFirst();
                    path=ThumbnailView1.myCursor.getColFilePath();
                }
            }
        }
        return path;
    }

    public static void updatePathShow(Activity activity, FileListManager fileListManager, String path, boolean isThumbnailView) {
        TextView tv;
        boolean pathSetted = false;
        if (isThumbnailView) {
            tv = (TextView) activity.findViewById(R.id.thumb_path);
        }
        else {
            tv = (TextView) activity.findViewById(R.id.path);
        }
        if (path.equals(FileListManager.STORAGE))
            tv.setText(activity.getText(R.string.rootDevice));
        else {
            int idx = -1;
            int len;
            String pathDest;
            String pathLast;
            String pathTmp = null;
            List<Map<String, Object>> tmpList = fileListManager.getDevices();
            int tCount = tmpList.size();
            for (int i = 0; i < tCount; i++) {
                Map<String, Object> tmap = tmpList.get(i);
                Iterator iterator = tmap.keySet().iterator();
                while (iterator.hasNext()) {
                    String string = (String) iterator.next();
                    if (string.equals(KEY_PATH)) {
                        pathTmp = (String)tmap.get(string);
                    }
                }
                idx = path.indexOf(pathTmp);
                if (idx != -1) {
                    len = pathTmp.length();
                    pathLast = path.substring(idx + len);
                    pathDest = pathTmp + pathLast;
                    tv.setText(pathDest);
                    pathSetted = true;
                    break;
                }
            }
            if (!pathSetted) {
                tv.setText(path);
            }
        }
    }
}
