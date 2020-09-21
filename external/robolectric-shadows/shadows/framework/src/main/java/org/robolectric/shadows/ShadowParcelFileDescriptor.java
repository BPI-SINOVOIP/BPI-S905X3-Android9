package org.robolectric.shadows;

import static org.robolectric.shadow.api.Shadow.invokeConstructor;
import static org.robolectric.util.ReflectionHelpers.ClassParameter.from;

import android.os.ParcelFileDescriptor;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.lang.reflect.Constructor;
import org.robolectric.Shadows;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.annotation.RealObject;

@Implements(ParcelFileDescriptor.class)
public class ShadowParcelFileDescriptor {
  private RandomAccessFile file;

  private @RealObject ParcelFileDescriptor realObject;

  @Implementation
  public void __constructor__(ParcelFileDescriptor wrapped) {
    invokeConstructor(ParcelFileDescriptor.class, realObject,
        from(ParcelFileDescriptor.class, wrapped));
    if (wrapped != null) {
      this.file = Shadows.shadowOf(wrapped).file;
    }
  }

  @Implementation
  public static ParcelFileDescriptor open(File file, int mode) throws FileNotFoundException {
    ParcelFileDescriptor pfd;
    try {
      Constructor<ParcelFileDescriptor> constructor = ParcelFileDescriptor.class.getDeclaredConstructor(FileDescriptor.class);
      pfd = constructor.newInstance(new FileDescriptor());
    } catch (Exception e) {
      throw new RuntimeException(e);
    }
    Shadows.shadowOf(pfd).file = new RandomAccessFile(file, mode == ParcelFileDescriptor.MODE_READ_ONLY ? "r" : "rw");
    return pfd;
  }

  @Implementation
  public FileDescriptor getFileDescriptor() {
    try {
      return file.getFD();
    } catch (IOException e) {
      throw new RuntimeException(e);
    }
  }

  @Implementation
  public long getStatSize() {
    try {
      return file.length();
    } catch (IOException e) {
      throw new RuntimeException(e);
    }
  }

  /**
   * Overrides framework to avoid call to FileDescriptor#getInt$ which does not exist on JVM.
   * Returns a fixed int (0).
   */
  @Implementation
  public int getFd() {
    return 0;
  }

  @Implementation
  public void close() throws IOException {
    file.close();
  }
}
