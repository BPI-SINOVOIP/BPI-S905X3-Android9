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

import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.TreeSet;

public class Errors {
  public static boolean hadError = false;
  private static boolean lintsAreErrors = false;
  private static boolean warningsAreErrors = false;
  private static TreeSet<ErrorMessage> allErrors = new TreeSet<ErrorMessage>();

  public static class ErrorMessage implements Comparable<ErrorMessage> {
    final int resolvedLevel;
    final Error error;
    final SourcePositionInfo pos;
    final String msg;

    ErrorMessage(int r, Error e, SourcePositionInfo p, String m) {
      resolvedLevel = r;
      error = e;
      pos = p;
      msg = m;
    }

    @Override
    public int compareTo(ErrorMessage other) {
      int r = pos.compareTo(other.pos);
      if (r != 0) return r;
      return msg.compareTo(other.msg);
    }

    @Override
    public String toString() {
      StringBuilder res = new StringBuilder();
      if (Doclava.android) {
        res.append("\033[1m").append(pos.toString()).append(": ");
        switch (error.getLevel()) {
          case LINT: res.append("\033[36mlint: "); break;
          case WARNING: res.append("\033[33mwarning: "); break;
          case ERROR: res.append("\033[31merror: "); break;
          default: break;
        }
        res.append("\033[0m");
        res.append(msg);
        res.append(" [").append(error.code).append("]");
      } else {
        // Sigh, some people are parsing the old format.
        res.append(pos.toString()).append(": ");
        switch (error.getLevel()) {
          case LINT: res.append("lint "); break;
          case WARNING: res.append("warning "); break;
          case ERROR: res.append("error "); break;
          default: break;
        }
        res.append(error.code).append(": ");
        res.append(msg);
      }
      return res.toString();
    }
    
    public Error error() {
      return error;
    }
  }

  public static void error(Error error, MemberInfo mi, String text) {
    if (error.getLevel() == Errors.LINT) {
      final String ident = "Doclava" + error.code;
      for (AnnotationInstanceInfo a : mi.annotations()) {
        if (a.type().qualifiedNameMatches("android", "annotation.SuppressLint")) {
          for (AnnotationValueInfo val : a.elementValues()) {
            if ("value".equals(val.element().name())) {
              for (AnnotationValueInfo inner : (ArrayList<AnnotationValueInfo>) val.value()) {
                if (ident.equals(String.valueOf(inner.value()))) {
                  return;
                }
              }
            }
          }
        }
      }
    }
    error(error, mi.position(), text);
  }

  public static void error(Error error, SourcePositionInfo where, String text) {
    if (error.getLevel() == HIDDEN) {
      return;
    }

    int resolvedLevel = error.getLevel();
    if (resolvedLevel == LINT && lintsAreErrors) {
      resolvedLevel = ERROR;
    }
    if (resolvedLevel == WARNING && warningsAreErrors) {
      resolvedLevel = ERROR;
    }

    if (where == null) {
      where = new SourcePositionInfo("unknown", 0, 0);
    }

    allErrors.add(new ErrorMessage(resolvedLevel, error, where, text));

    if (resolvedLevel == ERROR) {
      hadError = true;
    }
  }
  
  public static void clearErrors() {
    hadError = false;
    allErrors.clear();
  }

  public static void printErrors() {
    printErrors(allErrors);
  }
  
  public static void printErrors(Set<ErrorMessage> errors) {
    for (ErrorMessage m : errors) {
      System.err.println(m.toString());
    }
    System.err.flush();
  }
  
  public static Set<ErrorMessage> getErrors() {
    return allErrors;
  }

  public static final int INHERIT = -1;
  public static final int HIDDEN = 0;

  /**
   * Lint level means that we encountered inconsistent or broken documentation.
   * These should be resolved, but don't impact API compatibility.
   */
  public static final int LINT = 1;

  /**
   * Warning level means that we encountered some incompatible or inconsistent
   * API change. These must be resolved to preserve API compatibility.
   */
  public static final int WARNING = 2;

  /**
   * Error level means that we encountered severe trouble and were unable to
   * output the requested documentation.
   */
  public static final int ERROR = 3;

  public static void setLintsAreErrors(boolean val) {
    lintsAreErrors = val;
  }

  public static void setWarningsAreErrors(boolean val) {
    warningsAreErrors = val;
  }

  public static class Error {
    public int code;

    /**
     * @deprecated This field should not be access directly. Instead, use
     *             {@link #getLevel()}.
     */
    @Deprecated
    public int level;

    /**
     * When {@code level} is set to {@link #INHERIT}, this is the parent from
     * which the error will inherit its level.
     */
    private final Error parent;

    public Error(int code, int level) {
      this.code = code;
      this.level = level;
      this.parent = null;
      sErrors.add(this);
    }

    public Error(int code, Error parent) {
      this.code = code;
      this.level = -1;
      this.parent = parent;
      sErrors.add(this);
    }

    /**
     * Returns the implied level for this error.
     * <p>
     * If the level is {@link #INHERIT}, the level will be returned for the
     * parent.
     *
     * @return
     * @throws IllegalStateException if the level is {@link #INHERIT} and the
     *         parent is {@code null}
     */
    public int getLevel() {
      if (level == INHERIT) {
        if (parent == null) {
          throw new IllegalStateException("Error with level INHERIT must have non-null parent");
        }
        return parent.getLevel();
      }
      return level;
    }

