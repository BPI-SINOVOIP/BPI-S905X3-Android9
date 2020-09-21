package org.robolectric.shadows;

import static android.os.Build.VERSION_CODES.JELLY_BEAN_MR2;
import static android.os.Build.VERSION_CODES.KITKAT;
import static android.os.Build.VERSION_CODES.LOLLIPOP_MR1;
import static android.os.Build.VERSION_CODES.M;
import static org.assertj.core.api.Assertions.assertThat;

import android.text.format.DateUtils;
import java.util.Calendar;
import java.util.TimeZone;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
public class ShadowDateUtilsTest {

  @Test
  @Config(minSdk = KITKAT, maxSdk = LOLLIPOP_MR1)
  public void formatDateTime_withCurrentYear_worksSinceKitKat() {
    final long millisAtStartOfYear = getMillisAtStartOfYear();

    String actual = DateUtils.formatDateTime(RuntimeEnvironment.application, millisAtStartOfYear, DateUtils.FORMAT_NUMERIC_DATE);
    assertThat(actual).isEqualTo("1/1");
  }

  @Test
  @Config(minSdk = M)
  public void formatDateTime_withCurrentYear_worksSinceM() {
    final long millisAtStartOfYear = getMillisAtStartOfYear();

    // starting with M, sometimes the year is there, sometimes it's missing, unless you specify FORMAT_SHOW_YEAR
    String actual = DateUtils.formatDateTime(RuntimeEnvironment.application, millisAtStartOfYear, DateUtils.FORMAT_SHOW_YEAR | DateUtils.FORMAT_NUMERIC_DATE);
    final int currentYear = Calendar.getInstance().get(Calendar.YEAR);
    assertThat(actual).isEqualTo("1/1/" + currentYear);
  }

  @Test
  @Config(maxSdk = JELLY_BEAN_MR2)
  public void formatDateTime_withCurrentYear_worksPreKitKat() {
    Calendar calendar = Calendar.getInstance();
    final int currentYear = calendar.get(Calendar.YEAR);
    final long millisAtStartOfYear = getMillisAtStartOfYear();

    String actual = DateUtils.formatDateTime(RuntimeEnvironment.application, millisAtStartOfYear, DateUtils.FORMAT_NUMERIC_DATE);
    assertThat(actual).isEqualTo("1/1/" + currentYear);
  }

  @Test
  public void formatDateTime_withPastYear() {
      String actual = DateUtils.formatDateTime(RuntimeEnvironment.application, 1420099200000L, DateUtils.FORMAT_NUMERIC_DATE);
      assertThat(actual).isEqualTo("1/1/2015");
  }

  @Test
  public void isToday_shouldReturnFalseForNotToday() {
    long today = java.util.Calendar.getInstance().getTimeInMillis();
    ShadowSystemClock.setCurrentTimeMillis(today);

    assertThat(DateUtils.isToday(today)).isTrue();
    assertThat(DateUtils.isToday(today + (86400 * 1000)  /* 24 hours */)).isFalse();
    assertThat(DateUtils.isToday(today + (86400 * 10000) /* 240 hours */)).isFalse();
  }

  private long getMillisAtStartOfYear() {
    Calendar calendar = Calendar.getInstance();
    final int currentYear = calendar.get(Calendar.YEAR);
    calendar.setTimeZone(TimeZone.getTimeZone("GMT-8:00"));
    calendar.set(currentYear, Calendar.JANUARY, 1, 0, 0, 0);

    return calendar.getTimeInMillis();
  }
}
