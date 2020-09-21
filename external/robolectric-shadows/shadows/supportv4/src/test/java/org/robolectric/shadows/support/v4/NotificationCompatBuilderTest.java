package org.robolectric.shadows.support.v4;

import static org.assertj.core.api.Assertions.assertThat;

import android.app.Notification;
import android.support.v4.app.NotificationCompat;
import com.android.internal.R;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.util.TestRunnerWithManifest;

@RunWith(TestRunnerWithManifest.class)
@Config(sdk = 19)
public class NotificationCompatBuilderTest {
  @Test
  public void addAction__shouldAddActionToNotification() {
    NotificationCompat.Action action = new NotificationCompat.Action.Builder(R.drawable.ic_corp_icon, "a title", null).build();
    Notification notification =
        new NotificationCompat.Builder(RuntimeEnvironment.application)
            .addAction(action)
            .build();
    assertThat(notification.actions).hasSize(1);
  }
}
