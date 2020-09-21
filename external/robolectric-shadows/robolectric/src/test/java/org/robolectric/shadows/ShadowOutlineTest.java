package org.robolectric.shadows;

import static android.os.Build.VERSION_CODES.LOLLIPOP;

import android.graphics.Outline;
import android.graphics.Path;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(minSdk = LOLLIPOP)
public class ShadowOutlineTest {

    @Test
    public void setConvexPath_doesNothing() {
        final Outline outline = new Outline();
        outline.setConvexPath(new Path());
    }
}
