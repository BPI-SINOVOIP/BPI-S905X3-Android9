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

import com.google.clearsilver.jsilver.data.Data;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.SortedMap;
import java.util.TreeMap;

public class NavTree {

  public static void writeNavTree(String dir, String refPrefix) {
    List<Node> children = new ArrayList<Node>();
    for (PackageInfo pkg : Doclava.choosePackages()) {
      children.add(makePackageNode(pkg));
    }
    Node node = new Node("Reference", dir + refPrefix + "packages.html", children, null);

    StringBuilder buf = new StringBuilder();
    if (false) {
      // if you want a root node
      buf.append("[");
      node.render(buf);
      buf.append("]");
    } else {
      // if you don't want a root node
      node.renderChildren(buf);
    }

    Data data = Doclava.makeHDF();
    data.setValue("reference_tree", buf.toString());
    if (refPrefix == "gms-"){
      ClearPage.write(data, "gms_navtree_data.cs", "gms_navtree_data.js");
    } else if (refPrefix == "gcm-"){
      ClearPage.write(data, "gcm_navtree_data.cs", "gcm_navtree_data.js");
    } else if (Doclava.USE_DEVSITE_LOCALE_OUTPUT_PATHS && (Doclava.libraryRoot != null)) {
        ClearPage.write(data, "navtree_data.cs", dir + Doclava.libraryRoot
          + "navtree_data.js");
    } else {
      ClearPage.write(data, "navtree_data.cs", "navtree_data.js");
    }
  }

  /**
   * Write the YAML formatted navigation tree.
   * This is intended to replace writeYamlTree(), but for now requires an explicit opt-in via
   * the yamlV2 flag in the doclava command. This version creates a yaml file with all classes,
   * interface, exceptions, etc. separated into collapsible groups.
   */
  public static void writeYamlTree2(String dir, String fileName){
    List<Node> children = new ArrayList<Node>();
    for (PackageInfo pkg : Doclava.choosePackages()) {
      children.add(makePackageNode(pkg));
    }
    Node node = new Node("Reference", Doclava.ensureSlash(dir) + "packages.html", children, null);
    StringBuilder buf = new StringBuilder();

    node.renderChildrenYaml(buf, 0);

    Data data = Doclava.makeHDF();
    data.setValue("reference_tree", buf.toString());

    if (Doclava.USE_DEVSITE_LOCALE_OUTPUT_PATHS && (Doclava.libraryRoot != null)) {
      dir = Doclava.ensureSlash(dir) + Doclava.libraryRoot;
    }

    data.setValue("docs.classes.link", Doclava.ensureSlash(dir) + "classes.html");
    data.setValue("docs.packages.link", Doclava.ensureSlash(dir) + "packages.html");

    ClearPage.write(data, "yaml_navtree2.cs", Doclava.ensureSlash(dir) + fileName);

  }


  /**
   * Write the YAML formatted navigation tree (legacy version).
   * This creates a yaml file with package names followed by all
   * classes, interfaces, exceptions, etc. But they are not separated by classes, interfaces, etc.
   * It also nests any nested classes under the parent class, instead of listing them as siblings.
   * @see "http://yaml.org/"
   */
  public static void writeYamlTree(String dir, String fileName){
    Data data = Doclava.makeHDF();
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

    data = makeYamlHDF(sorted, "docs.pages", data);

    if (Doclava.USE_DEVSITE_LOCALE_OUTPUT_PATHS && (Doclava.libraryRoot != null)) {
      dir = Doclava.ensureSlash(dir) + Doclava.libraryRoot;
    }

    data.setValue("docs.classes.link", Doclava.ensureSlash(dir) + "classes.html");
    data.setValue("docs.packages.link", Doclava.ensureSlash(dir) + "packages.html");

    ClearPage.write(data, "yaml_navtree.cs", Doclava.ensureSlash(dir) + fileName);
  }

  public static Data makeYamlHDF(SortedMap<String, Object> sorted, String base, Data data) {

    String key = "docs.pages.";
    int i = 0;
    for (String s : sorted.keySet()) {
      Object o = sorted.get(s);

      if (o instanceof PackageInfo) {
        PackageInfo pkg = (PackageInfo) o;

        data.setValue("docs.pages." + i + ".id", "" + i);
        data.setValue("docs.pages." + i + ".label", pkg.name());
        data.setValue("docs.pages." + i + ".shortname", "API");
        data.setValue("docs.pages." + i + ".apilevel", pkg.getSince());
        data.setValue("docs.pages." + i + ".link", pkg.htmlPage());
        data.setValue("docs.pages." + i + ".type", "package");
      } else if (o instanceof ClassInfo) {
        ClassInfo cl = (ClassInfo) o;

        // skip classes that are the child of another class, recursion will handle those.
        if (cl.containingClass() == null) {

          data.setValue("docs.pages." + i + ".id", "" + i);
          data = makeYamlHDF(cl, "docs.pages."+i, data);
        }
      }

      i++;
    }

    return data;
  }

  public static Data makeYamlHDF(ClassInfo cl, String base, Data data) {
    data.setValue(base + ".label", cl.name());
    data.setValue(base + ".shortname", cl.name().substring(cl.name().lastIndexOf(".")+1));
    data.setValue(base + ".link", cl.htmlPage());
    data.setValue(base + ".type", cl.kind());

    if (cl.innerClasses().size() > 0) {
      int j = 0;
      for (ClassInfo cl2 : cl.innerClasses()) {
        if (cl2.isHiddenOrRemoved()) {
          continue;
        }
        data = makeYamlHDF(cl2, base + ".children." + j, data);
        j++;
      }
    }

    return data;
  }

