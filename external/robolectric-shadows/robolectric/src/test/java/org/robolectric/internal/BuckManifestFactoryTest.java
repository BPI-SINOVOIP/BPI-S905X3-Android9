package org.robolectric.internal;

import static org.assertj.core.api.Assertions.assertThat;

import com.google.common.base.Charsets;
import com.google.common.io.Files;
import java.io.File;
import java.util.List;
import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.manifest.AndroidManifest;
import org.robolectric.res.FileFsFile;
import org.robolectric.res.ResourcePath;

@RunWith(JUnit4.class)
public class BuckManifestFactoryTest {

  @Rule
  public TemporaryFolder tempFolder = new TemporaryFolder();

  private Config.Builder configBuilder;
  private BuckManifestFactory buckManifestFactory;

  @Before
  public void setUp() throws Exception {
    configBuilder = Config.Builder.defaults().setPackageName("com.robolectric.buck");
    System.setProperty("buck.robolectric_manifest", "buck/AndroidManifest.xml");
    buckManifestFactory = new BuckManifestFactory();
  }

  @After
  public void tearDown() {
    System.clearProperty("buck.robolectric_manifest");
    System.clearProperty("buck.robolectric_res_directories");
    System.clearProperty("buck.robolectric_assets_directories");
  }

  @Test public void identify() throws Exception {
    ManifestIdentifier manifestIdentifier = buckManifestFactory.identify(configBuilder.build());
    assertThat(manifestIdentifier.getManifestFile())
        .isEqualTo(FileFsFile.from("buck/AndroidManifest.xml"));
    assertThat(manifestIdentifier.getPackageName())
        .isEqualTo("com.robolectric.buck");
  }

  @Test public void multiple_res_dirs() throws Exception {
    System.setProperty("buck.robolectric_res_directories", "buck/res1:buck/res2");
    System.setProperty("buck.robolectric_assets_directories", "buck/assets1:buck/assets2");

    ManifestIdentifier manifestIdentifier = buckManifestFactory.identify(configBuilder.build());
    AndroidManifest manifest = RobolectricTestRunner.createAndroidManifest(manifestIdentifier);
    assertThat(manifest.getResDirectory())
            .isEqualTo(FileFsFile.from("buck/res2"));
    assertThat(manifest.getAssetsDirectory())
            .isEqualTo(FileFsFile.from("buck/assets2"));

    List<ResourcePath> resourcePathList = manifest.getIncludedResourcePaths();
    assertThat(resourcePathList.size()).isEqualTo(3);
    assertThat(resourcePathList).containsExactly(
      new ResourcePath(manifest.getRClass(), FileFsFile.from("buck/res2"), FileFsFile.from("buck/assets2")),
      new ResourcePath(manifest.getRClass(), FileFsFile.from("buck/res1"), null),
      new ResourcePath(manifest.getRClass(), null, FileFsFile.from("buck/assets1"))
    );
  }

  @Test public void pass_multiple_res_dirs_in_file() throws Exception {
    String resDirectoriesFileName = "res-directories";
    File resDirectoriesFile = tempFolder.newFile(resDirectoriesFileName);
    Files.write("buck/res1\nbuck/res2", resDirectoriesFile, Charsets.UTF_8);
    System.setProperty("buck.robolectric_res_directories", "@" + resDirectoriesFile.getAbsolutePath());

    String assetDirectoriesFileName = "asset-directories";
    File assetDirectoriesFile = tempFolder.newFile(assetDirectoriesFileName);
    Files.write("buck/assets1\nbuck/assets2", assetDirectoriesFile, Charsets.UTF_8);
    System.setProperty("buck.robolectric_assets_directories", "@" + assetDirectoriesFile.getAbsolutePath());

    ManifestIdentifier manifestIdentifier = buckManifestFactory.identify(configBuilder.build());
    AndroidManifest manifest = RobolectricTestRunner.createAndroidManifest(manifestIdentifier);
    assertThat(manifest.getResDirectory())
        .isEqualTo(FileFsFile.from("buck/res2"));
    assertThat(manifest.getAssetsDirectory())
        .isEqualTo(FileFsFile.from("buck/assets2"));

    List<ResourcePath> resourcePathList = manifest.getIncludedResourcePaths();
    assertThat(resourcePathList.size()).isEqualTo(3);
    assertThat(resourcePathList).containsExactly(
            new ResourcePath(manifest.getRClass(), FileFsFile.from("buck/res2"), FileFsFile.from("buck/assets2")),
            new ResourcePath(manifest.getRClass(), FileFsFile.from("buck/res1"), null),
            new ResourcePath(manifest.getRClass(), null, FileFsFile.from("buck/assets1"))
    );
  }
}
