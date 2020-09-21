package com.google.android.libraries.backup;

import android.app.backup.BackupAgentHelper;
import android.app.backup.BackupDataInput;
import android.app.backup.BackupDataOutput;
import android.app.backup.SharedPreferencesBackupHelper;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.ParcelFileDescriptor;
import android.support.annotation.VisibleForTesting;
import android.util.Log;
import java.io.File;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

/**
 * A {@link BackupAgentHelper} that contains the following improvements:
 *
 * <p>1) All backed-up shared preference files will automatically be restored; the app does not need
 * to know the list of files in advance at restore time. This is important for apps that generate
 * files dynamically, and it's also important for all apps that use restoreAnyVersion because
 * additional files could have been added.
 *
 * <p>2) Only the requested keys will be backed up from each shared preference file. All keys that
 * were backed up will be restored.
 *
 * <p>These benefits apply only to shared preference files. Other file helpers can be added in the
 * normal way for a {@link BackupAgentHelper}.
 *
 * <p>This class works by creating a separate shared preference file named
 * {@link #RESERVED_SHARED_PREFERENCES} that it backs up and restores. Before backing up, this file
 * is populated based on the requested shared preference files and keys. After restoring, the data
 * is copied back into the original files.
 */
public abstract class PersistentBackupAgentHelper extends BackupAgentHelper {

  /**
   * The name of the shared preferences file reserved for use by the
   * {@link PersistentBackupAgentHelper}. Files with this name cannot be backed up by this helper.
   */
  protected static final String RESERVED_SHARED_PREFERENCES = "persistent_backup_agent_helper";

  private static final String TAG = "PersistentBackupAgentHe"; // The max tag length is 23.
  private static final String BACKUP_KEY = RESERVED_SHARED_PREFERENCES + "_prefs";
  private static final String BACKUP_DELIMITER = "/";

  @Override
  public void onCreate() {
    addHelper(BACKUP_KEY, new SharedPreferencesBackupHelper(this, RESERVED_SHARED_PREFERENCES));
  }

  @Override
  public void onBackup(ParcelFileDescriptor oldState, BackupDataOutput data,
      ParcelFileDescriptor newState) throws IOException {
    writeFromPreferenceFilesToBackupFile();
    super.onBackup(oldState, data, newState);
    clearBackupFile();
  }

  @VisibleForTesting
  void writeFromPreferenceFilesToBackupFile() {
    Map<String, BackupKeyPredicate> fileBackupKeyPredicates = getBackupSpecification();
    Editor backupEditor = getSharedPreferences(RESERVED_SHARED_PREFERENCES, MODE_PRIVATE).edit();
    backupEditor.clear();
    for (Map.Entry<String, BackupKeyPredicate> entry : fileBackupKeyPredicates.entrySet()) {
      writeToBackupFile(entry.getKey(), backupEditor, entry.getValue());
    }
    backupEditor.apply();
  }

  /**
   * Returns the predicate that decides which keys should be backed up for each shared preference
   * file name.
   *
   * <p>There must be no files with the same name as {@link #RESERVED_SHARED_PREFERENCES}. This
   * method assumes that all shared preference file names are valid: they must not contain path
   * separators ("/").
   *
   * <p>This method will only be called at backup time. At restore time, everything that was backed
   * up is restored.
   *
   * @see #isSupportedSharedPreferencesName
   * @see BackupKeyPredicates
   */
  protected abstract Map<String, BackupKeyPredicate> getBackupSpecification();

  /**
   * Adds data from the given file name for keys that pass the given predicate.
   * {@link Editor#apply()} is not called.
   */
  private void writeToBackupFile(
      String srcFileName, Editor editor, BackupKeyPredicate backupKeyPredicate) {
    if (!isSupportedSharedPreferencesName(srcFileName)) {
      throw new IllegalArgumentException(
          "Unsupported shared preferences file name \"" + srcFileName + "\"");
    }
    SharedPreferences srcSharedPreferences = getSharedPreferences(srcFileName, MODE_PRIVATE);
    Map<String, ?> srcMap = srcSharedPreferences.getAll();
    for (Map.Entry<String, ?> entry : srcMap.entrySet()) {
      String key = entry.getKey();
      Object value = entry.getValue();
      if (backupKeyPredicate.shouldBeBackedUp(key)) {
        putSharedPreference(editor, buildBackupKey(srcFileName, key), value);
      }
    }
  }

  private static String buildBackupKey(String fileName, String key) {
    return fileName + BACKUP_DELIMITER + key;
  }

