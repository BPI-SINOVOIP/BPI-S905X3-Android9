package org.robolectric.res;

import static java.util.Arrays.asList;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URI;
import java.net.URL;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.NavigableMap;
import java.util.NavigableSet;
import java.util.TreeMap;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;
import org.robolectric.util.Join;
import org.robolectric.util.Util;

abstract public class Fs {
  public static Fs fromJar(URL url) {
    return new JarFs(new File(fixFileURL(url).getPath()));
  }
  
  private static URI fixFileURL(URL u) {
    if (!"file".equals(u.getProtocol())) {
      throw new IllegalArgumentException();
    }
    return new File(u.getPath()).toURI();
  }

  /**
   * @deprecated Use {@link #fromURL(URL)} instead.
   */
  @Deprecated
  public static FsFile fileFromPath(String urlString) {
    if (urlString.startsWith("jar:")) {
      String[] parts = urlString.replaceFirst("jar:", "").split("!");
      Fs fs = new JarFs(new File(parts[0]));
      return fs.join(parts[1].substring(1));
    } else {
      return new FileFsFile(new File(urlString));
    }
  }

  public static FsFile fromURL(URL url) {
    switch (url.getProtocol()) {
      case "file":
        return new FileFsFile(new File(url.getPath()));
      case "jar":
        String[] parts = url.getPath().split("!");
        try {
          Fs fs = fromJar(new URL(parts[0]));
          return fs.join(parts[1].substring(1));
        } catch (MalformedURLException e) {
          throw new IllegalArgumentException(e);
        }
      default:
        throw new IllegalArgumentException("unsupported fs type for '" + url + "'");
    }
  }

  public static FsFile newFile(File file) {
    return new FileFsFile(file);
  }

  public static FsFile newFile(String filePath) {
    return new FileFsFile(filePath);
  }

  public static FsFile currentDirectory() {
    return newFile(new File("."));
  }

  static class JarFs extends Fs {
    private static final Map<File, NavigableMap<String, JarEntry>> CACHE =
        new LinkedHashMap<File, NavigableMap<String, JarEntry>>() {
          @Override
          protected boolean removeEldestEntry(Map.Entry<File, NavigableMap<String, JarEntry>> fileNavigableMapEntry) {
            return size() > 10;
          }
        };

    private final JarFile jarFile;
    private final NavigableMap<String, JarEntry> jarEntryMap;

    public JarFs(File file) {
      try {
        jarFile = new JarFile(file);
      } catch (IOException e) {
        throw new RuntimeException(e);
      }

      NavigableMap<String, JarEntry> cachedMap;
      synchronized (CACHE) {
        cachedMap = CACHE.get(file.getAbsoluteFile());
      }

      if (cachedMap == null) {
        cachedMap = new TreeMap<>();
        Enumeration<JarEntry> entries = jarFile.entries();
        while (entries.hasMoreElements()) {
          JarEntry jarEntry = entries.nextElement();
          cachedMap.put(jarEntry.getName(), jarEntry);
        }
        synchronized (CACHE) {
          CACHE.put(file.getAbsoluteFile(), cachedMap);
        }
      }

      jarEntryMap = cachedMap;
    }

    @Override public FsFile join(String folderBaseName) {
      return new JarFsFile(folderBaseName);
    }

    class JarFsFile implements FsFile {
      private final String path;

      public JarFsFile(String path) {
        this.path = path;
      }

      @Override public boolean exists() {
        return isFile() || isDirectory();
      }

      @Override public boolean isDirectory() {
        return jarEntryMap.containsKey(path + "/");
      }

      @Override public boolean isFile() {
        return jarEntryMap.containsKey(path);
      }

      @Override public FsFile[] listFiles() {
        if (!isDirectory()) return null;
        NavigableSet<String> strings = jarEntryMap.navigableKeySet().subSet(path + "/", false, path + "0", false);
        List<FsFile> fsFiles = new ArrayList<>();
        int startOfFilename = path.length() + 2;
        for (String string : strings) {
          int nextSlash = string.indexOf('/', startOfFilename);
          if (nextSlash == string.length() - 1) {
            // directory entry
            fsFiles.add(new JarFsFile(string.substring(0, string.length() - 1)));
          } else if (nextSlash == -1) {
            // file entry
            fsFiles.add(new JarFsFile(string));
          }
        }
        return fsFiles.toArray(new FsFile[fsFiles.size()]);
      }

      @Override public FsFile[] listFiles(Filter filter) {
        List<FsFile> filteredFsFiles = new ArrayList<>();
        FsFile[] fsFiles = listFiles();
        if (fsFiles != null) {
          for (FsFile fsFile : fsFiles) {
            if (filter.accept(fsFile)) {
              filteredFsFiles.add(fsFile);
            }
          }
        }
        return filteredFsFiles.toArray(new FsFile[filteredFsFiles.size()]);
      }

      @Override public String[] listFileNames() {
        List<String> fileNames = new ArrayList<>();
        for (FsFile fsFile : listFiles()) {
          fileNames.add(fsFile.getName());
        }
        return fileNames.toArray(new String[fileNames.size()]);
      }

      @Override public FsFile getParent() {
        int index = path.lastIndexOf('/');
        String parent = index != -1 ? path.substring(0, index) : "";
        return new JarFsFile(parent);
      }

      @Override public String getName() {
        int index = path.lastIndexOf('/');
        return index != -1 ? path.substring(index + 1, path.length()) : path;
      }

      @Override public InputStream getInputStream() throws IOException {
        return new BufferedInputStream(jarFile.getInputStream(jarEntryMap.get(path)));
      }

      @Override public byte[] getBytes() throws IOException {
        return Util.readBytes(jarFile.getInputStream(jarEntryMap.get(path)));
      }

      @Override public FsFile join(String... pathParts) {
        return new JarFsFile(path + "/" + Join.join("/", asList(pathParts)));
      }

      @Override public String getBaseName() {
        String name = getName();
        int dotIndex = name.indexOf(".");
        return dotIndex >= 0 ? name.substring(0, dotIndex) : name;
      }

      @Override public String getPath() {
        return "jar:" + getJarFileName() + "!/" + path;
      }

      @Override
      public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        JarFsFile jarFsFile = (JarFsFile) o;

        if (!getJarFileName().equals(jarFsFile.getJarFileName())) return false;
        if (!path.equals(jarFsFile.path)) return false;

        return true;
      }

      private String getJarFileName() {
        return jarFile.getName();
      }

      @Override
      public int hashCode() {
        return getJarFileName().hashCode() * 31 + path.hashCode();
      }

      @Override public String toString() {
        return getPath();
      }
    }
  }

  abstract public FsFile join(String folderBaseName);
}
