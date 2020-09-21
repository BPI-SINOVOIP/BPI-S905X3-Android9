package com.google.android.libraries.backup.shadow;

import android.app.backup.FileBackupHelper;
import android.content.Context;
import android.util.Log;
import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Preconditions;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import com.google.common.io.Files;
import java.io.File;
import java.io.IOException;
import java.lang.reflect.Field;
import java.util.Arrays;
import java.util.Map;
import java.util.Set;

/**
 * Representation of {@link FileBackupHelper} configuration used for testing. This class simulates
 * backing up and restoring files by storing their contents in memory.
 *
 * <p>{@see BackupAgentHelperShadow}
 */
public class FileBackupHelperSimulator extends BackupHelperSimulator {
  private static final String TAG = "FileBackupHelperSimulat";

  /** Filenames which should be backed up/restored. */
  private final Set<String> fileNames;

  private FileBackupHelperSimulator(String keyPrefix, Set<String> fileNames) {
    super(keyPrefix);
    this.fileNames = Preconditions.checkNotNull(fileNames);
  }

  public static FileBackupHelperSimulator fromFileNames(String keyPrefix, Set<String> fileNames) {
    return new FileBackupHelperSimulator(keyPrefix, fileNames);
  }

  public static FileBackupHelperSimulator fromHelper(String keyPrefix, FileBackupHelper helper) {
    return new FileBackupHelperSimulator(keyPrefix, extractFileNamesFromHelper(helper));
  }

  @VisibleForTesting
  static Set<String> extractFileNamesFromHelper(FileBackupHelper helper) {
    try {
      Field filesField = FileBackupHelper.class.getDeclaredField("mFiles");
      filesField.setAccessible(true);
      return ImmutableSet.copyOf((String[]) filesField.get(helper));
    } catch (ReflectiveOperationException e) {
      throw new IllegalStateException(e);
    }
  }

  /** Collection of backed up files. */
  public static class FilesBackupData {
    /** Map from file names to their backed up contents. */
    private final Map<String, FileBackupContents> files;

    public FilesBackupData(Map<String, FileBackupContents> files) {
      this.files = Preconditions.checkNotNull(files);
    }

    @Override
    public boolean equals(Object obj) {
      return obj instanceof FilesBackupData && files.equals(((FilesBackupData) obj).files);
    }

    @Override
    public int hashCode() {
      return files.hashCode();
    }

    public Map<String, FileBackupContents> getFiles() {
      return files;
    }
  }

  /** Single backed up file. */
  public static class FileBackupContents {
    private final byte[] bytes;

    public FileBackupContents(byte[] bytes) {
      this.bytes = Preconditions.checkNotNull(bytes);
    }

    @Override
    public boolean equals(Object obj) {
      return obj instanceof FileBackupContents
          && Arrays.equals(bytes, ((FileBackupContents) obj).bytes);
    }

    @Override
    public int hashCode() {
      return Arrays.hashCode(bytes);
    }

    public static FileBackupContents readFromFile(File file) {
      return new FileBackupContents(readBytesFromFile(file));
    }

    public void writeToFile(File file) {
      writeBytesToFile(file, bytes);
    }

    public static byte[] readBytesFromFile(File file) {
      try {
        return Files.toByteArray(file);
      } catch (IOException e) {
        throw new IllegalStateException(e);
      }
    }

    public static void writeBytesToFile(File file, byte[] bytes) {
      try {
        Files.write(bytes, file);
      } catch (IOException e) {
        throw new IllegalStateException(e);
      }
    }
  }

  @Override
  public Object backup(Context context) {
    File base = context.getFilesDir();
    ImmutableMap.Builder<String, FileBackupContents> dataToBackupBuilder = ImmutableMap.builder();
    for (String fileName : fileNames) {
      File file = new File(base, fileName);
      if (!file.exists()) {
        Log.w(TAG, "File \"" + fileName + "\" not found by helper \"" + keyPrefix + "\".");
        continue;
      }
      dataToBackupBuilder.put(fileName, FileBackupContents.readFromFile(file));
    }
    return new FilesBackupData(dataToBackupBuilder.build());
  }

  @Override
  public void restore(Context context, Object data) {
    if (!(data instanceof FilesBackupData)) {
      throw new IllegalArgumentException("Invalid type of files to restore in helper \""
          + keyPrefix + "\": " + data.getClass());
    }

    File base = context.getFilesDir();
    Map<String, FileBackupContents> dataToRestore = ((FilesBackupData) data).getFiles();
    for (Map.Entry<String, FileBackupContents> restoreEntry : dataToRestore.entrySet()) {
      String fileName = restoreEntry.getKey();
      if (!fileNames.contains(fileName)) {
        Log.w(TAG, "File \"" + fileName + "\" ignored by helper \"" + keyPrefix + "\".");
        continue;
      }
      FileBackupContents contents = restoreEntry.getValue();
      File file = new File(base, fileName);
      contents.writeToFile(file);
    }
  }
}
