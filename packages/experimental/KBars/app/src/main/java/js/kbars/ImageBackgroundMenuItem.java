package js.kbars;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.net.Uri;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MenuItem.OnMenuItemClickListener;
import android.view.View;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

public final class ImageBackgroundMenuItem implements OnMenuItemClickListener {
    public static final int REQUEST_CODE = 234;
    private static final String TAG = ImageBackgroundMenuItem.class.getSimpleName();
    private final Activity mActivity;
    private final View mTarget;

    public ImageBackgroundMenuItem(Menu menu, Activity activity, View target) {
        this.mActivity = activity;
        this.mTarget = target;
        menu.add("Select image background...").setOnMenuItemClickListener(this);
        loadBitmap();
    }

    public boolean onMenuItemClick(MenuItem item) {
        chooseBackground();
        return true;
    }

    public void onActivityResult(int resultCode, Intent data) {
        if (resultCode == -1) {
            Uri image = data.getData();
            try {
                Bitmap bitmap = BitmapFactory.decodeStream(this.mActivity.getContentResolver().openInputStream(image));
                if (bitmap != null) {
                    setTargetBackground(bitmap);
                    saveBitmap(bitmap);
                }
            } catch (FileNotFoundException e) {
                Log.w(TAG, "Error getting image " + image, e);
            }
        }
    }

    private void chooseBackground() {
        Intent photoPickerIntent = new Intent("android.intent.action.PICK");
        photoPickerIntent.setType("image/*");
        this.mActivity.startActivityForResult(photoPickerIntent, REQUEST_CODE);
    }

    private File bitmapFile() {
        return new File(this.mActivity.getCacheDir(), "background.png");
    }

    private void saveBitmap(Bitmap bitmap) {
        FileNotFoundException e;
        Throwable th;
        File f = bitmapFile();
        if (f.exists()) {
            f.delete();
        }
        if (bitmap != null) {
            try {
                FileOutputStream out = new FileOutputStream(f);
                bitmap.compress(CompressFormat.PNG, 100, out);
                out.close();
            } catch (IOException e1) {
                e1.printStackTrace();
            }
        }
    }

    private void loadBitmap() {
        Bitmap bitmap = loadBitmapFromCache();
        if (bitmap == null) {
            bitmap = loadBitmapFromAssets();
        }
        setTargetBackground(bitmap);
    }

    private Bitmap loadBitmapFromCache() {
        File f = bitmapFile();
        return f.exists() ? BitmapFactory.decodeFile(f.getAbsolutePath()) : null;
    }

    private Bitmap loadBitmapFromAssets() {
        try {
            return BitmapFactory.decodeStream(this.mActivity.getAssets().open("background.png"));
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    private void setTargetBackground(Bitmap bitmap) {
        if (bitmap == null) {
            this.mTarget.setBackground(null);
        } else {
            this.mTarget.setBackground(new BitmapDrawable(this.mActivity.getResources(), bitmap));
        }
    }
}
