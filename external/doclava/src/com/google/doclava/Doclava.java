/*
 * Copyright (C) 2010 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.doclava;

import com.google.clearsilver.jsilver.JSilver;
import com.google.clearsilver.jsilver.data.Data;
import com.google.clearsilver.jsilver.resourceloader.ClassResourceLoader;
import com.google.clearsilver.jsilver.resourceloader.CompositeResourceLoader;
import com.google.clearsilver.jsilver.resourceloader.FileSystemResourceLoader;
import com.google.clearsilver.jsilver.resourceloader.ResourceLoader;

import com.sun.javadoc.*;

import java.util.*;
import java.util.jar.JarFile;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Collectors;
import java.io.*;
import java.lang.reflect.Proxy;
import java.lang.reflect.Array;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.MalformedURLException;
import java.net.URL;

public class Doclava {
  private static final String SDK_CONSTANT_ANNOTATION = "android.annotation.SdkConstant";
  private static final String SDK_CONSTANT_TYPE_ACTIVITY_ACTION =
      "android.annotation.SdkConstant.SdkConstantType.ACTIVITY_INTENT_ACTION";
  private static final String SDK_CONSTANT_TYPE_BROADCAST_ACTION =
      "android.annotation.SdkConstant.SdkConstantType.BROADCAST_INTENT_ACTION";
  private static final String SDK_CONSTANT_TYPE_SERVICE_ACTION =
      "android.annotation.SdkConstant.SdkConstantType.SERVICE_ACTION";
  private static final String SDK_CONSTANT_TYPE_CATEGORY =
      "android.annotation.SdkConstant.SdkConstantType.INTENT_CATEGORY";
  private static final String SDK_CONSTANT_TYPE_FEATURE =
      "android.annotation.SdkConstant.SdkConstantType.FEATURE";
  private static final String SDK_WIDGET_ANNOTATION = "android.annotation.Widget";
  private static final String SDK_LAYOUT_ANNOTATION = "android.annotation.Layout";

  private static final int TYPE_NONE = 0;
  private static final int TYPE_WIDGET = 1;
  private static final int TYPE_LAYOUT = 2;
  private static final int TYPE_LAYOUT_PARAM = 3;

  public static final int SHOW_PUBLIC = 0x00000001;
  public static final int SHOW_PROTECTED = 0x00000003;
  public static final int SHOW_PACKAGE = 0x00000007;
  public static final int SHOW_PRIVATE = 0x0000000f;
  public static final int SHOW_HIDDEN = 0x0000001f;

  public static int showLevel = SHOW_PROTECTED;

  public static final boolean SORT_BY_NAV_GROUPS = true;
  /* Debug output for PageMetadata, format urls from site root */
  public static boolean META_DBG=false;
  /* Generate the static html docs with devsite tempating only */
  public static boolean DEVSITE_STATIC_ONLY = false;
  /* Don't resolve @link refs found in devsite pages */
  public static boolean DEVSITE_IGNORE_JDLINKS = false;
  /* Show Preview navigation and process preview docs */
  public static boolean INCLUDE_PREVIEW = false;
  /* output en, es, ja without parent intl/ container */
  public static boolean USE_DEVSITE_LOCALE_OUTPUT_PATHS = false;
  /* generate navtree.js without other docs */
  public static boolean NAVTREE_ONLY = false;
  /* Generate reference navtree.js with all inherited members */
  public static boolean AT_LINKS_NAVTREE = false;
  public static String outputPathBase = "/";
  public static ArrayList<String> inputPathHtmlDirs = new ArrayList<String>();
  public static ArrayList<String> inputPathHtmlDir2 = new ArrayList<String>();
  public static String inputPathResourcesDir;
  public static String outputPathResourcesDir;
  public static String outputPathHtmlDirs;
  public static String outputPathHtmlDir2;
  /* Javadoc output directory and included in url path */
  public static String javadocDir = "reference/";
  public static String htmlExtension;

  public static RootDoc root;
  public static ArrayList<String[]> mHDFData = new ArrayList<String[]>();
  public static List<PageMetadata.Node> sTaglist = new ArrayList<PageMetadata.Node>();
  public static ArrayList<SampleCode> sampleCodes = new ArrayList<SampleCode>();
  public static ArrayList<SampleCode> sampleCodeGroups = new ArrayList<SampleCode>();
  public static Data samplesNavTree;
  public static Map<Character, String> escapeChars = new HashMap<Character, String>();
  public static String title = "";
  public static SinceTagger sinceTagger = new SinceTagger();
  public static ArtifactTagger artifactTagger = new ArtifactTagger();
  public static HashSet<String> knownTags = new HashSet<String>();
  public static FederationTagger federationTagger = new FederationTagger();
  public static boolean showUnannotated = false;
  public static Set<String> showAnnotations = new HashSet<String>();
  public static Set<String> hideAnnotations = new HashSet<String>();
  public static boolean showAnnotationOverridesVisibility = false;
  public static Set<String> hiddenPackages = new HashSet<String>();
  public static boolean includeAssets = true;
  public static boolean includeDefaultAssets = true;
  private static boolean generateDocs = true;
  private static boolean parseComments = false;
  private static String yamlNavFile = null;
  public static boolean documentAnnotations = false;
  public static String documentAnnotationsPath = null;
  public static Map<String, String> annotationDocumentationMap = null;
  public static boolean referenceOnly = false;
  public static boolean staticOnly = false;
  public static boolean yamlV2 = false; /* whether to build the new version of the yaml file */
  public static AuxSource auxSource = new EmptyAuxSource();
  public static Linter linter = new EmptyLinter();
  public static boolean android = false;
  public static String manifestFile = null;
  public static Map<String, String> manifestPermissions = new HashMap<>();

  public static JSilver jSilver = null;

  //API reference extensions
  private static boolean gmsRef = false;
  private static boolean gcmRef = false;
  public static String libraryRoot = null;
  private static boolean samplesRef = false;
  private static boolean sac = false;

  public static boolean checkLevel(int level) {
    return (showLevel & level) == level;
  }

  /**
   * Returns true if we should parse javadoc comments,
   * reporting errors in the process.
   */
  public static boolean parseComments() {
    return generateDocs || parseComments;
  }

  public static boolean checkLevel(boolean pub, boolean prot, boolean pkgp, boolean priv,
      boolean hidden) {
    if (hidden && !checkLevel(SHOW_HIDDEN)) {
      return false;
    }
    if (pub && checkLevel(SHOW_PUBLIC)) {
      return true;
    }
    if (prot && checkLevel(SHOW_PROTECTED)) {
      return true;
    }
    if (pkgp && checkLevel(SHOW_PACKAGE)) {
      return true;
    }
    if (priv && checkLevel(SHOW_PRIVATE)) {
      return true;
    }
    return false;
  }

  public static void main(String[] args) {
    com.sun.tools.javadoc.Main.execute(args);
  }

  public static boolean start(RootDoc r) {
    long startTime = System.nanoTime();
    String keepListFile = null;
    String proguardFile = null;
    String proofreadFile = null;
    String todoFile = null;
    String sdkValuePath = null;
    String stubsDir = null;
    // Create the dependency graph for the stubs  directory
    boolean offlineMode = false;
    String apiFile = null;
    String dexApiFile = null;
    String removedApiFile = null;
    String removedDexApiFile = null;
    String exactApiFile = null;
    String privateApiFile = null;
    String privateDexApiFile = null;
    String debugStubsFile = "";
    HashSet<String> stubPackages = null;
    HashSet<String> stubImportPackages = null;
    boolean stubSourceOnly = false;
    ArrayList<String> knownTagsFiles = new ArrayList<String>();

    root = r;

    String[][] options = r.options();
    for (String[] a : options) {
      if (a[0].equals("-d")) {
        outputPathBase = outputPathHtmlDirs = ClearPage.outputDir = a[1];
      } else if (a[0].equals("-templatedir")) {
        ClearPage.addTemplateDir(a[1]);
      } else if (a[0].equals("-hdf")) {
        mHDFData.add(new String[] {a[1], a[2]});
      } else if (a[0].equals("-knowntags")) {
        knownTagsFiles.add(a[1]);
      } else if (a[0].equals("-apidocsdir")) {
        javadocDir = a[1];
      } else if (a[0].equals("-toroot")) {
        ClearPage.toroot = a[1];
      } else if (a[0].equals("-samplecode")) {
        sampleCodes.add(new SampleCode(a[1], a[2], a[3]));
      } else if (a[0].equals("-samplegroup")) {
        sampleCodeGroups.add(new SampleCode(null, null, a[1]));
      } else if (a[0].equals("-samplesdir")) {
        getSampleProjects(new File(a[1]));
      //the destination output path for main htmldir
      } else if (a[0].equals("-htmldir")) {
        inputPathHtmlDirs.add(a[1]);
        ClearPage.htmlDirs = inputPathHtmlDirs;
      //the destination output path for additional htmldir
      } else if (a[0].equals("-htmldir2")) {
        if (a[2].equals("default")) {
          inputPathHtmlDir2.add(a[1]);
        } else {
          inputPathHtmlDir2.add(a[1]);
          outputPathHtmlDir2 = a[2];
        }
      //the destination output path for additional resources (images)
      } else if (a[0].equals("-resourcesdir")) {
        inputPathResourcesDir = a[1];
      } else if (a[0].equals("-resourcesoutdir")) {
        outputPathResourcesDir = a[1];
      } else if (a[0].equals("-title")) {
        Doclava.title = a[1];
      } else if (a[0].equals("-werror")) {
        Errors.setWarningsAreErrors(true);
      } else if (a[0].equals("-lerror")) {
        Errors.setLintsAreErrors(true);
      } else if (a[0].equals("-error") || a[0].equals("-warning") || a[0].equals("-lint")
          || a[0].equals("-hide")) {
        try {
          int level = -1;
          if (a[0].equals("-error")) {
            level = Errors.ERROR;
          } else if (a[0].equals("-warning")) {
            level = Errors.WARNING;
          } else if (a[0].equals("-lint")) {
            level = Errors.LINT;
          } else if (a[0].equals("-hide")) {
            level = Errors.HIDDEN;
          }
          Errors.setErrorLevel(Integer.parseInt(a[1]), level);
        } catch (NumberFormatException e) {
          // already printed below
          return false;
        }
      } else if (a[0].equals("-keeplist")) {
        keepListFile = a[1];
      } else if (a[0].equals("-showUnannotated")) {
        showUnannotated = true;
      } else if (a[0].equals("-showAnnotation")) {
        showAnnotations.add(a[1]);
      } else if (a[0].equals("-hideAnnotation")) {
        hideAnnotations.add(a[1]);
      } else if (a[0].equals("-showAnnotationOverridesVisibility")) {
        showAnnotationOverridesVisibility = true;
      } else if (a[0].equals("-hidePackage")) {
        hiddenPackages.add(a[1]);
      } else if (a[0].equals("-proguard")) {
        proguardFile = a[1];
      } else if (a[0].equals("-proofread")) {
        proofreadFile = a[1];
      } else if (a[0].equals("-todo")) {
        todoFile = a[1];
      } else if (a[0].equals("-public")) {
        showLevel = SHOW_PUBLIC;
      } else if (a[0].equals("-protected")) {
        showLevel = SHOW_PROTECTED;
      } else if (a[0].equals("-package")) {
        showLevel = SHOW_PACKAGE;
      } else if (a[0].equals("-private")) {
        showLevel = SHOW_PRIVATE;
      } else if (a[0].equals("-hidden")) {
        showLevel = SHOW_HIDDEN;
      } else if (a[0].equals("-stubs")) {
        stubsDir = a[1];
      } else if (a[0].equals("-stubpackages")) {
        stubPackages = new HashSet<String>();
        for (String pkg : a[1].split(":")) {
          stubPackages.add(pkg);
        }
      } else if (a[0].equals("-stubimportpackages")) {
        stubImportPackages = new HashSet<String>();
        for (String pkg : a[1].split(":")) {
          stubImportPackages.add(pkg);
          hiddenPackages.add(pkg);
        }
      } else if (a[0].equals("-stubsourceonly")) {
        stubSourceOnly = true;
      } else if (a[0].equals("-sdkvalues")) {
        sdkValuePath = a[1];
      } else if (a[0].equals("-api")) {
        apiFile = a[1];
      } else if (a[0].equals("-dexApi")) {
        dexApiFile = a[1];
      } else if (a[0].equals("-removedApi")) {
        removedApiFile = a[1];
      } else if (a[0].equals("-removedDexApi")) {
        removedDexApiFile = a[1];
      } else if (a[0].equals("-exactApi")) {
        exactApiFile = a[1];
      } else if (a[0].equals("-privateApi")) {
        privateApiFile = a[1];
      } else if (a[0].equals("-privateDexApi")) {
        privateDexApiFile = a[1];
      } else if (a[0].equals("-nodocs")) {
        generateDocs = false;
      } else if (a[0].equals("-noassets")) {
        includeAssets = false;
      } else if (a[0].equals("-nodefaultassets")) {
        includeDefaultAssets = false;
      } else if (a[0].equals("-parsecomments")) {
        parseComments = true;
      } else if (a[0].equals("-since")) {
        sinceTagger.addVersion(a[1], a[2]);
      } else if (a[0].equals("-artifact")) {
        artifactTagger.addArtifact(a[1], a[2]);
      } else if (a[0].equals("-offlinemode")) {
        offlineMode = true;
      } else if (a[0].equals("-metadataDebug")) {
        META_DBG = true;
      } else if (a[0].equals("-includePreview")) {
        INCLUDE_PREVIEW = true;
      } else if (a[0].equals("-ignoreJdLinks")) {
        if (DEVSITE_STATIC_ONLY) {
          DEVSITE_IGNORE_JDLINKS = true;
        }
      } else if (a[0].equals("-federate")) {
        try {
          String name = a[1];
          URL federationURL = new URL(a[2]);
          federationTagger.addSiteUrl(name, federationURL);
        } catch (MalformedURLException e) {
          System.err.println("Could not parse URL for federation: " + a[1]);
          return false;
        }
      } else if (a[0].equals("-federationapi")) {
        String name = a[1];
        String file = a[2];
        federationTagger.addSiteApi(name, file);
      } else if (a[0].equals("-yaml")) {
        yamlNavFile = a[1];
      } else if (a[0].equals("-dac_libraryroot")) {
        libraryRoot = ensureSlash(a[1]);
        mHDFData.add(new String[] {"library.root", a[1]});
      } else if (a[0].equals("-dac_dataname")) {
        mHDFData.add(new String[] {"dac_dataname", a[1]});
      } else if (a[0].equals("-documentannotations")) {
        documentAnnotations = true;
        documentAnnotationsPath = a[1];
      } else if (a[0].equals("-referenceonly")) {
        referenceOnly = true;
        mHDFData.add(new String[] {"referenceonly", "1"});
      } else if (a[0].equals("-staticonly")) {
        staticOnly = true;
        mHDFData.add(new String[] {"staticonly", "1"});
      } else if (a[0].equals("-navtreeonly")) {
        NAVTREE_ONLY = true;
      } else if (a[0].equals("-atLinksNavtree")) {
        AT_LINKS_NAVTREE = true;
      } else if (a[0].equals("-yamlV2")) {
        yamlV2 = true;
      } else if (a[0].equals("-devsite")) {
        // Don't copy any assets to devsite output
        includeAssets = false;
        USE_DEVSITE_LOCALE_OUTPUT_PATHS = true;
        mHDFData.add(new String[] {"devsite", "1"});
        if (staticOnly) {
          DEVSITE_STATIC_ONLY = true;
          System.out.println("  ... Generating static html only for devsite");
        }
        if (yamlNavFile == null) {
          yamlNavFile = "_book.yaml";
        }
      } else if (a[0].equals("-android")) {
        auxSource = new AndroidAuxSource();
        linter = new AndroidLinter();
        android = true;
      } else if (a[0].equals("-manifest")) {
        manifestFile = a[1];
      }
    }

    // If the caller has not explicitly requested that unannotated classes and members should be
    // shown in the output then only show them if no annotations were provided.
    if (!showUnannotated && showAnnotations.isEmpty()) {
      showUnannotated = true;
    }

    if (!readKnownTagsFiles(knownTags, knownTagsFiles)) {
      return false;
    }
    if (!readManifest()) {
      return false;
    }

    // Set up the data structures
    Converter.makeInfo(r);

    if (generateDocs) {
      ClearPage.addBundledTemplateDir("assets/customizations");
      ClearPage.addBundledTemplateDir("assets/templates");

      List<ResourceLoader> resourceLoaders = new ArrayList<ResourceLoader>();
      List<String> templates = ClearPage.getTemplateDirs();
      for (String tmpl : templates) {
        resourceLoaders.add(new FileSystemResourceLoader(tmpl));
      }
      // If no custom template path is provided, and this is a devsite build,
      // then use the bundled templates-sdk/ files by default
      if (templates.isEmpty() && USE_DEVSITE_LOCALE_OUTPUT_PATHS) {
        resourceLoaders.add(new ClassResourceLoader(Doclava.class, "/assets/templates-sdk"));
        System.out.println("\n#########  OK, Using templates-sdk ############\n");
      }

      templates = ClearPage.getBundledTemplateDirs();
      for (String tmpl : templates) {
          // TODO - remove commented line - it's here for debugging purposes
        //  resourceLoaders.add(new FileSystemResourceLoader("/Volumes/Android/master/external/doclava/res/" + tmpl));
        resourceLoaders.add(new ClassResourceLoader(Doclava.class, '/'+tmpl));
      }

      ResourceLoader compositeResourceLoader = new CompositeResourceLoader(resourceLoaders);
      jSilver = new JSilver(compositeResourceLoader);

      if (!Doclava.readTemplateSettings()) {
        return false;
      }

      // if requested, only generate the navtree for ds use-case
      if (NAVTREE_ONLY) {
        if (AT_LINKS_NAVTREE) {
          AtLinksNavTree.writeAtLinksNavTree(javadocDir);
        } else {
          NavTree.writeNavTree(javadocDir, "");
        }
        return true;
      }

      // don't do ref doc tasks in devsite static-only builds
      if (!DEVSITE_STATIC_ONLY) {
        // Load additional data structures from federated sites.
        for(FederatedSite site : federationTagger.getSites()) {
          Converter.addApiInfo(site.apiInfo());
        }

        // Apply @since tags from the XML file
        sinceTagger.tagAll(Converter.rootClasses());

        // Apply @artifact tags from the XML file
        artifactTagger.tagAll(Converter.rootClasses());

        // Apply details of federated documentation
        federationTagger.tagAll(Converter.rootClasses());

        // Files for proofreading
        if (proofreadFile != null) {
          Proofread.initProofread(proofreadFile);
        }
        if (todoFile != null) {
          TodoFile.writeTodoFile(todoFile);
        }

        if (samplesRef) {
          // always write samples without offlineMode behaviors
          writeSamples(false, sampleCodes, SORT_BY_NAV_GROUPS);
        }
      }
      if (!referenceOnly) {
        // HTML2 Pages -- Generate Pages from optional secondary dir
        if (!inputPathHtmlDir2.isEmpty()) {
          if (!outputPathHtmlDir2.isEmpty()) {
            ClearPage.outputDir = outputPathBase + "/" + outputPathHtmlDir2;
          }
          ClearPage.htmlDirs = inputPathHtmlDir2;
          writeHTMLPages();
          ClearPage.htmlDirs = inputPathHtmlDirs;
        }

        // HTML Pages
        if (!ClearPage.htmlDirs.isEmpty()) {
          ClearPage.htmlDirs = inputPathHtmlDirs;
          if (USE_DEVSITE_LOCALE_OUTPUT_PATHS) {
            ClearPage.outputDir = outputPathHtmlDirs + "/en/";
          } else {
            ClearPage.outputDir = outputPathHtmlDirs;
          }
          writeHTMLPages();
        }
      }

      writeResources();

      writeAssets();

      // don't do ref doc tasks in devsite static-only builds
      if (!DEVSITE_STATIC_ONLY) {
        // Navigation tree
        String refPrefix = new String();
        if(gmsRef){
          refPrefix = "gms-";
        } else if(gcmRef){
          refPrefix = "gcm-";
        }
        NavTree.writeNavTree(javadocDir, refPrefix);

        // Write yaml tree.
        if (yamlNavFile != null){
          NavTree.writeYamlTree(javadocDir, yamlNavFile);
          if (yamlV2) {
            // Generate both for good measure, to make transitions easier, but change the filename
            // for the new one so there's yet another explicit opt-in required by fixing the name.
            yamlNavFile = "_NEW" + yamlNavFile;
            NavTree.writeYamlTree2(javadocDir, yamlNavFile);
          }
        }

        // Packages Pages
        writePackages(refPrefix + "packages" + htmlExtension);

        // Classes
        writeClassLists();
        writeClasses();
        writeHierarchy();
        // writeKeywords();

        // Lists for JavaScript
        writeLists();
        if (keepListFile != null) {
          writeKeepList(keepListFile);
        }

        Proofread.finishProofread(proofreadFile);

        if (sdkValuePath != null) {
          writeSdkValues(sdkValuePath);
        }
      }
      // Write metadata for all processed files to jd_lists_unified in out dir
      if (!sTaglist.isEmpty()) {
        PageMetadata.WriteListByLang(sTaglist);
        // For devsite (ds) reference only, write samples_metadata to out dir
        if ((USE_DEVSITE_LOCALE_OUTPUT_PATHS) && (!DEVSITE_STATIC_ONLY)) {
          PageMetadata.WriteSamplesListByLang(sTaglist);
        }
      }
    }

    // Stubs
    if (stubsDir != null || apiFile != null || dexApiFile != null || proguardFile != null
        || removedApiFile != null || removedDexApiFile != null || exactApiFile != null
        || privateApiFile != null || privateDexApiFile != null) {
      Stubs.writeStubsAndApi(stubsDir, apiFile, dexApiFile, proguardFile, removedApiFile,
          removedDexApiFile, exactApiFile, privateApiFile, privateDexApiFile, stubPackages,
          stubImportPackages, stubSourceOnly);
    }

    Errors.printErrors();

    long time = System.nanoTime() - startTime;
    System.out.println("DroidDoc took " + (time / 1000000000) + " sec. to write docs to "
        + outputPathBase );

    return !Errors.hadError;
  }

  private static void writeIndex(String dir) {
    Data data = makeHDF();
    ClearPage.write(data, "index.cs", dir + "index" + htmlExtension);
  }

  private static boolean readTemplateSettings() {
    Data data = makeHDF();

    // The .html extension is hard-coded in several .cs files,
    // and so you cannot currently set it as a property.
    htmlExtension = ".html";
    // htmlExtension = data.getValue("template.extension", ".html");
    int i = 0;
    while (true) {
      String k = data.getValue("template.escape." + i + ".key", "");
      String v = data.getValue("template.escape." + i + ".value", "");
      if ("".equals(k)) {
        break;
      }
      if (k.length() != 1) {
        System.err.println("template.escape." + i + ".key must have a length of 1: " + k);
        return false;
      }
      escapeChars.put(k.charAt(0), v);
      i++;
    }
    return true;
  }

    private static boolean readKnownTagsFiles(HashSet<String> knownTags,
            ArrayList<String> knownTagsFiles) {
        for (String fn: knownTagsFiles) {
           BufferedReader in = null;
           try {
               in = new BufferedReader(new FileReader(fn));
               int lineno = 0;
               boolean fail = false;
               while (true) {
                   lineno++;
                   String line = in.readLine();
                   if (line == null) {
                       break;
                   }
                   line = line.trim();
                   if (line.length() == 0) {
                       continue;
                   } else if (line.charAt(0) == '#') {
                       continue;
                   }
                   String[] words = line.split("\\s+", 2);
                   if (words.length == 2) {
                       if (words[1].charAt(0) != '#') {
                           System.err.println(fn + ":" + lineno
                                   + ": Only one tag allowed per line: " + line);
                           fail = true;
                           continue;
                       }
                   }
                   knownTags.add(words[0]);
               }
               if (fail) {
                   return false;
               }
           } catch (IOException ex) {
               System.err.println("Error reading file: " + fn + " (" + ex.getMessage() + ")");
               return false;
           } finally {
               if (in != null) {
                   try {
                       in.close();
                   } catch (IOException e) {
                   }
               }
           }
        }
        return true;
    }

  private static boolean readManifest() {
    manifestPermissions.clear();
    if (manifestFile == null) {
      return true;
    }
    try (BufferedInputStream in = new BufferedInputStream(new FileInputStream(manifestFile));
        ByteArrayOutputStream out = new ByteArrayOutputStream()) {
      byte[] buffer = new byte[1024];
      int count;
      while ((count = in.read(buffer)) != -1) {
        out.write(buffer, 0, count);
      }
      final Matcher m = Pattern.compile("(?s)<permission "
          + "[^>]*android:name=\"([^\">]+)\""
          + "[^>]*android:protectionLevel=\"([^\">]+)\"").matcher(out.toString());
      while (m.find()) {
        manifestPermissions.put(m.group(1), m.group(2));
      }
    } catch (IOException e) {
      Errors.error(Errors.PARSE_ERROR, (SourcePositionInfo) null,
          "Failed to parse " + manifestFile + ": " + e);
      return false;
    }
    return true;
  }

  public static String escape(String s) {
    if (escapeChars.size() == 0) {
      return s;
    }
    StringBuffer b = null;
    int begin = 0;
    final int N = s.length();
    for (int i = 0; i < N; i++) {
      char c = s.charAt(i);
      String mapped = escapeChars.get(c);
      if (mapped != null) {
        if (b == null) {
          b = new StringBuffer(s.length() + mapped.length());
        }
        if (begin != i) {
          b.append(s.substring(begin, i));
        }
        b.append(mapped);
        begin = i + 1;
      }
    }
    if (b != null) {
      if (begin != N) {
        b.append(s.substring(begin, N));
      }
      return b.toString();
    }
    return s;
  }

  public static void setPageTitle(Data data, String title) {
    String s = title;
    if (Doclava.title.length() > 0) {
      s += " - " + Doclava.title;
    }
    data.setValue("page.title", s);
  }


  public static LanguageVersion languageVersion() {
    return LanguageVersion.JAVA_1_5;
  }


  public static int optionLength(String option) {
    if (option.equals("-d")) {
      return 2;
    }
    if (option.equals("-templatedir")) {
      return 2;
    }
    if (option.equals("-hdf")) {
      return 3;
    }
    if (option.equals("-knowntags")) {
      return 2;
    }
    if (option.equals("-apidocsdir")) {
      return 2;
    }
    if (option.equals("-toroot")) {
      return 2;
    }
    if (option.equals("-samplecode")) {
      samplesRef = true;
      return 4;
    }
    if (option.equals("-samplegroup")) {
      return 2;
    }
    if (option.equals("-samplesdir")) {
      samplesRef = true;
      return 2;
    }
    if (option.equals("-devsite")) {
      return 1;
    }
    if (option.equals("-yamlV2")) {
      return 1;
    }
    if (option.equals("-dac_libraryroot")) {
      return 2;
    }
    if (option.equals("-dac_dataname")) {
      return 2;
    }
    if (option.equals("-ignoreJdLinks")) {
      return 1;
    }
    if (option.equals("-htmldir")) {
      return 2;
    }
    if (option.equals("-htmldir2")) {
      return 3;
    }
    if (option.equals("-resourcesdir")) {
      return 2;
    }
    if (option.equals("-resourcesoutdir")) {
      return 2;
    }
    if (option.equals("-title")) {
      return 2;
    }
    if (option.equals("-werror")) {
      return 1;
    }
    if (option.equals("-lerror")) {
      return 1;
    }
    if (option.equals("-hide")) {
      return 2;
    }
    if (option.equals("-warning")) {
      return 2;
    }
    if (option.equals("-error")) {
      return 2;
    }
    if (option.equals("-keeplist")) {
      return 2;
    }
    if (option.equals("-showUnannotated")) {
      return 1;
    }
    if (option.equals("-showAnnotation")) {
      return 2;
    }
    if (option.equals("-hideAnnotation")) {
      return 2;
    }
    if (option.equals("-showAnnotationOverridesVisibility")) {
      return 1;
    }
    if (option.equals("-hidePackage")) {
      return 2;
    }
    if (option.equals("-proguard")) {
      return 2;
    }
    if (option.equals("-proofread")) {
      return 2;
    }
    if (option.equals("-todo")) {
      return 2;
    }
    if (option.equals("-public")) {
      return 1;
    }
    if (option.equals("-protected")) {
      return 1;
    }
    if (option.equals("-package")) {
      return 1;
    }
    if (option.equals("-private")) {
      return 1;
    }
    if (option.equals("-hidden")) {
      return 1;
    }
    if (option.equals("-stubs")) {
      return 2;
    }
    if (option.equals("-stubpackages")) {
      return 2;
    }
    if (option.equals("-stubimportpackages")) {
      return 2;
    }
    if (option.equals("-stubsourceonly")) {
      return 1;
    }
    if (option.equals("-sdkvalues")) {
      return 2;
    }
    if (option.equals("-api")) {
      return 2;
    }
    if (option.equals("-dexApi")) {
      return 2;
    }
    if (option.equals("-removedApi")) {
      return 2;
    }
    if (option.equals("-removedDexApi")) {
      return 2;
    }
    if (option.equals("-exactApi")) {
      return 2;
    }
    if (option.equals("-privateApi")) {
      return 2;
    }
    if (option.equals("-privateDexApi")) {
      return 2;
    }
    if (option.equals("-nodocs")) {
      return 1;
    }
    if (option.equals("-nodefaultassets")) {
      return 1;
    }
    if (option.equals("-parsecomments")) {
      return 1;
    }
    if (option.equals("-since")) {
      return 3;
    }
    if (option.equals("-artifact")) {
      return 3;
    }
    if (option.equals("-offlinemode")) {
      return 1;
    }
    if (option.equals("-federate")) {
      return 3;
    }
    if (option.equals("-federationapi")) {
      return 3;
    }
    if (option.equals("-yaml")) {
      return 2;
    }
    if (option.equals("-gmsref")) {
      gmsRef = true;
      return 1;
    }
    if (option.equals("-gcmref")) {
      gcmRef = true;
      return 1;
    }
    if (option.equals("-metadataDebug")) {
      return 1;
    }
    if (option.equals("-includePreview")) {
      return 1;
    }
    if (option.equals("-documentannotations")) {
      return 2;
    }
    if (option.equals("-referenceonly")) {
      return 1;
    }
    if (option.equals("-staticonly")) {
      return 1;
    }
    if (option.equals("-navtreeonly")) {
      return 1;
    }
    if (option.equals("-atLinksNavtree")) {
      return 1;
    }
    if (option.equals("-android")) {
      return 1;
    }
    if (option.equals("-manifest")) {
      return 2;
    }
    return 0;
  }
  public static boolean validOptions(String[][] options, DocErrorReporter r) {
    for (String[] a : options) {
      if (a[0].equals("-error") || a[0].equals("-warning") || a[0].equals("-hide")) {
        try {
          Integer.parseInt(a[1]);
        } catch (NumberFormatException e) {
          r.printError("bad -" + a[0] + " value must be a number: " + a[1]);
          return false;
        }
      }
    }

    return true;
  }

  public static Data makeHDF() {
    Data data = jSilver.createData();

    for (String[] p : mHDFData) {
      data.setValue(p[0], p[1]);
    }

    return data;
  }

  public static Data makePackageHDF() {
    Data data = makeHDF();
    Collection<ClassInfo> classes = Converter.rootClasses();

    SortedMap<String, PackageInfo> sorted = new TreeMap<String, PackageInfo>();
    for (ClassInfo cl : classes) {
      PackageInfo pkg = cl.containingPackage();
      String name;
      if (pkg == null) {
        name = "";
      } else {
        name = pkg.name();
      }
      sorted.put(name, pkg);
    }

    int i = 0;
    for (Map.Entry<String, PackageInfo> entry : sorted.entrySet()) {
      String s = entry.getKey();
      PackageInfo pkg = entry.getValue();

      if (pkg.isHiddenOrRemoved()) {
        continue;
      }
      boolean allHiddenOrRemoved = true;
      int pass = 0;
      ClassInfo[] classesToCheck = null;
      while (pass < 6) {
        switch (pass) {
          case 0:
            classesToCheck = pkg.ordinaryClasses();
            break;
          case 1:
            classesToCheck = pkg.enums();
            break;
          case 2:
            classesToCheck = pkg.errors();
            break;
          case 3:
            classesToCheck = pkg.exceptions();
            break;
          case 4:
            classesToCheck = pkg.interfaces();
            break;
          case 5:
            classesToCheck = pkg.annotations();
            break;
          default:
            System.err.println("Error reading package: " + pkg.name());
            break;
        }
        for (ClassInfo cl : classesToCheck) {
          if (!cl.isHiddenOrRemoved()) {
            allHiddenOrRemoved = false;
            break;
          }
        }
        if (!allHiddenOrRemoved) {
          break;
        }
        pass++;
      }
      if (allHiddenOrRemoved) {
        continue;
      }
      if(gmsRef){
          data.setValue("reference.gms", "true");
      } else if(gcmRef){
          data.setValue("reference.gcm", "true");
      }
      data.setValue("reference", "1");
      data.setValue("reference.apilevels", sinceTagger.hasVersions() ? "1" : "0");
      data.setValue("reference.artifacts", artifactTagger.hasArtifacts() ? "1" : "0");
      data.setValue("docs.packages." + i + ".name", s);
      data.setValue("docs.packages." + i + ".link", pkg.htmlPage());
      data.setValue("docs.packages." + i + ".since", pkg.getSince());
      TagInfo.makeHDF(data, "docs.packages." + i + ".shortDescr", pkg.firstSentenceTags());
      i++;
    }

    sinceTagger.writeVersionNames(data);
    return data;
  }

  private static void writeDirectory(File dir, String relative, JSilver js) {
    File[] files = dir.listFiles();
    int i, count = files.length;
    for (i = 0; i < count; i++) {
      File f = files[i];
      if (f.isFile()) {
        String templ = relative + f.getName();
        int len = templ.length();
        if (len > 3 && ".cs".equals(templ.substring(len - 3))) {
          Data data = makePackageHDF();
          String filename = templ.substring(0, len - 3) + htmlExtension;
          ClearPage.write(data, templ, filename, js);
        } else if (len > 3 && ".jd".equals(templ.substring(len - 3))) {
          Data data = makePackageHDF();
          String filename = templ.substring(0, len - 3) + htmlExtension;
          DocFile.writePage(f.getAbsolutePath(), relative, filename, data);
        } else if(!f.getName().equals(".DS_Store")){
              Data data = makeHDF();
              String hdfValue = data.getValue("sac") == null ? "" : data.getValue("sac");
              boolean allowExcepted = hdfValue.equals("true") ? true : false;
              boolean append = false;
              ClearPage.copyFile(allowExcepted, f, templ, append);
        }
      } else if (f.isDirectory()) {
        writeDirectory(f, relative + f.getName() + "/", js);
      }
    }
  }

  public static void writeHTMLPages() {
    for (String htmlDir : ClearPage.htmlDirs) {
      File f = new File(htmlDir);
      if (!f.isDirectory()) {
        System.err.println("htmlDir not a directory: " + htmlDir);
        continue;
      }

      ResourceLoader loader = new FileSystemResourceLoader(f);
      JSilver js = new JSilver(loader);
      writeDirectory(f, "", js);
    }
  }

  /* copy files supplied by the -resourcesdir flag */
  public static void writeResources() {
    if (inputPathResourcesDir != null && !inputPathResourcesDir.isEmpty()) {
      try {
        File f = new File(inputPathResourcesDir);
        if (!f.isDirectory()) {
          System.err.println("resourcesdir is not a directory: " + inputPathResourcesDir);
          return;
        }

        ResourceLoader loader = new FileSystemResourceLoader(f);
        JSilver js = new JSilver(loader);
        writeDirectory(f, outputPathResourcesDir, js);
      } catch(Exception e) {
        System.err.println("Could not copy resourcesdir: " + e);
      }
    }
  }

  public static void writeAssets() {
    if (!includeAssets) return;
    JarFile thisJar = JarUtils.jarForClass(Doclava.class, null);
    if ((thisJar != null) && (includeDefaultAssets)) {
      try {
        List<String> templateDirs = ClearPage.getBundledTemplateDirs();
        for (String templateDir : templateDirs) {
          String assetsDir = templateDir + "/assets";
          JarUtils.copyResourcesToDirectory(thisJar, assetsDir, ClearPage.outputDir + "/assets");
        }
      } catch (IOException e) {
        System.err.println("Error copying assets directory.");
        e.printStackTrace();
        return;
      }
    }

    //write the project-specific assets
    List<String> templateDirs = ClearPage.getTemplateDirs();
    for (String templateDir : templateDirs) {
      File assets = new File(templateDir + "/assets");
      if (assets.isDirectory()) {
        writeDirectory(assets, "assets/", null);
      }
    }

    // Create the timestamp.js file based on .cs file
    Data timedata = Doclava.makeHDF();
    ClearPage.write(timedata, "timestamp.cs", "timestamp.js");
  }

  /** Go through the docs and generate meta-data about each
      page to use in search suggestions */
  public static void writeLists() {

    // Write the lists for API references
    Data data = makeHDF();

    Collection<ClassInfo> classes = Converter.rootClasses();

    SortedMap<String, Object> sorted = new TreeMap<String, Object>();
    for (ClassInfo cl : classes) {
      if (cl.isHiddenOrRemoved()) {
        continue;
      }
      sorted.put(cl.qualifiedName(), cl);
      PackageInfo pkg = cl.containingPackage();
      String name;
      if (pkg == null) {
        name = "";
      } else {
        name = pkg.name();
      }
      sorted.put(name, pkg);
    }

    int i = 0;
    String listDir = javadocDir;
    if (USE_DEVSITE_LOCALE_OUTPUT_PATHS) {
      if (libraryRoot != null) {
        listDir = listDir + libraryRoot;
      }
    }
    for (String s : sorted.keySet()) {
      data.setValue("docs.pages." + i + ".id", "" + i);
      data.setValue("docs.pages." + i + ".label", s);

      Object o = sorted.get(s);
      if (o instanceof PackageInfo) {
        PackageInfo pkg = (PackageInfo) o;
        data.setValue("docs.pages." + i + ".link", pkg.htmlPage());
        data.setValue("docs.pages." + i + ".type", "package");
        data.setValue("docs.pages." + i + ".deprecated", pkg.isDeprecated() ? "true" : "false");
      } else if (o instanceof ClassInfo) {
        ClassInfo cl = (ClassInfo) o;
        data.setValue("docs.pages." + i + ".link", cl.htmlPage());
        data.setValue("docs.pages." + i + ".type", "class");
        data.setValue("docs.pages." + i + ".deprecated", cl.isDeprecated() ? "true" : "false");
      }
      i++;
    }
    ClearPage.write(data, "lists.cs", listDir + "lists.js");


    // Write the lists for JD documents (if there are HTML directories to process)
    // Skip this for devsite builds
    if ((inputPathHtmlDirs.size() > 0) && (!USE_DEVSITE_LOCALE_OUTPUT_PATHS)) {
      Data jddata = makeHDF();
      Iterator counter = new Iterator();
      for (String htmlDir : inputPathHtmlDirs) {
        File dir = new File(htmlDir);
        if (!dir.isDirectory()) {
          continue;
        }
        writeJdDirList(dir, jddata, counter);
      }
      ClearPage.write(jddata, "jd_lists.cs", javadocDir + "jd_lists.js");
    }
  }

  private static class Iterator {
    int i = 0;
  }

  /** Write meta-data for a JD file, used for search suggestions */
  private static void writeJdDirList(File dir, Data data, Iterator counter) {
    File[] files = dir.listFiles();
    int i, count = files.length;
    // Loop all files in given directory
    for (i = 0; i < count; i++) {
      File f = files[i];
      if (f.isFile()) {
        String filePath = f.getAbsolutePath();
        String templ = f.getName();
        int len = templ.length();
        // If it's a .jd file we want to process
        if (len > 3 && ".jd".equals(templ.substring(len - 3))) {
          // remove the directories below the site root
          String webPath = filePath.substring(filePath.indexOf("docs/html/") + 10,
              filePath.length());
          // replace .jd with .html
          webPath = webPath.substring(0, webPath.length() - 3) + htmlExtension;
          // Parse the .jd file for properties data at top of page
          Data hdf = Doclava.makeHDF();
          String filedata = DocFile.readFile(filePath);
          Matcher lines = DocFile.LINE.matcher(filedata);
          String line = null;
          // Get each line to add the key-value to hdf
          while (lines.find()) {
            line = lines.group(1);
            if (line.length() > 0) {
              // Stop when we hit the body
              if (line.equals("@jd:body")) {
                break;
              }
              Matcher prop = DocFile.PROP.matcher(line);
              if (prop.matches()) {
                String key = prop.group(1);
                String value = prop.group(2);
                hdf.setValue(key, value);
              } else {
                break;
              }
            }
          } // done gathering page properties

          // Insert the goods into HDF data (title, link, tags, type)
          String title = hdf.getValue("page.title", "");
          title = title.replaceAll("\"", "'");
          // if there's a <span> in the title, get rid of it
          if (title.indexOf("<span") != -1) {
            String[] splitTitle = title.split("<span(.*?)</span>");
            title = splitTitle[0];
            for (int j = 1; j < splitTitle.length; j++) {
              title.concat(splitTitle[j]);
            }
          }

          StringBuilder tags =  new StringBuilder();
          String tagsList = hdf.getValue("page.tags", "");
          if (!tagsList.equals("")) {
            tagsList = tagsList.replaceAll("\"", "");
            String[] tagParts = tagsList.split(",");
            for (int iter = 0; iter < tagParts.length; iter++) {
              tags.append("\"");
              tags.append(tagParts[iter].trim());
              tags.append("\"");
              if (iter < tagParts.length - 1) {
                tags.append(",");
              }
            }
          }

          String dirName = (webPath.indexOf("/") != -1)
                  ? webPath.substring(0, webPath.indexOf("/")) : "";

          if (!"".equals(title) &&
              !"intl".equals(dirName) &&
              !hdf.getBooleanValue("excludeFromSuggestions")) {
            data.setValue("docs.pages." + counter.i + ".label", title);
            data.setValue("docs.pages." + counter.i + ".link", webPath);
            data.setValue("docs.pages." + counter.i + ".tags", tags.toString());
            data.setValue("docs.pages." + counter.i + ".type", dirName);
            counter.i++;
          }
        }
      } else if (f.isDirectory()) {
        writeJdDirList(f, data, counter);
      }
    }
  }

  public static void cantStripThis(ClassInfo cl, HashSet<ClassInfo> notStrippable) {
    if (!notStrippable.add(cl)) {
      // slight optimization: if it already contains cl, it already contains
      // all of cl's parents
      return;
    }
    ClassInfo supr = cl.superclass();
    if (supr != null) {
      cantStripThis(supr, notStrippable);
    }
    for (ClassInfo iface : cl.interfaces()) {
      cantStripThis(iface, notStrippable);
    }
  }

  private static String getPrintableName(ClassInfo cl) {
    ClassInfo containingClass = cl.containingClass();
    if (containingClass != null) {
      // This is an inner class.
      String baseName = cl.name();
      baseName = baseName.substring(baseName.lastIndexOf('.') + 1);
      return getPrintableName(containingClass) + '$' + baseName;
    }
    return cl.qualifiedName();
  }

  /**
   * Writes the list of classes that must be present in order to provide the non-hidden APIs known
   * to javadoc.
   *
   * @param filename the path to the file to write the list to
   */
  public static void writeKeepList(String filename) {
    HashSet<ClassInfo> notStrippable = new HashSet<ClassInfo>();
    Collection<ClassInfo> all = Converter.allClasses().stream().sorted(ClassInfo.comparator)
        .collect(Collectors.toList());

    // If a class is public and not hidden, then it and everything it derives
    // from cannot be stripped. Otherwise we can strip it.
    for (ClassInfo cl : all) {
      if (cl.isPublic() && !cl.isHiddenOrRemoved()) {
        cantStripThis(cl, notStrippable);
      }
    }
    PrintStream stream = null;
    try {
      stream = new PrintStream(new BufferedOutputStream(new FileOutputStream(filename)));
      for (ClassInfo cl : notStrippable) {
        stream.println(getPrintableName(cl));
      }
    } catch (FileNotFoundException e) {
      System.err.println("error writing file: " + filename);
    } finally {
      if (stream != null) {
        stream.close();
      }
    }
  }

  private static PackageInfo[] sVisiblePackages = null;

  public static PackageInfo[] choosePackages() {
    if (sVisiblePackages != null) {
      return sVisiblePackages;
    }

    Collection<ClassInfo> classes = Converter.rootClasses();
    SortedMap<String, PackageInfo> sorted = new TreeMap<String, PackageInfo>();
    for (ClassInfo cl : classes) {
      PackageInfo pkg = cl.containingPackage();
      String name;
      if (pkg == null) {
        name = "";
      } else {
        name = pkg.name();
      }
      sorted.put(name, pkg);
    }

    ArrayList<PackageInfo> result = new ArrayList<PackageInfo>();

    for (String s : sorted.keySet()) {
      PackageInfo pkg = sorted.get(s);

      if (pkg.isHiddenOrRemoved()) {
        continue;
      }

      boolean allHiddenOrRemoved = true;
      int pass = 0;
      ClassInfo[] classesToCheck = null;
      while (pass < 6) {
        switch (pass) {
          case 0:
            classesToCheck = pkg.ordinaryClasses();
            break;
          case 1:
            classesToCheck = pkg.enums();
            break;
          case 2:
            classesToCheck = pkg.errors();
            break;
          case 3:
            classesToCheck = pkg.exceptions();
            break;
          case 4:
            classesToCheck = pkg.interfaces();
            break;
          case 5:
            classesToCheck = pkg.annotations();
            break;
          default:
            System.err.println("Error reading package: " + pkg.name());
            break;
        }
        for (ClassInfo cl : classesToCheck) {
          if (!cl.isHiddenOrRemoved()) {
            allHiddenOrRemoved = false;
            break;
          }
        }
        if (!allHiddenOrRemoved) {
          break;
        }
        pass++;
      }
      if (allHiddenOrRemoved) {
        continue;
      }

      result.add(pkg);
    }

    sVisiblePackages = result.toArray(new PackageInfo[result.size()]);
    return sVisiblePackages;
  }

  public static void writePackages(String filename) {
    Data data = makePackageHDF();

    int i = 0;
    for (PackageInfo pkg : choosePackages()) {
      writePackage(pkg);

      data.setValue("docs.packages." + i + ".name", pkg.name());
      data.setValue("docs.packages." + i + ".link", pkg.htmlPage());
      TagInfo.makeHDF(data, "docs.packages." + i + ".shortDescr", pkg.firstSentenceTags());

      i++;
    }

    setPageTitle(data, "Package Index");

    TagInfo.makeHDF(data, "root.descr", Converter.convertTags(root.inlineTags(), null));

    String packageDir = javadocDir;
    if (USE_DEVSITE_LOCALE_OUTPUT_PATHS) {
      if (libraryRoot != null) {
        packageDir = packageDir + libraryRoot;
      }
    }
    data.setValue("page.not-api", "true");
    ClearPage.write(data, "packages.cs", packageDir + filename);
    ClearPage.write(data, "package-list.cs", packageDir + "package-list");

    Proofread.writePackages(filename, Converter.convertTags(root.inlineTags(), null));
  }

  public static void writePackage(PackageInfo pkg) {
    // these this and the description are in the same directory,
    // so it's okay
    Data data = makePackageHDF();

    String name = pkg.name();

    data.setValue("package.name", name);
    data.setValue("package.since", pkg.getSince());
    data.setValue("package.descr", "...description...");
    pkg.setFederatedReferences(data, "package");

    makeClassListHDF(data, "package.annotations", ClassInfo.sortByName(pkg.annotations()));
    makeClassListHDF(data, "package.interfaces", ClassInfo.sortByName(pkg.interfaces()));
    makeClassListHDF(data, "package.classes", ClassInfo.sortByName(pkg.ordinaryClasses()));
    makeClassListHDF(data, "package.enums", ClassInfo.sortByName(pkg.enums()));
    makeClassListHDF(data, "package.exceptions", ClassInfo.sortByName(pkg.exceptions()));
    makeClassListHDF(data, "package.errors", ClassInfo.sortByName(pkg.errors()));
    TagInfo.makeHDF(data, "package.shortDescr", pkg.firstSentenceTags());
    TagInfo.makeHDF(data, "package.descr", pkg.inlineTags());

    String filename = pkg.htmlPage();
    setPageTitle(data, name);
    ClearPage.write(data, "package.cs", filename);

    Proofread.writePackage(filename, pkg.inlineTags());
  }

  public static void writeClassLists() {
    int i;
    Data data = makePackageHDF();

    ClassInfo[] classes = PackageInfo.filterHiddenAndRemoved(
        Converter.convertClasses(root.classes()));
    if (classes.length == 0) {
      return;
    }

    Sorter[] sorted = new Sorter[classes.length];
    for (i = 0; i < sorted.length; i++) {
      ClassInfo cl = classes[i];
      String name = cl.name();
      sorted[i] = new Sorter(name, cl);
    }

    Arrays.sort(sorted);

    // make a pass and resolve ones that have the same name
    int firstMatch = 0;
    String lastName = sorted[0].label;
    for (i = 1; i < sorted.length; i++) {
      String s = sorted[i].label;
      if (!lastName.equals(s)) {
        if (firstMatch != i - 1) {
          // there were duplicates
          for (int j = firstMatch; j < i; j++) {
            PackageInfo pkg = ((ClassInfo) sorted[j].data).containingPackage();
            if (pkg != null) {
              sorted[j].label = sorted[j].label + " (" + pkg.name() + ")";
            }
          }
        }
        firstMatch = i;
        lastName = s;
      }
    }

    // and sort again
    Arrays.sort(sorted);

    for (i = 0; i < sorted.length; i++) {
      String s = sorted[i].label;
      ClassInfo cl = (ClassInfo) sorted[i].data;
      char first = Character.toUpperCase(s.charAt(0));
      cl.makeShortDescrHDF(data, "docs.classes." + first + '.' + i);
    }

    String packageDir = javadocDir;
    if (USE_DEVSITE_LOCALE_OUTPUT_PATHS) {
      if (libraryRoot != null) {
        packageDir = packageDir + libraryRoot;
      }
    }

    data.setValue("page.not-api", "true");
    setPageTitle(data, "Class Index");
    ClearPage.write(data, "classes.cs", packageDir + "classes" + htmlExtension);

    // Index page redirects to the classes.html page, so use the same directory
    writeIndex(packageDir);
  }

  // we use the word keywords because "index" means something else in html land
  // the user only ever sees the word index
  /*
   * public static void writeKeywords() { ArrayList<KeywordEntry> keywords = new
   * ArrayList<KeywordEntry>();
   *
   * ClassInfo[] classes = PackageInfo.filterHiddenAndRemoved(Converter.convertClasses(root.classes()));
   *
   * for (ClassInfo cl: classes) { cl.makeKeywordEntries(keywords); }
   *
   * HDF data = makeHDF();
   *
   * Collections.sort(keywords);
   *
   * int i=0; for (KeywordEntry entry: keywords) { String base = "keywords." + entry.firstChar() +
   * "." + i; entry.makeHDF(data, base); i++; }
   *
   * setPageTitle(data, "Index"); ClearPage.write(data, "keywords.cs", javadocDir + "keywords" +
   * htmlExtension); }
   */

  public static void writeHierarchy() {
    Collection<ClassInfo> classes = Converter.rootClasses();
    ArrayList<ClassInfo> info = new ArrayList<ClassInfo>();
    for (ClassInfo cl : classes) {
      if (!cl.isHiddenOrRemoved()) {
        info.add(cl);
      }
    }
    Data data = makePackageHDF();
    Hierarchy.makeHierarchy(data, info.toArray(new ClassInfo[info.size()]));
    setPageTitle(data, "Class Hierarchy");
    ClearPage.write(data, "hierarchy.cs", javadocDir + "hierarchy" + htmlExtension);
  }

  public static void writeClasses() {
    Collection<ClassInfo> classes = Converter.rootClasses();

    for (ClassInfo cl : classes) {
      Data data = makePackageHDF();
      if (!cl.isHiddenOrRemoved()) {
        writeClass(cl, data);
      }
    }
  }

  public static void writeClass(ClassInfo cl, Data data) {
    cl.makeHDF(data);
    setPageTitle(data, cl.name());
    String outfile = cl.htmlPage();
    ClearPage.write(data, "class.cs", outfile);
    Proofread.writeClass(cl.htmlPage(), cl);
  }

  public static void makeClassListHDF(Data data, String base, ClassInfo[] classes) {
    for (int i = 0; i < classes.length; i++) {
      ClassInfo cl = classes[i];
      if (!cl.isHiddenOrRemoved()) {
        cl.makeShortDescrHDF(data, base + "." + i);
      }
    }
  }

  public static String linkTarget(String source, String target) {
    String[] src = source.split("/");
    String[] tgt = target.split("/");

    int srclen = src.length;
    int tgtlen = tgt.length;

    int same = 0;
    while (same < (srclen - 1) && same < (tgtlen - 1) && (src[same].equals(tgt[same]))) {
      same++;
    }

    String s = "";

    int up = srclen - same - 1;
    for (int i = 0; i < up; i++) {
      s += "../";
    }


    int N = tgtlen - 1;
    for (int i = same; i < N; i++) {
      s += tgt[i] + '/';
    }
    s += tgt[tgtlen - 1];

    return s;
  }

  /**
   * Returns true if the given element has an @hide, @removed or @pending annotation.
   */
  private static boolean hasHideOrRemovedAnnotation(Doc doc) {
    String comment = doc.getRawCommentText();
    return comment.indexOf("@hide") != -1 || comment.indexOf("@pending") != -1 ||
        comment.indexOf("@removed") != -1;
  }

  /**
   * Returns true if the given element is hidden.
   */
  private static boolean isHiddenOrRemoved(Doc doc) {
    // Methods, fields, constructors.
    if (doc instanceof MemberDoc) {
      return hasHideOrRemovedAnnotation(doc);
    }

    // Classes, interfaces, enums, annotation types.
    if (doc instanceof ClassDoc) {
      ClassDoc classDoc = (ClassDoc) doc;

      // Check the containing package.
      if (hasHideOrRemovedAnnotation(classDoc.containingPackage())) {
        return true;
      }

      // Check the class doc and containing class docs if this is a
      // nested class.
      ClassDoc current = classDoc;
      do {
        if (hasHideOrRemovedAnnotation(current)) {
          return true;
        }

        current = current.containingClass();
      } while (current != null);
    }

    return false;
  }

  /**
   * Filters out hidden and removed elements.
   */
  private static Object filterHiddenAndRemoved(Object o, Class<?> expected) {
    if (o == null) {
      return null;
    }

    Class type = o.getClass();
    if (type.getName().startsWith("com.sun.")) {
      // TODO: Implement interfaces from superclasses, too.
      return Proxy
          .newProxyInstance(type.getClassLoader(), type.getInterfaces(), new HideHandler(o));
    } else if (o instanceof Object[]) {
      Class<?> componentType = expected.getComponentType();
      Object[] array = (Object[]) o;
      List<Object> list = new ArrayList<Object>(array.length);
      for (Object entry : array) {
        if ((entry instanceof Doc) && isHiddenOrRemoved((Doc) entry)) {
          continue;
        }
        list.add(filterHiddenAndRemoved(entry, componentType));
      }
      return list.toArray((Object[]) Array.newInstance(componentType, list.size()));
    } else {
      return o;
    }
  }

  /**
   * Filters hidden elements out of method return values.
   */
  private static class HideHandler implements InvocationHandler {

    private final Object target;

    public HideHandler(Object target) {
      this.target = target;
    }

    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
      String methodName = method.getName();
      if (args != null) {
        if (methodName.equals("compareTo") || methodName.equals("equals")
            || methodName.equals("overrides") || methodName.equals("subclassOf")) {
          args[0] = unwrap(args[0]);
        }
      }

      if (methodName.equals("getRawCommentText")) {
        return filterComment((String) method.invoke(target, args));
      }

      // escape "&" in disjunctive types.
      if (proxy instanceof Type && methodName.equals("toString")) {
        return ((String) method.invoke(target, args)).replace("&", "&amp;");
      }

      try {
        return filterHiddenAndRemoved(method.invoke(target, args), method.getReturnType());
      } catch (InvocationTargetException e) {
        throw e.getTargetException();
      }
    }

    private String filterComment(String s) {
      if (s == null) {
        return null;
      }

      s = s.trim();

      // Work around off by one error
      while (s.length() >= 5 && s.charAt(s.length() - 5) == '{') {
        s += "&nbsp;";
      }

      return s;
    }

    private static Object unwrap(Object proxy) {
      if (proxy instanceof Proxy) return ((HideHandler) Proxy.getInvocationHandler(proxy)).target;
      return proxy;
    }
  }

  /**
   * Collect the values used by the Dev tools and write them in files packaged with the SDK
   *
   * @param output the ouput directory for the files.
   */
  private static void writeSdkValues(String output) {
    ArrayList<String> activityActions = new ArrayList<String>();
    ArrayList<String> broadcastActions = new ArrayList<String>();
    ArrayList<String> serviceActions = new ArrayList<String>();
    ArrayList<String> categories = new ArrayList<String>();
    ArrayList<String> features = new ArrayList<String>();

    ArrayList<ClassInfo> layouts = new ArrayList<ClassInfo>();
    ArrayList<ClassInfo> widgets = new ArrayList<ClassInfo>();
    ArrayList<ClassInfo> layoutParams = new ArrayList<ClassInfo>();

    Collection<ClassInfo> classes = Converter.allClasses();

    // The topmost LayoutParams class - android.view.ViewGroup.LayoutParams
    ClassInfo topLayoutParams = null;

    // Go through all the fields of all the classes, looking SDK stuff.
    for (ClassInfo clazz : classes) {

      // first check constant fields for the SdkConstant annotation.
      ArrayList<FieldInfo> fields = clazz.allSelfFields();
      for (FieldInfo field : fields) {
        Object cValue = field.constantValue();
        if (cValue != null) {
            ArrayList<AnnotationInstanceInfo> annotations = field.annotations();
          if (!annotations.isEmpty()) {
            for (AnnotationInstanceInfo annotation : annotations) {
              if (SDK_CONSTANT_ANNOTATION.equals(annotation.type().qualifiedName())) {
                if (!annotation.elementValues().isEmpty()) {
                  String type = annotation.elementValues().get(0).valueString();
                  if (SDK_CONSTANT_TYPE_ACTIVITY_ACTION.equals(type)) {
                    activityActions.add(cValue.toString());
                  } else if (SDK_CONSTANT_TYPE_BROADCAST_ACTION.equals(type)) {
                    broadcastActions.add(cValue.toString());
                  } else if (SDK_CONSTANT_TYPE_SERVICE_ACTION.equals(type)) {
                    serviceActions.add(cValue.toString());
                  } else if (SDK_CONSTANT_TYPE_CATEGORY.equals(type)) {
                    categories.add(cValue.toString());
                  } else if (SDK_CONSTANT_TYPE_FEATURE.equals(type)) {
                    features.add(cValue.toString());
                  }
                }
                break;
              }
            }
          }
        }
      }

      // Now check the class for @Widget or if its in the android.widget package
      // (unless the class is hidden or abstract, or non public)
      if (clazz.isHiddenOrRemoved() == false && clazz.isPublic() && clazz.isAbstract() == false) {
        boolean annotated = false;
        ArrayList<AnnotationInstanceInfo> annotations = clazz.annotations();
        if (!annotations.isEmpty()) {
          for (AnnotationInstanceInfo annotation : annotations) {
            if (SDK_WIDGET_ANNOTATION.equals(annotation.type().qualifiedName())) {
              widgets.add(clazz);
              annotated = true;
              break;
            } else if (SDK_LAYOUT_ANNOTATION.equals(annotation.type().qualifiedName())) {
              layouts.add(clazz);
              annotated = true;
              break;
            }
          }
        }

        if (annotated == false) {
          if (topLayoutParams == null
              && "android.view.ViewGroup.LayoutParams".equals(clazz.qualifiedName())) {
            topLayoutParams = clazz;
          }
          // let's check if this is inside android.widget or android.view
          if (isIncludedPackage(clazz)) {
            // now we check what this class inherits either from android.view.ViewGroup
            // or android.view.View, or android.view.ViewGroup.LayoutParams
            int type = checkInheritance(clazz);
            switch (type) {
              case TYPE_WIDGET:
                widgets.add(clazz);
                break;
              case TYPE_LAYOUT:
                layouts.add(clazz);
                break;
              case TYPE_LAYOUT_PARAM:
                layoutParams.add(clazz);
                break;
            }
          }
        }
      }
    }

    // now write the files, whether or not the list are empty.
    // the SDK built requires those files to be present.

    Collections.sort(activityActions);
    writeValues(output + "/activity_actions.txt", activityActions);

    Collections.sort(broadcastActions);
    writeValues(output + "/broadcast_actions.txt", broadcastActions);

    Collections.sort(serviceActions);
    writeValues(output + "/service_actions.txt", serviceActions);

    Collections.sort(categories);
    writeValues(output + "/categories.txt", categories);

    Collections.sort(features);
    writeValues(output + "/features.txt", features);

    // before writing the list of classes, we do some checks, to make sure the layout params
    // are enclosed by a layout class (and not one that has been declared as a widget)
    for (int i = 0; i < layoutParams.size();) {
      ClassInfo clazz = layoutParams.get(i);
      ClassInfo containingClass = clazz.containingClass();
      boolean remove = containingClass == null || layouts.indexOf(containingClass) == -1;
      // Also ensure that super classes of the layout params are in android.widget or android.view.
      while (!remove && (clazz = clazz.superclass()) != null && !clazz.equals(topLayoutParams)) {
        remove = !isIncludedPackage(clazz);
      }
      if (remove) {
        layoutParams.remove(i);
      } else {
        i++;
      }
    }

    writeClasses(output + "/widgets.txt", widgets, layouts, layoutParams);
  }

  /**
   * Check if the clazz is in package android.view or android.widget
   */
  private static boolean isIncludedPackage(ClassInfo clazz) {
    String pckg = clazz.containingPackage().name();
    return "android.widget".equals(pckg) || "android.view".equals(pckg);
  }

  /**
   * Writes a list of values into a text files.
   *
   * @param pathname the absolute os path of the output file.
   * @param values the list of values to write.
   */
  private static void writeValues(String pathname, ArrayList<String> values) {
    FileWriter fw = null;
    BufferedWriter bw = null;
    try {
      fw = new FileWriter(pathname, false);
      bw = new BufferedWriter(fw);

      for (String value : values) {
        bw.append(value).append('\n');
      }
    } catch (IOException e) {
      // pass for now
    } finally {
      try {
        if (bw != null) bw.close();
      } catch (IOException e) {
        // pass for now
      }
      try {
        if (fw != null) fw.close();
      } catch (IOException e) {
        // pass for now
      }
    }
  }

  /**
   * Writes the widget/layout/layout param classes into a text files.
   *
   * @param pathname the absolute os path of the output file.
   * @param widgets the list of widget classes to write.
   * @param layouts the list of layout classes to write.
   * @param layoutParams the list of layout param classes to write.
   */
  private static void writeClasses(String pathname, ArrayList<ClassInfo> widgets,
      ArrayList<ClassInfo> layouts, ArrayList<ClassInfo> layoutParams) {
    FileWriter fw = null;
    BufferedWriter bw = null;
    try {
      fw = new FileWriter(pathname, false);
      bw = new BufferedWriter(fw);

      // write the 3 types of classes.
      for (ClassInfo clazz : widgets) {
        writeClass(bw, clazz, 'W');
      }
      for (ClassInfo clazz : layoutParams) {
        writeClass(bw, clazz, 'P');
      }
      for (ClassInfo clazz : layouts) {
        writeClass(bw, clazz, 'L');
      }
    } catch (IOException e) {
      // pass for now
    } finally {
      try {
        if (bw != null) bw.close();
      } catch (IOException e) {
        // pass for now
      }
      try {
        if (fw != null) fw.close();
      } catch (IOException e) {
        // pass for now
      }
    }
  }

  /**
   * Writes a class name and its super class names into a {@link BufferedWriter}.
   *
   * @param writer the BufferedWriter to write into
   * @param clazz the class to write
   * @param prefix the prefix to put at the beginning of the line.
   * @throws IOException
   */
  private static void writeClass(BufferedWriter writer, ClassInfo clazz, char prefix)
      throws IOException {
    writer.append(prefix).append(clazz.qualifiedName());
    ClassInfo superClass = clazz;
    while ((superClass = superClass.superclass()) != null) {
      writer.append(' ').append(superClass.qualifiedName());
    }
    writer.append('\n');
  }

  /**
   * Checks the inheritance of {@link ClassInfo} objects. This method return
   * <ul>
   * <li>{@link #TYPE_LAYOUT}: if the class extends <code>android.view.ViewGroup</code></li>
   * <li>{@link #TYPE_WIDGET}: if the class extends <code>android.view.View</code></li>
   * <li>{@link #TYPE_LAYOUT_PARAM}: if the class extends
   * <code>android.view.ViewGroup$LayoutParams</code></li>
   * <li>{@link #TYPE_NONE}: in all other cases</li>
   * </ul>
   *
   * @param clazz the {@link ClassInfo} to check.
   */
  private static int checkInheritance(ClassInfo clazz) {
    if ("android.view.ViewGroup".equals(clazz.qualifiedName())) {
      return TYPE_LAYOUT;
    } else if ("android.view.View".equals(clazz.qualifiedName())) {
      return TYPE_WIDGET;
    } else if ("android.view.ViewGroup.LayoutParams".equals(clazz.qualifiedName())) {
      return TYPE_LAYOUT_PARAM;
    }

    ClassInfo parent = clazz.superclass();
    if (parent != null) {
      return checkInheritance(parent);
    }

    return TYPE_NONE;
  }

  /**
   * Ensures a trailing '/' at the end of a string.
   */
  static String ensureSlash(String path) {
    return path.endsWith("/") ? path : path + "/";
  }

  /**
  * Process sample projects. Generate the TOC for the samples groups and project
  * and write it to a cs var, which is then written to files during templating to
  * html output. Collect metadata from sample project _index.jd files. Copy html
  * and specific source file types to the output directory.
  */
  public static void writeSamples(boolean offlineMode, ArrayList<SampleCode> sampleCodes,
      boolean sortNavByGroups) {
    samplesNavTree = makeHDF();

    // Go through samples processing files. Create a root list for SC nodes,
    // pass it to SCs for their NavTree children and append them.
    List<SampleCode.Node> samplesList = new ArrayList<SampleCode.Node>();
    List<SampleCode.Node> sampleGroupsRootNodes = null;
    for (SampleCode sc : sampleCodes) {
      samplesList.add(sc.setSamplesTOC(offlineMode));
     }
    if (sortNavByGroups) {
      sampleGroupsRootNodes = new ArrayList<SampleCode.Node>();
      for (SampleCode gsc : sampleCodeGroups) {
        String link =  ClearPage.toroot + "samples/" + gsc.mTitle.replaceAll(" ", "").trim().toLowerCase() + ".html";
        sampleGroupsRootNodes.add(new SampleCode.Node.Builder().setLabel(gsc.mTitle).setLink(link).setType("groupholder").build());
      }
    }
    // Pass full samplesList to SC to render the samples TOC to sampleNavTree hdf
    if (!offlineMode) {
      SampleCode.writeSamplesNavTree(samplesList, sampleGroupsRootNodes);
    }
    // Iterate the samplecode projects writing the files to out
    for (SampleCode sc : sampleCodes) {
      sc.writeSamplesFiles(offlineMode);
    }
  }

  /**
  * Given an initial samples directory root, walk through the directory collecting
  * sample code project roots and adding them to an array of SampleCodes.
  * @param rootDir Root directory holding all browseable sample code projects,
  *        defined in frameworks/base/Android.mk as "-sampleDir path".
  */
  public static void getSampleProjects(File rootDir) {
    for (File f : rootDir.listFiles()) {
      String name = f.getName();
      if (f.isDirectory()) {
        if (isValidSampleProjectRoot(f)) {
          sampleCodes.add(new SampleCode(f.getAbsolutePath(), "samples/" + name, name));
        } else {
          getSampleProjects(f);
        }
      }
    }
  }

  /**
  * Test whether a given directory is the root directory for a sample code project.
  * Root directories must contain a valid _index.jd file and a src/ directory
  * or a module directory that contains a src/ directory.
  */
  public static boolean isValidSampleProjectRoot(File dir) {
    File indexJd = new File(dir, "_index.jd");
    if (!indexJd.exists()) {
      return false;
    }
    File srcDir = new File(dir, "src");
    if (srcDir.exists()) {
      return true;
    } else {
      // Look for a src/ directory one level below the root directory, so
      // modules are supported.
      for (File childDir : dir.listFiles()) {
        if (childDir.isDirectory()) {
          srcDir = new File(childDir, "src");
          if (srcDir.exists()) {
            return true;
          }
        }
      }
      return false;
    }
  }

  public static String getDocumentationStringForAnnotation(String annotationName) {
    if (!documentAnnotations) return null;
    if (annotationDocumentationMap == null) {
      // parse the file for map
      annotationDocumentationMap = new HashMap<String, String>();
      try {
        BufferedReader in = new BufferedReader(
            new FileReader(documentAnnotationsPath));
        try {
          String line = in.readLine();
          String[] split;
          while (line != null) {
            split = line.split(":");
            annotationDocumentationMap.put(split[0], split[1]);
            line = in.readLine();
          }
        } finally {
          in.close();
        }
      } catch (IOException e) {
        System.err.println("Unable to open annotations documentation file for reading: "
            + documentAnnotationsPath);
      }
    }
    return annotationDocumentationMap.get(annotationName);
  }

}