    /**
     * Sets the level.
     * <p>
     * Valid arguments are:
     * <ul>
     *     <li>{@link #HIDDEN}
     *     <li>{@link #WARNING}
     *     <li>{@link #ERROR}
     * </ul>
     *
     * @param level the level to set
     */
    public void setLevel(int level) {
      if (level == INHERIT) {
        throw new IllegalArgumentException("Error level may not be set to INHERIT");
      }
      this.level = level;
    }

    public String toString() {
      return "Error #" + this.code;
    }
  }

  public static final List<Error> sErrors = new ArrayList<>();

  // Errors for API verification
  public static final Error PARSE_ERROR = new Error(1, ERROR);
  public static final Error ADDED_PACKAGE = new Error(2, WARNING);
  public static final Error ADDED_CLASS = new Error(3, WARNING);
  public static final Error ADDED_METHOD = new Error(4, WARNING);
  public static final Error ADDED_FIELD = new Error(5, WARNING);
  public static final Error ADDED_INTERFACE = new Error(6, WARNING);
  public static final Error REMOVED_PACKAGE = new Error(7, WARNING);
  public static final Error REMOVED_CLASS = new Error(8, WARNING);
  public static final Error REMOVED_METHOD = new Error(9, WARNING);
  public static final Error REMOVED_FIELD = new Error(10, WARNING);
  public static final Error REMOVED_INTERFACE = new Error(11, WARNING);
  public static final Error CHANGED_STATIC = new Error(12, WARNING);
  public static final Error ADDED_FINAL = new Error(13, WARNING);
  public static final Error CHANGED_TRANSIENT = new Error(14, WARNING);
  public static final Error CHANGED_VOLATILE = new Error(15, WARNING);
  public static final Error CHANGED_TYPE = new Error(16, WARNING);
  public static final Error CHANGED_VALUE = new Error(17, WARNING);
  public static final Error CHANGED_SUPERCLASS = new Error(18, WARNING);
  public static final Error CHANGED_SCOPE = new Error(19, WARNING);
  public static final Error CHANGED_ABSTRACT = new Error(20, WARNING);
  public static final Error CHANGED_THROWS = new Error(21, WARNING);
  public static final Error CHANGED_NATIVE = new Error(22, HIDDEN);
  public static final Error CHANGED_CLASS = new Error(23, WARNING);
  public static final Error CHANGED_DEPRECATED = new Error(24, WARNING);
  public static final Error CHANGED_SYNCHRONIZED = new Error(25, WARNING);
  public static final Error ADDED_FINAL_UNINSTANTIABLE = new Error(26, WARNING);
  public static final Error REMOVED_FINAL = new Error(27, WARNING);
  public static final Error REMOVED_DEPRECATED_CLASS = new Error(28, REMOVED_CLASS);
  public static final Error REMOVED_DEPRECATED_METHOD = new Error(29, REMOVED_METHOD);
  public static final Error REMOVED_DEPRECATED_FIELD = new Error(30, REMOVED_FIELD);
  public static final Error ADDED_ABSTRACT_METHOD = new Error(31, ADDED_METHOD);

  // Errors in javadoc generation
  public static final Error UNRESOLVED_LINK = new Error(101, LINT);
  public static final Error BAD_INCLUDE_TAG = new Error(102, LINT);
  public static final Error UNKNOWN_TAG = new Error(103, LINT);
  public static final Error UNKNOWN_PARAM_TAG_NAME = new Error(104, LINT);
  public static final Error UNDOCUMENTED_PARAMETER = new Error(105, HIDDEN); // LINT
  public static final Error BAD_ATTR_TAG = new Error(106, LINT);
  public static final Error BAD_INHERITDOC = new Error(107, HIDDEN); // LINT
  public static final Error HIDDEN_LINK = new Error(108, LINT);
  public static final Error HIDDEN_CONSTRUCTOR = new Error(109, WARNING);
  public static final Error UNAVAILABLE_SYMBOL = new Error(110, WARNING);
  public static final Error HIDDEN_SUPERCLASS = new Error(111, WARNING);
  public static final Error DEPRECATED = new Error(112, HIDDEN);
  public static final Error DEPRECATION_MISMATCH = new Error(113, WARNING);
  public static final Error MISSING_COMMENT = new Error(114, LINT);
  public static final Error IO_ERROR = new Error(115, ERROR);
  public static final Error NO_SINCE_DATA = new Error(116, HIDDEN);
  public static final Error NO_FEDERATION_DATA = new Error(117, WARNING);
  public static final Error BROKEN_SINCE_FILE = new Error(118, ERROR);
  public static final Error INVALID_CONTENT_TYPE = new Error(119, ERROR);
  public static final Error INVALID_SAMPLE_INDEX = new Error(120, ERROR);
  public static final Error HIDDEN_TYPE_PARAMETER = new Error(121, WARNING);
  public static final Error PRIVATE_SUPERCLASS = new Error(122, WARNING);
  public static final Error NULLABLE = new Error(123, HIDDEN); // LINT
  public static final Error INT_DEF = new Error(124, HIDDEN); // LINT
  public static final Error REQUIRES_PERMISSION = new Error(125, LINT);
  public static final Error BROADCAST_BEHAVIOR = new Error(126, LINT);
  public static final Error SDK_CONSTANT = new Error(127, LINT);
  public static final Error TODO = new Error(128, LINT);
  public static final Error NO_ARTIFACT_DATA = new Error(129, HIDDEN);
  public static final Error BROKEN_ARTIFACT_FILE = new Error(130, ERROR);

  public static boolean setErrorLevel(int code, int level) {
    for (Error e : sErrors) {
      if (e.code == code) {
        e.setLevel(level);
        return true;
      }
    }
    return false;
  }
}
