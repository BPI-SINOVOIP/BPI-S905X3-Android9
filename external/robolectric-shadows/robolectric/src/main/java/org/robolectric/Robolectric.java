package org.robolectric;

import android.app.Activity;
import android.app.Fragment;
import android.app.IntentService;
import android.app.Service;
import android.app.backup.BackupAgent;
import android.content.ContentProvider;
import android.content.Intent;
import android.os.Bundle;
import android.util.AttributeSet;
import android.view.View;
import java.util.ServiceLoader;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import org.robolectric.android.XmlResourceParserImpl;
import org.robolectric.android.controller.ActivityController;
import org.robolectric.android.controller.BackupAgentController;
import org.robolectric.android.controller.ContentProviderController;
import org.robolectric.android.controller.FragmentController;
import org.robolectric.android.controller.IntentServiceController;
import org.robolectric.android.controller.ServiceController;
import org.robolectric.internal.ShadowProvider;
import org.robolectric.res.ResName;
import org.robolectric.res.ResourceTable;
import org.robolectric.shadows.ShadowApplication;
import org.robolectric.util.ReflectionHelpers;
import org.robolectric.util.Scheduler;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

public class Robolectric {

  /**
   * This method is internal and shouldn't be called by developers.
   */
  @Deprecated
  public static void reset() {
    // No-op- is now handled in the test runner. Users should not be calling this method anyway.
  }

  public static <T extends Service> ServiceController<T> buildService(Class<T> serviceClass) {
    return buildService(serviceClass, null);
  }

  public static <T extends Service> ServiceController<T> buildService(Class<T> serviceClass, Intent intent) {
    return ServiceController.of(ReflectionHelpers.callConstructor(serviceClass), intent);
  }

  public static <T extends Service> T setupService(Class<T> serviceClass) {
    return buildService(serviceClass).create().get();
  }

  public static <T extends IntentService> IntentServiceController<T> buildIntentService(Class<T> serviceClass) {
    return buildIntentService(serviceClass, null);
  }

  public static <T extends IntentService> IntentServiceController<T> buildIntentService(Class<T> serviceClass, Intent intent) {
    return IntentServiceController.of(ReflectionHelpers.callConstructor(serviceClass, new ReflectionHelpers.ClassParameter<String>(String.class, "IntentService")), intent);
  }

  public static <T extends IntentService> T setupIntentService(Class<T> serviceClass) {
    return buildIntentService(serviceClass).create().get();
  }

  public static <T extends ContentProvider> ContentProviderController<T> buildContentProvider(Class<T> contentProviderClass) {
    return ContentProviderController.of(ReflectionHelpers.callConstructor(contentProviderClass));
  }

  public static <T extends ContentProvider> T setupContentProvider(Class<T> contentProviderClass) {
    return buildContentProvider(contentProviderClass).create().get();
  }

  public static <T extends ContentProvider> T setupContentProvider(Class<T> contentProviderClass, String authority) {
    return buildContentProvider(contentProviderClass).create(authority).get();
  }

  public static <T extends Activity> ActivityController<T> buildActivity(Class<T> activityClass) {
    return buildActivity(activityClass, null);
  }

  public static <T extends Activity> ActivityController<T> buildActivity(Class<T> activityClass, Intent intent) {
    return ActivityController.of(ReflectionHelpers.callConstructor(activityClass), intent);
  }

  public static <T extends Activity> T setupActivity(Class<T> activityClass) {
    return buildActivity(activityClass).setup().get();
  }

  public static <T extends Fragment> FragmentController<T> buildFragment(Class<T> fragmentClass) {
    return FragmentController.of(ReflectionHelpers.callConstructor(fragmentClass));
  }

  public static <T extends Fragment> FragmentController<T> buildFragment(Class<T> fragmentClass,
                                                                         Bundle arguments) {
    return FragmentController.of(ReflectionHelpers.callConstructor(fragmentClass), arguments);
  }

  public static <T extends Fragment> FragmentController<T> buildFragment(Class<T> fragmentClass,
                                                                         Class<? extends Activity> activityClass) {
    return FragmentController.of(ReflectionHelpers.callConstructor(fragmentClass), activityClass);
  }

  public static <T extends Fragment> FragmentController<T> buildFragment(Class<T> fragmentClass, Intent intent) {
    return FragmentController.of(ReflectionHelpers.callConstructor(fragmentClass), intent);
  }

  public static <T extends Fragment> FragmentController<T> buildFragment(Class<T> fragmentClass,
                                                                         Intent intent,
                                                                         Bundle arguments) {
    return FragmentController.of(ReflectionHelpers.callConstructor(fragmentClass), intent, arguments);
  }