  /**
   * Puts the given value into the given editor for the given key. {@link Editor#apply()} is not
   * called.
   */
  @SuppressWarnings("unchecked") // There are no unchecked casts - the Set<String> cast IS checked.
  public static void putSharedPreference(Editor editor, String key, Object value) {
    if (value instanceof Boolean) {
      editor.putBoolean(key, (Boolean) value);
    } else if (value instanceof Float) {
      editor.putFloat(key, (Float) value);
    } else if (value instanceof Integer) {
      editor.putInt(key, (Integer) value);
    } else if (value instanceof Long) {
      editor.putLong(key, (Long) value);
    } else if (value instanceof String) {
      editor.putString(key, (String) value);
    } else if (value instanceof Set) {
      for (Object object : (Set) value) {
        if (!(object instanceof String)) {
          // If a new type of shared preference set is added in the future, it can't be correctly
          // restored on this version.
          Log.w(TAG, "Skipping restore of key " + key + " because its value is a set containing"
              + " an object of type " + (value == null ? null : value.getClass()) + ".");
          return;
        }
      }
      editor.putStringSet(key, (Set<String>) value);
    } else {
      // If a new type of shared preference is added in the future, it can't be correctly restored
      // on this version.
      Log.w(TAG, "Skipping restore of key " + key + " because its value is the unrecognized type "
          + (value == null ? null : value.getClass()) + ".");
      return;
    }
  }

  private void clearBackupFile() {
    // We don't currently delete the file because of a lack of a supported way to do it and because
    // of the concerns of synchronously doing so.
    getSharedPreferences(RESERVED_SHARED_PREFERENCES, MODE_PRIVATE).edit().clear().apply();
  }

  @Override
  public void onRestore(BackupDataInput data, int appVersionCode, ParcelFileDescriptor stateFile)
      throws IOException {
    super.onRestore(data, appVersionCode, stateFile);
    writeFromBackupFileToPreferenceFiles(appVersionCode);
    clearBackupFile();
  }

  @VisibleForTesting
  void writeFromBackupFileToPreferenceFiles(int appVersionCode) {
    SharedPreferences backupSharedPreferences =
        getSharedPreferences(RESERVED_SHARED_PREFERENCES, MODE_PRIVATE);
    Map<String, Editor> editors = new HashMap<>();
    for (Map.Entry<String, ?> entry : backupSharedPreferences.getAll().entrySet()) {
      // We restore all files and keys, including those that this version doesn't know about or
      // wouldn't have backed up. This ensures forward-compatibility.
      String backupKey = entry.getKey();
      Object value = entry.getValue();
      int backupDelimiterIndex = backupKey.indexOf(BACKUP_DELIMITER);
      if (backupDelimiterIndex < 0 || backupDelimiterIndex >= backupKey.length() - 1) {
        Log.w(TAG, "Format of key \"" + backupKey + "\" not understood, so skipping its restore.");
        continue;
      }
      String fileName = backupKey.substring(0, backupDelimiterIndex);
      String preferenceKey = backupKey.substring(backupDelimiterIndex + 1);
      Editor editor = editors.get(fileName);
      if (editor == null) {
        if (!isSupportedSharedPreferencesName(fileName)) {
          Log.w(TAG, "Skipping unsupported shared preferences file name \"" + fileName + "\"");
          continue;
        }
        // #apply is called once for each editor later.
        editor = getSharedPreferences(fileName, MODE_PRIVATE).edit();
        editors.put(fileName, editor);
      }
      putSharedPreference(editor, preferenceKey, value);
    }
    for (Editor editor : editors.values()) {
      editor.apply();
    }
    onPreferencesRestored(editors.keySet(), appVersionCode);
  }

  /**
   * This method is called when the preferences have been restored. It can be overridden to apply
   * processing to the restored preferences. However, this is not recommended to be used in
   * conjunction with restoreAnyVersion unless the following problems are considered:
   *
   * <p>1) Once the processing is live, it could be applied to any data that ever gets backed up by
   * the app, not just the types of data that were available when the processing was originally
   * added.
   *
   * <p>2) Older versions of the app (that use restoreAnyVersion) will restore data without applying
   * the processing. For first-party apps pre-installed on the device, this could be the case for
   * every new user.
   *
   * @param names The list of files restored.
   * @param appVersionCode The app version code from {@link #onRestore}.
   */
  @SuppressWarnings({"unused"})
  protected void onPreferencesRestored(Set<String> names, int appVersionCode) {}

  /**
   * Returns whether the provided shared preferences file name is supported by this class.
   *
   * <p>The following file names are NOT supported:
   * <ul>
   *   <li>{@link #RESERVED_SHARED_PREFERENCES}
   *   <li>file names containing path separators ("/")
   * </ul>
   */
  public static boolean isSupportedSharedPreferencesName(String fileName) {
    return !fileName.contains(File.separator)
        && !fileName.contains(BACKUP_DELIMITER) // Same as File.separator. Better safe than sorry.
        && !RESERVED_SHARED_PREFERENCES.equals(fileName);
  }
}
