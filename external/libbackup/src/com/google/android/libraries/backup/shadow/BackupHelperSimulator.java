package com.google.android.libraries.backup.shadow;

import android.app.backup.BackupHelper;
import android.app.backup.FileBackupHelper;
import android.content.Context;
import com.google.common.base.Preconditions;

/**
 * Class which simulates backup & restore functionality of a {@link BackupHelper}.
 */
public abstract class BackupHelperSimulator {

  /** Prefix key of the corresponding {@link FileBackupHelper}. */
  protected final String keyPrefix;

  public BackupHelperSimulator(String keyPrefix) {
    this.keyPrefix = Preconditions.checkNotNull(keyPrefix);
  }

  /** Perform backup into an {@link Object}, which is then returned by the method. */
  public abstract Object backup(Context context);

  /**
   * Perform restore from the provided {@link Object}, which must have the same type as the one
   * returned by {@link #backup}.
   */
  public abstract void restore(Context context, Object data);
}
