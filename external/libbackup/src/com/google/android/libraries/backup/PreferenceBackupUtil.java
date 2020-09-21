package com.google.android.libraries.backup;

import android.content.Context;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.media.Ringtone;
import android.media.RingtoneManager;
import android.net.Uri;
import android.support.annotation.Nullable;
import android.support.annotation.VisibleForTesting;

/**
 * Utility class of static methods to help process shared preferences for backup and restore.
 */
public class PreferenceBackupUtil {

  @VisibleForTesting
  @Nullable
  static String getRingtoneTitleFromUri(Context context, @Nullable String uri) {
    if (uri == null) {
      return null;
    }

    Ringtone sound = RingtoneManager.getRingtone(context, Uri.parse(uri));
    if (sound == null) {
      return null;
    }
    return sound.getTitle(context);
  }

  /**
   * Get ringtone uri from a preference key in a shared preferences file, retrieve the associated
   * ringtone's title and, if possible, save the title to the target preference key.
   *
   * @param srcRingtoneUriPrefKey preference key of the ringtone uri.
   * @param dstRingtoneTitlePrefKey preference key where the ringtone title should be put.
   * @return whether the ringtoneTitleKey was set.
   */
  public static boolean encodeRingtonePreference(Context context, String prefsName,
      String srcRingtoneUriPrefKey, String dstRingtoneTitlePrefKey) {
    SharedPreferences preferences = context.getSharedPreferences(prefsName, Context.MODE_PRIVATE);

    String uri = preferences.getString(srcRingtoneUriPrefKey, null);
    String title = getRingtoneTitleFromUri(context, uri);
    if (title == null) {
      return false;
    }

    preferences.edit().putString(dstRingtoneTitlePrefKey, title).apply();
    return true;
  }

  @VisibleForTesting
  @Nullable
  static String getRingtoneUriFromTitle(Context context, @Nullable String title, int ringtoneType) {
    // Check whether the ringtoneType is a valid combination of the 3 ringtone types.
    if ((ringtoneType == 0)
        || ((RingtoneManager.TYPE_ALL & ringtoneType) != ringtoneType)) {
      throw new IllegalStateException();
    }
    if (title == null) {
      return null;
    }

    RingtoneManager manager = new RingtoneManager(context);
    manager.setType(ringtoneType);
    Cursor cur = manager.getCursor();
    for (int i = 0; i < cur.getCount(); i++) {
      Ringtone ringtone = manager.getRingtone(i);
      if (ringtone.getTitle(context).equals(title)) {
        return manager.getRingtoneUri(i).toString();
      }
    }

    return null;
  }

  /**
   * Get ringtone title from a preference key of a shared preferences file, find a ringtone with the
   * same title and, if possible, save its uri to the target preference key.
   *
   * @param dstRingtoneUriPrefKey preference key where the ringtone uri should be put.
   * @param srcRingtoneTitlePrefKey preference key of the ringtone title.
   * @return whether the ringtoneUriKey was set.
   */
  public static boolean decodeRingtonePreference(Context context, String prefsName,
      String dstRingtoneUriPrefKey, String srcRingtoneTitlePrefKey, int ringtoneType) {
    SharedPreferences preferences = context.getSharedPreferences(prefsName, Context.MODE_PRIVATE);

    String title = preferences.getString(srcRingtoneTitlePrefKey, null);
    String uri = getRingtoneUriFromTitle(context, title, ringtoneType);
    if (uri == null) {
      return false;
    }

    preferences.edit().putString(dstRingtoneUriPrefKey, uri).apply();
    return true;
  }
}