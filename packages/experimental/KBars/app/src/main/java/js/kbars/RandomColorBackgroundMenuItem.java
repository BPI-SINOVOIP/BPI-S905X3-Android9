package js.kbars;

import android.graphics.Color;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MenuItem.OnMenuItemClickListener;
import android.view.View;
import java.util.Random;

public class RandomColorBackgroundMenuItem implements OnMenuItemClickListener {
    private static final Random RANDOM = new Random();
    private final View mTarget;

    public RandomColorBackgroundMenuItem(Menu menu, View target) {
        this.mTarget = target;
        MenuItem mi = menu.add("Random color background");
        mi.setShowAsActionFlags(0);
        mi.setOnMenuItemClickListener(this);
    }

    private void setColorBackground() {
        this.mTarget.setBackgroundColor(Color.rgb(RANDOM.nextInt(255), RANDOM.nextInt(255), RANDOM.nextInt(255)));
    }

    public boolean onMenuItemClick(MenuItem item) {
        setColorBackground();
        return true;
    }
}
