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
import java.io.File;
import java.util.ArrayList;
import java.util.List;
import android.graphics.Bitmap;
import android.content.Context;
import com.droidlogic.app.FileListManager;

class  Photo {
    private  Bitmap bm;
    private  String filename;
    public  Photo(Bitmap bm,String filename) {
        this .bm = bm;
        this .filename = filename;
    }

    public  Bitmap getBm() {
        return  bm;
    }
    public  String getFilename() {
        return  filename;
    }
    public   void  setBm(Bitmap bm) {
        this .bm = bm;
    }
    public   void  setFilename(String filename) {
        this .filename = filename;
    }
}
interface  PhotoDecodeListener {
    public   void  onPhotoDecodeListener(Photo photo);
}

    class DecodePhoto {
        int decodeNo = 0;
        private static final DecodePhoto decodephoto = new DecodePhoto();
        private static List<String> myImageURL = null;
        private List<Photo> photos = new ArrayList<Photo>();
        private static Context mContext;
        private FileListManager mFileListManager = new FileListManager(mContext);
        public static DecodePhoto getInstance() {
            //setImageURL(filelist);
            return decodephoto;
        }
        public static List<String> setImageURL(List<String> filelist){
            myImageURL = filelist;
            return myImageURL;
        }
        public List<Photo> decodeImage(PhotoDecodeListener listener,List<String> flist){
            Bitmap bm = null;
            Photo photo = null;
            String filename;
            try{
                for (int i = 0; i < flist.size(); i++) {
                    File f = new File(flist.get(i));
                    filename = f.getName();
                    if (mFileListManager.isPhoto(filename)) {
                        bm = FileOp.fitSizePic(f);
                    }
                    else{
                        bm = null;
                    }
                    photo = new Photo(bm,flist.get(i));
                    listener.onPhotoDecodeListener(photo);
                    photos.add(photo);
                }
            }catch  (Exception e) {
                throw   new  RuntimeException(e);
            }
        return photos;
    }
}