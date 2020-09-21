package js.kbars;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MenuItem.OnMenuItemClickListener;
import android.view.View;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

public final class CameraBackgroundMenuItem implements OnMenuItemClickListener {
    public static final int REQUEST_CODE = 123;
    private final Activity mActivity;
    private final View mTarget;

    public CameraBackgroundMenuItem(Menu menu, Activity activity, View target) {
        this.mActivity = activity;
        this.mTarget = target;
        menu.add("Camera image background...").setOnMenuItemClickListener(this);
        loadBitmap();
    }

    public boolean onMenuItemClick(MenuItem item) {
        chooseBackground();
        return true;
    }

    public void onActivityResult(int resultCode, Intent data) {
        if (resultCode == -1) {
            Bitmap bitmap = (Bitmap) data.getParcelableExtra("data");
            if (bitmap != null) {
                setTargetBackground(bitmap);
                saveBitmap(bitmap);
            }
        }
    }

    private void chooseBackground() {
        this.mActivity.startActivityForResult(new Intent("android.media.action.IMAGE_CAPTURE"), REQUEST_CODE);
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
            FileOutputStream out = null;
            try {
                FileOutputStream out2 = new FileOutputStream(f);
                bitmap.compress(CompressFormat.PNG, 100, out2);
                out2.close();
            } catch (FileNotFoundException e1) {
                e1.printStackTrace();
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
