package com.google.android.libraries.backup.shadow;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.app.backup.BackupAgent;
import android.app.backup.BackupAgentHelper;
import android.app.backup.BackupDataInput;
import android.app.backup.BackupDataOutput;
import android.app.backup.BackupHelper;
import android.app.backup.FileBackupHelper;
import android.app.backup.SharedPreferencesBackupHelper;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.os.ParcelFileDescriptor;
import android.util.Log;
import com.google.common.collect.ImmutableMap;
import java.io.IOException;
import java.lang.reflect.Method;
import java.util.Map;
import java.util.TreeMap;
import java.util.concurrent.atomic.AtomicReference;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.annotation.RealObject;
import org.robolectric.fakes.RoboSharedPreferences;

/**
 * Shadow class for end-to-end testing of {@link BackupAgentHelper} subclasses in unit tests.
 *
 * <p>This class currently supports <b>key-value backups only</b>. In other words, it does
 * <b>not</b> support Dolly. In addition, the testing framework has the following two limitations
 * with regards to backup/restore of {@link SharedPreferences}:
 *
 * <ol>
 *   <li>Preferences are normally backed by xml files in the app's shared_prefs directory, but
 *   Robolectric replaces them with {@link RoboSharedPreferences}, which are backed by an in-memory
 *   {@link Map}. Therefore, modifying the relevant xml files will have no effect on the preferences
 *   (and vice versa).
 *   <li>For the same reason, the testing framework cannot easily determine whether the underlying
 *   xml file for given shared preferences would have been empty or missing upon backup. The latter
 *   is assumed to ensure that apps don't rely on restore to implicitly clear data (potentially
 *   PII).
 * </ol>
 */
@Implements(BackupAgentHelper.class)
public class BackupAgentHelperShadow {
  private static final String TAG = "BackupAgentHelperShadow";

  /**
   * Temporarily stores the backup data generated in {@link #onBackup} so that it could be returned
   * by {@link #simulateBackup}.
   */
  private static final AtomicReference<Map<String, Object>> backupDataMapToBackup =
      new AtomicReference<>();

  /**
   * Temporarily stores the backed up data passed to {@link #simulateRestore} so that it could be
   * used in {@link #onRestore}.
   */
  private static final AtomicReference<Map<String, Object>> backupDataMapToRestore =
      new AtomicReference<>();

  /**
   * Simulates key-value backup for the provided agent all the way from {@link
   * BackupAgentHelper#onCreate} to {@link BackupAgentHelper#onDestroy} (both inclusive).
   */
  public static Map<String, Object> simulateBackup(BackupAgentHelper agent) {
    Map<String, Object> backupDataMap;
    attachBaseContextToAgentIfNecessary(agent);
    agent.onCreate();
    try {
      agent.onBackup(null, null, null);
      backupDataMap = backupDataMapToBackup.getAndSet(null);
    } catch (IOException e) {
      backupDataMapToBackup.set(null);
      throw new IllegalStateException(e);
    }
    agent.onDestroy();
    return backupDataMap;
  }

  /**
   * Simulates key-value restore for the provided agent all the way from {@link
   * BackupAgentHelper#onCreate} to {@link BackupAgentHelper#onDestroy} (both inclusive).
   *
   * <p>Note: To make end-to-end tests more realistic, <b>different {@link BackupAgentHelper}
   * instances</b> should be used in {@link #simulateBackup} and {@link #simulateRestore}.
   */
  public static void simulateRestore(
      BackupAgentHelper agent, Map<String, Object> backupDataMap, int appVersionCode) {
    attachBaseContextToAgentIfNecessary(agent);
    agent.onCreate();
    assertTrue(backupDataMapToRestore.compareAndSet(null, backupDataMap));
    try {
      agent.onRestore(null, appVersionCode, null);
    } catch (IOException e) {
      throw new IllegalStateException(e);
    } finally {
      backupDataMapToRestore.set(null);
    }
    if (VERSION.SDK_INT >= VERSION_CODES.LOLLIPOP) {
      agent.onRestoreFinished();
    }
    agent.onDestroy();
  }

  private static void attachBaseContextToAgentIfNecessary(BackupAgentHelper agent) {
    if (agent.getBaseContext() != null) {
      return;
    }
    try {
      // {@link BackupAgent#attach} is a hidden method, so we need to call it via reflection.
      Method method = BackupAgent.class.getMethod("attach", Context.class);
      method.invoke(agent, RuntimeEnvironment.application);
    } catch (ReflectiveOperationException e) {
      throw new IllegalStateException(e);
    }
  }

  private final Map<String, BackupHelperSimulator> helperSimulators;

  public BackupAgentHelperShadow() {
    // Use a {@link TreeMap} to mirror the internal implementation of {@link BackupHelperDispatcher}
    // as closely as possible.
    helperSimulators = new TreeMap<>();
  }

  @RealObject private BackupAgentHelper realHelper;

  @Implementation
  public void addHelper(String keyPrefix, BackupHelper helper) {
    Class<? extends BackupHelper> helperClass = helper.getClass();
    final BackupHelperSimulator simulator;
    if (helperClass == SharedPreferencesBackupHelper.class) {
      simulator = SharedPreferencesBackupHelperSimulator.fromHelper(
          keyPrefix, (SharedPreferencesBackupHelper) helper);
    } else if (helperClass == FileBackupHelper.class) {
      simulator = FileBackupHelperSimulator.fromHelper(keyPrefix, (FileBackupHelper) helper);
    } else {
      throw new UnsupportedOperationException(
          "Unknown backup helper class for key prefix \"" + keyPrefix + "\": " + helperClass);
    }
    helperSimulators.put(keyPrefix, simulator);
  }

  @Implementation
  public void onBackup(
      ParcelFileDescriptor oldState, BackupDataOutput data, ParcelFileDescriptor newState)
      throws IOException {
    ImmutableMap.Builder<String, Object> backupDataMapBuilder = ImmutableMap.builder();
    for (Map.Entry<String, BackupHelperSimulator> simulatorEntry : helperSimulators.entrySet()) {
      String keyPrefix = simulatorEntry.getKey();
      BackupHelperSimulator simulator = simulatorEntry.getValue();
      backupDataMapBuilder.put(keyPrefix, simulator.backup(realHelper));
    }

    assertTrue(backupDataMapToBackup.compareAndSet(null, backupDataMapBuilder.build()));
  }

  @Implementation
  public void onRestore(BackupDataInput data, int appVersionCode, ParcelFileDescriptor newState)
      throws IOException {
    Map<String, Object> backupDataMap = backupDataMapToRestore.getAndSet(null);
    assertNotNull(backupDataMap);

    for (Map.Entry<String, BackupHelperSimulator> simulatorEntry : helperSimulators.entrySet()) {
      String keyPrefix = simulatorEntry.getKey();
      Object dataToRestore = backupDataMap.get(keyPrefix);
      if (dataToRestore == null) {
        Log.w(TAG, "No data to restore for key prefix: \"" + keyPrefix + "\".");
        continue;
      }
      BackupHelperSimulator simulator = simulatorEntry.getValue();
      simulator.restore(realHelper, dataToRestore);
    }
  }
}
