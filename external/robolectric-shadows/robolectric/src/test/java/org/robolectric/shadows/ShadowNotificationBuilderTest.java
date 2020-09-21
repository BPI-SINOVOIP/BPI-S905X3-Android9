package org.robolectric.shadows;

import static android.os.Build.VERSION_CODES.JELLY_BEAN_MR1;
import static android.os.Build.VERSION_CODES.JELLY_BEAN_MR2;
import static android.os.Build.VERSION_CODES.M;
import static org.assertj.core.api.Assertions.assertThat;
import static org.robolectric.Shadows.shadowOf;

import android.app.Notification;
import android.app.PendingIntent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.Icon;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.R;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
public class ShadowNotificationBuilderTest {
  private final Notification.Builder builder = new Notification.Builder(RuntimeEnvironment.application);

  @Test
  public void build_setsContentTitleOnNotification() throws Exception {
    Notification notification = builder.setContentTitle("Hello").build();
    assertThat(shadowOf(notification).getContentTitle().toString()).isEqualTo("Hello");
  }

  @Test
  public void build_whenSetOngoingNotSet_leavesSetOngoingAsFalse() {
    Notification notification = builder.build();
    assertThat(shadowOf(notification).isOngoing()).isFalse();
  }

  @Test
  public void build_whenSetOngoing_setsOngoingToTrue() {
    Notification notification = builder.setOngoing(true).build();
    assertThat(shadowOf(notification).isOngoing()).isTrue();
  }

  @Test
  @Config(minSdk = JELLY_BEAN_MR1)
  public void build_whenShowWhenNotSet_setsShowWhenOnNotificationToTrue() {
    Notification notification = builder.setWhen(100).setShowWhen(true).build();

    assertThat(shadowOf(notification).isWhenShown()).isTrue();
  }

  @Test
  @Config(minSdk = JELLY_BEAN_MR1)
  public void build_setShowWhenOnNotification() {
    Notification notification = builder.setShowWhen(false).build();

    assertThat(shadowOf(notification).isWhenShown()).isFalse();
  }

  @Test
  public void build_setsContentTextOnNotification() throws Exception {
    Notification notification = builder.setContentText("Hello Text").build();

    assertThat(shadowOf(notification).getContentText().toString()).isEqualTo("Hello Text");
  }

  @Test
  public void build_setsTickerOnNotification() throws Exception {
    Notification notification = builder.setTicker("My ticker").build();

    assertThat(notification.tickerText).isEqualTo("My ticker");
  }

  @Test
  public void build_setsContentInfoOnNotification() throws Exception {
    builder.setContentInfo("11");
    Notification notification = builder.build();
    assertThat(shadowOf(notification).getContentInfo().toString()).isEqualTo("11");
  }

  @Test
  @Config(minSdk = M)
  public void build_setsIconOnNotification() throws Exception {
    Notification notification = builder.setSmallIcon(R.drawable.an_image).build();

    assertThat(notification.getSmallIcon().getResId()).isEqualTo(R.drawable.an_image);
  }

  @Test
  public void build_setsWhenOnNotification() throws Exception {
    Notification notification = builder.setWhen(11L).build();

    assertThat(notification.when).isEqualTo(11L);
  }

  @Test
  public void build_setsProgressOnNotification_true() throws Exception {
    Notification notification = builder.setProgress(36, 57, true).build();
    // If indeterminate then max and progress values are ignored.
    assertThat(shadowOf(notification).isIndeterminate()).isTrue();
  }

  @Test
  public void build_setsProgressOnNotification_false() throws Exception {
    Notification notification = builder.setProgress(50, 10, false).build();

    assertThat(shadowOf(notification).getMax()).isEqualTo(50);
    assertThat(shadowOf(notification).getProgress()).isEqualTo(10);
    assertThat(shadowOf(notification).isIndeterminate()).isFalse();
  }

  @Test
  @Config(minSdk = JELLY_BEAN_MR1)
  public void build_setsUsesChronometerOnNotification_true() throws Exception {
    Notification notification = builder.setUsesChronometer(true).setWhen(10).setShowWhen(true).build();

    assertThat(shadowOf(notification).usesChronometer()).isTrue();
  }

  @Test
  @Config(minSdk = JELLY_BEAN_MR1)
  public void build_setsUsesChronometerOnNotification_false() throws Exception {
    Notification notification = builder.setUsesChronometer(false).setWhen(10).setShowWhen(true).build();

    assertThat(shadowOf(notification).usesChronometer()).isFalse();
  }

  @Test
  public void build_handlesNullContentTitle() {
    Notification notification = builder.setContentTitle(null).build();

    assertThat(shadowOf(notification).getContentTitle()).isNullOrEmpty();
  }

  @Test
  public void build_handlesNullContentText() {
    Notification notification = builder.setContentText(null).build();

    assertThat(shadowOf(notification).getContentText()).isNullOrEmpty();
  }

  @Test
  public void build_handlesNullTicker() {
    Notification notification = builder.setTicker(null).build();

    assertThat(notification.tickerText).isNull();
  }

  @Test
  public void build_handlesNullContentInfo() {
    Notification notification = builder.setContentInfo(null).build();

    assertThat(shadowOf(notification).getContentInfo()).isNullOrEmpty();
  }

  @Test
  @Config(minSdk = JELLY_BEAN_MR2)
  public void build_addsActionToNotification() throws Exception {
    PendingIntent action = PendingIntent.getBroadcast(RuntimeEnvironment.application, 0, null, 0);
    Notification notification = builder.addAction(0, "Action", action).build();

    assertThat(notification.actions[0].actionIntent).isEqualToComparingFieldByField(action);
  }

  @Test
  public void withBigTextStyle() {
    Notification notification = builder.setStyle(new Notification.BigTextStyle(builder)
        .bigText("BigText")
        .setBigContentTitle("Title")
        .setSummaryText("Summary"))
        .build();

    assertThat(shadowOf(notification).getBigText()).isEqualTo("BigText");
    assertThat(shadowOf(notification).getBigContentTitle()).isEqualTo("Title");
    assertThat(shadowOf(notification).getBigContentText()).isEqualTo("Summary");
    assertThat(shadowOf(notification).getBigPicture()).isNull();
  }

  @Test
  @Config(minSdk = M)
  public void withBigPictureStyle() {
    Bitmap bigPicture = BitmapFactory.decodeResource(RuntimeEnvironment.application.getResources(), R.drawable.an_image);

    Icon bigLargeIcon = Icon.createWithBitmap(bigPicture);
    Notification notification = builder.setStyle(new Notification.BigPictureStyle(builder)
        .bigPicture(bigPicture)
        .bigLargeIcon(bigLargeIcon))
        .build();

    assertThat(shadowOf(notification).getBigPicture()).isEqualTo(bigPicture);
  }
}
