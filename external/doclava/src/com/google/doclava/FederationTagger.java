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

import com.google.doclava.apicheck.ApiParseException;
import java.net.URL;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Cross-references documentation among different libraries. A FederationTagger
 * is populated with a list of {@link FederatedSite} objects which are linked
 * against when overlapping content is discovered.
 */
public final class FederationTagger {
  private final Map<String, URL> federatedUrls = new HashMap<>();
  private final Map<String, String> federatedXmls = new HashMap<>();
  private final List<FederatedSite> federatedSites = new ArrayList<>();

  private boolean initialized = false;

  /**
   * Adds a Doclava documentation site for federation. Accepts the base URL of
   * the remote API.
   * <p>
   * If {@link #addSiteApi(String, String)} is not called, this will default to
   * reading the API from "/xml/current.xml" within the site's base URL.
   * <p>
   * <strong>Note:</strong> Must be called before calling tag() or get() methods.
   *
   * @param name internally-used name for federation site
   */
  public void addSiteUrl(String name, URL site) {
    if (initialized) {
      throw new IllegalStateException("Cannot add sites after calling tag() or get() methods.");
    }
    federatedUrls.put(name, site);
  }

  /**
   * Adds an explicit Doclava-generated API file for the specified site.
   * <p>
   * <strong>Note:</strong> Must be called before calling tag() or get() methods.
   *
   * @param name internally-used name for federation site (must match name used
   *             for {@link #addSiteUrl(String, URL)})
   * @param file path to a Doclava-generated API file
   */
  public void addSiteApi(String name, String file) {
    if (initialized) {
      throw new IllegalStateException("Cannot add sites after calling tag() or get() methods.");
    }
    federatedXmls.put(name, file);
  }

  public void tag(ClassInfo classDoc) {
    initialize();
    for (FederatedSite site : federatedSites) {
      applyFederation(site, Arrays.asList(classDoc));
    }
  }

  public void tagAll(Collection<ClassInfo> classDocs) {
    initialize();
    for (FederatedSite site : federatedSites) {
      applyFederation(site, classDocs);
    }
  }

  /**
   * Returns a non-{@code null} list of {@link FederatedSite} objects, one for
   * each unique {@code name} added using {@link #addSiteUrl(String, URL)}.
   */
  public List<FederatedSite> getSites() {
    initialize();
    return federatedSites;
  }

  private void initialize() {
    if (initialized) {
      return;
    }

    for (String name : federatedXmls.keySet()) {
      if (!federatedUrls.containsKey(name)) {
        Errors.error(Errors.NO_FEDERATION_DATA, (SourcePositionInfo) null,
            "Unknown documentation site for " + name);
      }
    }

    for (String name : federatedUrls.keySet()) {
      try {
        if (federatedXmls.containsKey(name)) {
          federatedSites.add(new FederatedSite(name, federatedUrls.get(name),
              federatedXmls.get(name)));
        } else {
          federatedSites.add(new FederatedSite(name, federatedUrls.get(name)));
        }
      } catch (ApiParseException e) {
        String error = "Could not add site for federation: " + name;
        if (e.getMessage() != null) {
          error += ": " + e.getMessage();
        }
        Errors.error(Errors.NO_FEDERATION_DATA, (SourcePositionInfo) null, error);
      }
    }

    initialized = true;
  }

  private void applyFederation(FederatedSite federationSource, Collection<ClassInfo> classDocs) {
    for (ClassInfo classDoc : classDocs) {
      PackageInfo packageSpec
          = federationSource.apiInfo().getPackages().get(classDoc.containingPackage().name());

      if (packageSpec == null) {
        continue;
      }

      ClassInfo classSpec = packageSpec.allClasses().get(classDoc.name());

      if (classSpec == null) {
        continue;
      }

      federateMethods(federationSource, classSpec, classDoc);
      federateConstructors(federationSource, classSpec, classDoc);
      federateFields(federationSource, classSpec, classDoc);
      federateClass(federationSource, classDoc);
      federatePackage(federationSource, classDoc.containingPackage());
    }
  }

  private void federateMethods(FederatedSite site, ClassInfo federatedClass, ClassInfo localClass) {
    for (MethodInfo method : localClass.methods()) {
      for (ClassInfo superclass : federatedClass.hierarchy()) {
        if (superclass.allMethods().containsKey(method.getHashableName())) {
          method.addFederatedReference(site);
          break;
        }
      }
    }
  }

  private void federateConstructors(FederatedSite site, ClassInfo federatedClass,
      ClassInfo localClass) {
    for (MethodInfo constructor : localClass.constructors()) {
      if (federatedClass.hasConstructor(constructor)) {
        constructor.addFederatedReference(site);
      }
    }
  }

  private void federateFields(FederatedSite site, ClassInfo federatedClass, ClassInfo localClass) {
    for (FieldInfo field : localClass.fields()) {
      if (federatedClass.allFields().containsKey(field.name())) {
        field.addFederatedReference(site);
      }
    }
  }

  private void federateClass(FederatedSite source, ClassInfo doc) {
    doc.addFederatedReference(source);
  }

  private void federatePackage(FederatedSite source, PackageInfo pkg) {
    pkg.addFederatedReference(source);
  }
}