  private static Node makePackageNode(PackageInfo pkg) {
    List<Node> children = new ArrayList<Node>();

    addClassNodes(children, "Annotations", pkg.annotations());
    addClassNodes(children, "Interfaces", pkg.interfaces());
    addClassNodes(children, "Classes", pkg.ordinaryClasses());
    addClassNodes(children, "Enums", pkg.enums());
    addClassNodes(children, "Exceptions", pkg.exceptions());
    addClassNodes(children, "Errors", pkg.errors());

    return new Node(pkg.name(), pkg.htmlPage(), children, pkg.getSince());
  }

  private static void addClassNodes(List<Node> parent, String label, ClassInfo[] classes) {
    List<Node> children = new ArrayList<Node>();

    for (ClassInfo cl : classes) {
      if (cl.checkLevel()) {
        children.add(new Node(cl.name(), cl.htmlPage(), null, cl.getSince(), cl.getArtifact()));
      }
    }

    if (children.size() > 0) {
      parent.add(new Node(label, null, children, null));
    }
  }

  private static class Node {
    private String mLabel;
    private String mLink;
    List<Node> mChildren;
    private String mSince;
    private String mArtifact;

    Node(String label, String link, List<Node> children, String since) {
      this(label, link, children, since, null);
    }

    Node(String label, String link, List<Node> children, String since, String artifact) {
      mLabel = label;
      mLink = link;
      mChildren = children;
      mSince = since;
      mArtifact = artifact;
    }

    static void renderString(StringBuilder buf, String s) {
      if (s == null) {
        buf.append("null");
      } else {
        buf.append('"');
        final int N = s.length();
        for (int i = 0; i < N; i++) {
          char c = s.charAt(i);
          if (c >= ' ' && c <= '~' && c != '"' && c != '\\') {
            buf.append(c);
          } else {
            buf.append("\\u");
            for (int j = 0; i < 4; i++) {
              char x = (char) (c & 0x000f);
              if (x >= 10) {
                x = (char) (x - 10 + 'a');
              } else {
                x = (char) (x + '0');
              }
              buf.append(x);
              c >>= 4;
            }
          }
        }
        buf.append('"');
      }
    }

    void renderChildren(StringBuilder buf) {
      List<Node> list = mChildren;
      if (list == null || list.size() == 0) {
        // We output null for no children. That way empty lists here can just
        // be a byproduct of how we generate the lists.
        buf.append("null");
      } else {
        buf.append("[ ");
        final int N = list.size();
        for (int i = 0; i < N; i++) {
          list.get(i).render(buf);
          if (i != N - 1) {
            buf.append(", ");
          }
        }
        buf.append(" ]\n");
      }
    }

    void render(StringBuilder buf) {
      buf.append("[ ");
      renderString(buf, mLabel);
      buf.append(", ");
      renderString(buf, mLink);
      buf.append(", ");
      renderChildren(buf);
      buf.append(", ");
      renderString(buf, mSince);
      buf.append(", ");
      renderString(buf, mArtifact);
      buf.append(" ]");
    }


    // YAML VERSION


    static void renderStringYaml(StringBuilder buf, String s) {
      if (s != null) {
        final int N = s.length();
        for (int i = 0; i < N; i++) {
          char c = s.charAt(i);
          if (c >= ' ' && c <= '~' && c != '"' && c != '\\') {
            buf.append(c);
          } else {
            buf.append("\\u");
            for (int j = 0; i < 4; i++) {
              char x = (char) (c & 0x000f);
              if (x >= 10) {
                x = (char) (x - 10 + 'a');
              } else {
                x = (char) (x + '0');
              }
              buf.append(x);
              c >>= 4;
            }
          }
        }
      }
    }
    void renderChildrenYaml(StringBuilder buf, int depth) {
      List<Node> list = mChildren;
      if (list != null && list.size() > 0) {
        if (depth > 0) {
          buf.append("\n\n" + getIndent(depth));
          buf.append("section:");
        }
        final int N = list.size();
        for (int i = 0; i < N; i++) {
          // get each child Node and render it
          list.get(i).renderYaml(buf, depth);
        }
        // Extra line break after each "section"
        buf.append("\n");
      }
    }
    void renderYaml(StringBuilder buf, int depth) {
      buf.append("\n" + getIndent(depth));
      buf.append("- title: \"");
      renderStringYaml(buf, mLabel);
      buf.append("\"");
      // Add link path, if it exists (the class/interface toggles don't have links)
      if (mLink != null) {
        buf.append("\n" + getIndent(depth));
        buf.append("  path: ");
        renderStringYaml(buf, "/" + mLink);
        // add the API level info only if we have it
        if (mSince != null) {
          buf.append("\n" + getIndent(depth));
          buf.append("  version_added: ");
          renderStringYaml(buf, "'" + mSince + "'");
        }
      }
      // try rendering child Nodes
      renderChildrenYaml(buf, depth + 1);
    }
    String getIndent(int depth) {
      String spaces = "";
      for (int i = 0; i < depth; i++) {
        spaces += "  ";
      }
      return spaces;
    }
  }
}