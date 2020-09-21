package com.google.android.libraries.backup.shadow;

import static android.content.Context.MODE_PRIVATE;

import android.app.backup.SharedPreferencesBackupHelper;
import android.content.Context;
import android.content.SharedPreferences.Editor;
import android.util.Log;
import com.google.android.libraries.backup.PersistentBackupAgentHelper;
import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Preconditions;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import java.lang.reflect.Field;
import java.util.Map;
import java.util.Set;

/**
 * Representation of {@link SharedPreferencesBackupHelper} configuration used for testing. This
 * class simulates backing up and restoring shared preferences by storing them in memory.
 *
 * <p>{@see BackupAgentHelperShadow}
 */
public class SharedPreferencesBackupHelperSimulator extends BackupHelperSimulator {
  private static final String TAG = "SharedPreferencesBackup";

  /** Shared preferences file names which should be backed up/restored. */
  private final Set<String> prefGroups;

  private SharedPreferencesBackupHelperSimulator(String keyPrefix, Set<String> prefGroups) {
    super(keyPrefix);
    this.prefGroups = Preconditions.checkNotNull(prefGroups);
  }

  public static SharedPreferencesBackupHelperSimulator fromPreferenceGroups(
      String keyPrefix, Set<String> prefGroups) {
    return new SharedPreferencesBackupHelperSimulator(keyPrefix, prefGroups);
  }

  public static SharedPreferencesBackupHelperSimulator fromHelper(
      String keyPrefix, SharedPreferencesBackupHelper helper) {
    return new SharedPreferencesBackupHelperSimulator(
        keyPrefix, extractPreferenceGroupsFromHelper(helper));
  }

  @VisibleForTesting
  static Set<String> extractPreferenceGroupsFromHelper(SharedPreferencesBackupHelper helper) {
    try {
      Field prefGroupsField = SharedPreferencesBackupHelper.class.getDeclaredField("mPrefGroups");
      prefGroupsField.setAccessible(true);
      return ImmutableSet.copyOf((String[]) prefGroupsField.get(helper));
    } catch (ReflectiveOperationException e) {
      throw new IllegalStateException(
          "Failed to construct SharedPreferencesBackupHelperSimulator", e);
    }
  }

  /** Collection of backed up shared preferences. */
  public static class SharedPreferencesBackupData {
    /** Map from shared preferences file names to key-value preference maps. */
    private final Map<String, Map<String, ?>> preferences;

    public SharedPreferencesBackupData(Map<String, Map<String, ?>> data) {
      this.preferences = Preconditions.checkNotNull(data);
    }

    @Override
    public boolean equals(Object obj) {
      return obj instanceof SharedPreferencesBackupData
          && preferences.equals(((SharedPreferencesBackupData) obj).preferences);
    }

    @Override
    public int hashCode() {
      return preferences.hashCode();
    }

    public Map<String, Map<String, ?>> getPreferences() {
      return preferences;
    }
  }

  @Override
  public Object backup(Context context) {
    ImmutableMap.Builder<String, Map<String, ?>> dataToBackupBuilder = ImmutableMap.builder();
    for (String prefGroup : prefGroups) {
      Map<String, ?> prefs = context.getSharedPreferences(prefGroup, MODE_PRIVATE).getAll();
      if (prefs.isEmpty()) {
        Log.w(TAG, "Shared prefs \"" + prefGroup + "\" are empty. The helper \"" + keyPrefix
            + "\" assumes this is due to a missing (rather than empty) shared preferences file.");
        continue;
      }
      ImmutableMap.Builder<String, Object> prefsData = ImmutableMap.builder();
      for (Map.Entry<String, ?> prefEntry : prefs.entrySet()) {
        String key = prefEntry.getKey();
        Object value = prefEntry.getValue();
        if (value instanceof Set) {
          value = ImmutableSet.copyOf((Set<?>) value);
        }
        prefsData.put(key, value);
      }
      dataToBackupBuilder.put(prefGroup, prefsData.build());
    }
    return new SharedPreferencesBackupData(dataToBackupBuilder.build());
  }

  @Override
  public void restore(Context context, Object data) {
    if (!(data instanceof SharedPreferencesBackupData)) {
      throw new IllegalArgumentException("Invalid type of files to restore in helper \""
          + keyPrefix + "\": " + data.getClass());
    }

    Map<String, Map<String, ?>> prefsToRestore =
        ((SharedPreferencesBackupData) data).getPreferences();

    // Display a warning when missing/empty preferences are restored onto non-empty preferences.
    for (String prefGroup : prefGroups) {
      if (context.getSharedPreferences(prefGroup, MODE_PRIVATE).getAll().isEmpty()) {
        continue;
      }
      Map<String, ?> prefsData = prefsToRestore.get(prefGroup);
      if (prefsData == null) {
        Log.w(TAG, "Non-empty shared prefs \"" + prefGroup + "\" will NOT be cleared by helper \""
            + keyPrefix + "\" because the corresponding file is missing in the restored data.");
      } else if (prefsData.isEmpty()) {
        Log.w(TAG, "Non-empty shared prefs \"" + prefGroup + "\" will be cleared by helper \""
            + keyPrefix + "\" because the corresponding file is empty in the restored data.");
      }
    }

    for (Map.Entry<String, Map<String, ?>> restoreEntry : prefsToRestore.entrySet()) {
      String prefGroup = restoreEntry.getKey();
      if (!prefGroups.contains(prefGroup)) {
        Log.w(TAG, "Shared prefs \"" + prefGroup + "\" ignored by helper \"" + keyPrefix + "\".");
        continue;
      }
      Map<String, ?> prefsData = restoreEntry.getValue();
      Editor editor = context.getSharedPreferences(prefGroup, MODE_PRIVATE).edit().clear();
      for (Map.Entry<String, ?> prefEntry : prefsData.entrySet()) {
        PersistentBackupAgentHelper.putSharedPreference(
            editor, prefEntry.getKey(), prefEntry.getValue());
      }
      editor.apply();
    }
  }
}
