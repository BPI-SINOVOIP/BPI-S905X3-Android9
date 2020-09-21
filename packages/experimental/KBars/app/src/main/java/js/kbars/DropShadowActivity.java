package js.kbars;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.BitmapFactory;
import android.graphics.BlurMaskFilter;
import android.graphics.BlurMaskFilter.Blur;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.os.Bundle;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;

public class DropShadowActivity extends Activity {
    private final Context mContext = this;
    private ImageView mImageView;

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        this.mImageView = new ImageView(this.mContext);
        this.mImageView.setBackgroundColor(-1);
        this.mImageView.setScaleType(ScaleType.CENTER);
        this.mImageView.setScaleX(1.0f);
        this.mImageView.setScaleY(1.0f);
        setContentView(this.mImageView);
        setImage();
    }

    private void setImage() {
        BlurMaskFilter blurFilter = new BlurMaskFilter(1.0f, Blur.SOLID);
        Paint shadowPaint = new Paint();
        int[] offsetXY = new int[2];
        Bitmap originalBitmap = BitmapFactory.decodeResource(getResources(), 17301624);
        Bitmap shadowImage32 = originalBitmap.extractAlpha(shadowPaint, offsetXY).copy(Config.ARGB_8888, true);
        new Canvas(shadowImage32).drawBitmap(originalBitmap, (float) ((-offsetXY[0]) - 10), (float) ((-offsetXY[1]) - 10), null);
        this.mImageView.setImageBitmap(shadowImage32);
    }
}
