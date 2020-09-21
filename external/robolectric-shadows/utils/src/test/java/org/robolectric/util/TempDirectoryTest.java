package org.robolectric.util;

import static org.assertj.core.api.Assertions.assertThat;

import java.io.IOException;
import java.nio.file.Path;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class TempDirectoryTest {

  @Test
  public void createsDirsWithSameParent() throws IOException {
    TempDirectory tempDir = new TempDirectory("temp_dir");
    Path path = tempDir.create("dir1");
    Path path2 = tempDir.create("dir2");
    assertThat(path.getParent()).isEqualTo(path2.getParent());
  }
}