  public static <T extends Fragment> FragmentController<T> buildFragment(Class<T> fragmentClass,
                                                                         Class<? extends Activity> activityClass,
                                                                         Intent intent) {
    return FragmentController.of(ReflectionHelpers.callConstructor(fragmentClass), activityClass, intent);
  }

  public static <T extends Fragment> FragmentController<T> buildFragment(Class<T> fragmentClass,
                                                                         Class<? extends Activity> activityClass,
                                                                         Bundle arguments) {
    return FragmentController.of(ReflectionHelpers.callConstructor(fragmentClass), activityClass, arguments);
  }

  public static <T extends Fragment> FragmentController<T> buildFragment(Class<T> fragmentClass,
                                                                         Class<? extends Activity> activityClass,
                                                                         Intent intent,
                                                                         Bundle arguments) {
    return FragmentController.of(ReflectionHelpers.callConstructor(fragmentClass), activityClass, intent, arguments);
  }

  public static <T extends BackupAgent> BackupAgentController<T> buildBackupAgent(Class<T> backupAgentClass) {
    return BackupAgentController.of(ReflectionHelpers.callConstructor(backupAgentClass));
  }

  public static <T extends BackupAgent> T setupBackupAgent(Class<T> backupAgentClass) {
    return buildBackupAgent(backupAgentClass).create().get();
  }

  /**
   * Allows for the programatic creation of an {@link AttributeSet} useful for testing {@link View} classes without
   * the need for creating XML snippets.
   */
  public static AttributeSetBuilder buildAttributeSet() {
    DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
    factory.setNamespaceAware(true);
    factory.setIgnoringComments(true);
    factory.setIgnoringElementContentWhitespace(true);
    Document document;
    try {
      DocumentBuilder documentBuilder = factory.newDocumentBuilder();
      document = documentBuilder.newDocument();
      Element dummy = document.createElementNS("http://schemas.android.com/apk/res/" + RuntimeEnvironment.application.getPackageName(), "dummy");
      document.appendChild(dummy);
    } catch (ParserConfigurationException e) {
      throw new RuntimeException(e);
    }
    return new AttributeSetBuilder(document, RuntimeEnvironment.getCompileTimeResourceTable());
  }

  public static class AttributeSetBuilder {

    private Document doc;
    private ResourceTable appResourceTable;

    AttributeSetBuilder(Document doc, ResourceTable resourceTable) {
      this.doc = doc;
      this.appResourceTable = resourceTable;
    }

    public AttributeSetBuilder addAttribute(int resId, String value) {
      ResName resName = appResourceTable.getResName(resId);
      if ("style".equals(resName.name)) {
        ((Element)doc.getFirstChild()).setAttribute(resName.name, value);
      } else {
        ((Element)doc.getFirstChild()).setAttributeNS(resName.getNamespaceUri(), resName.packageName + ":" + resName.name, value);
      }
      return this;
    }

    public AttributeSetBuilder setStyleAttribute(String value) {
      ((Element)doc.getFirstChild()).setAttribute("style", value);
      return this;
    }

    public AttributeSet build() {
      XmlResourceParserImpl parser = new XmlResourceParserImpl(doc, null, RuntimeEnvironment.application.getPackageName(), RuntimeEnvironment.application.getPackageName(), appResourceTable);
      try {
        parser.next(); // Root document element
        parser.next(); // "dummy" element
      } catch (Exception e) {
        throw new IllegalStateException("Expected single dummy element in the document to contain the attributes.", e);
      }

      return parser;
    }
  }

  /**
   * Return the foreground scheduler (e.g. the UI thread scheduler).
   *
   * @return  Foreground scheduler.
   */
  public static Scheduler getForegroundThreadScheduler() {
    return ShadowApplication.getInstance().getForegroundThreadScheduler();
  }

  /**
   * Execute all runnables that have been enqueued on the foreground scheduler.
   */
  public static void flushForegroundThreadScheduler() {
    getForegroundThreadScheduler().advanceToLastPostedRunnable();
  }

  /**
   * Return the background scheduler.
   *
   * @return  Background scheduler.
   */
  public static Scheduler getBackgroundThreadScheduler() {
    return ShadowApplication.getInstance().getBackgroundThreadScheduler();
  }

  /**
   * Execute all runnables that have been enqueued on the background scheduler.
   */
  public static void flushBackgroundThreadScheduler() {
    getBackgroundThreadScheduler().advanceToLastPostedRunnable();
  }
}
