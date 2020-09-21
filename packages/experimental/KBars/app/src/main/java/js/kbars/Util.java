package js.kbars;

import android.content.Context;
import android.content.pm.PackageManager.NameNotFoundException;
import android.util.DisplayMetrics;
import android.view.WindowManager;
import java.lang.reflect.Field;

public class Util {
    public static String logTag(Class<?> c) {
        return "kbars." + c.getSimpleName();
    }

    public static Object getField(Object obj, String fieldName) {
        Class<?> c = obj.getClass();
        try {
            if (obj instanceof String) {
                c = c.getClassLoader().loadClass((String) obj);
                obj = null;
            }
            Field f = c.getDeclaredField(fieldName);
            f.setAccessible(true);
            return f.get(obj);
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    public static int getDensityDpi(Context context) {
        DisplayMetrics metrics = new DisplayMetrics();
        ((WindowManager) context.getSystemService("window")).getDefaultDisplay().getMetrics(metrics);
        return metrics.densityDpi;
    }

    public static String getVersionName(Context context) {
        try {
            return context.getPackageManager().getPackageInfo(context.getPackageName(), 0).versionName;
        } catch (NameNotFoundException e) {
            throw new RuntimeException(e);
        }
    }
}
