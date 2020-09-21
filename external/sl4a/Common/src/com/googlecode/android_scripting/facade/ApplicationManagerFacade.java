/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.googlecode.android_scripting.facade;

import android.app.ActivityManager;
import android.app.ActivityManager.RunningAppProcessInfo;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;

import com.googlecode.android_scripting.Log;
import com.googlecode.android_scripting.jsonrpc.RpcReceiver;
import com.googlecode.android_scripting.rpc.Rpc;
import com.googlecode.android_scripting.rpc.RpcParameter;
import com.googlecode.android_scripting.rpc.RpcOptional;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.json.JSONException;
import org.json.JSONObject;

/**
 * Facade for managing Applications.
 *
 */
public class ApplicationManagerFacade extends RpcReceiver {

  private final Service mService;
  private final AndroidFacade mAndroidFacade;
  private final ActivityManager mActivityManager;
  private final PackageManager mPackageManager;

  public ApplicationManagerFacade(FacadeManager manager) {
    super(manager);
    mService = manager.getService();
    mAndroidFacade = manager.getReceiver(AndroidFacade.class);
    mActivityManager = (ActivityManager) mService.getSystemService(Context.ACTIVITY_SERVICE);
    mPackageManager = mService.getPackageManager();
  }

  @Rpc(description = "Returns a list of all launchable application class names.")
  public Map<String, String> getLaunchableApplications() {
    Intent intent = new Intent(Intent.ACTION_MAIN);
    intent.addCategory(Intent.CATEGORY_LAUNCHER);
    List<ResolveInfo> resolveInfos = mPackageManager.queryIntentActivities(intent, 0);
    Map<String, String> applications = new HashMap<String, String>();
    for (ResolveInfo info : resolveInfos) {
      applications.put(info.loadLabel(mPackageManager).toString(), info.activityInfo.name);
    }
    return applications;
  }

  @Rpc(description = "Start activity with the given class name.")
  public void launch(@RpcParameter(name = "className") String className) {
    Intent intent = new Intent(Intent.ACTION_MAIN);
    String packageName = className.substring(0, className.lastIndexOf("."));
    intent.setClassName(packageName, className);
    mAndroidFacade.startActivity(intent);
  }

  @Rpc(description = "Start activity with the given class name with result")
  public Intent launchForResult(@RpcParameter(name = "className") String className) {
    Intent intent = new Intent(Intent.ACTION_MAIN);
    String packageName = className.substring(0, className.lastIndexOf("."));
    intent.setClassName(packageName, className);
    return mAndroidFacade.startActivityForResult(intent);
  }

  @Rpc(description = "Launch the specified app.")
  public void appLaunch(@RpcParameter(name = "name") String name) {
      Intent LaunchIntent = mPackageManager.getLaunchIntentForPackage(name);
      mService.startActivity(LaunchIntent);
  }

  @Rpc(description = "Launch activity for result with intent")
  public Intent launchForResultWithIntent(
  @RpcParameter(name = "intent") Intent intent,
          @RpcParameter(name = "extras")@RpcOptional JSONObject extras)
          throws JSONException {
      if(extras != null) {
          mAndroidFacade.putExtrasFromJsonObject(extras, intent);
      }
      return mAndroidFacade.startActivityForResult(intent);
  }

  @Rpc(description = "Create intent given the class name")
  public Intent createIntentForClassName(
          @RpcParameter(name = "className") String className) {
      Intent intent = new Intent(Intent.ACTION_MAIN);
      String packageName = className.substring(0, className.lastIndexOf("."));
      intent.setClassName(packageName, className);
      return intent;
  }

  @Rpc(description = "Get UID of a package")
  public int getUidForPackage(
          @RpcParameter(name = "className") String className) {
      String packageName = className.substring(0, className.lastIndexOf("."));
      try {
          return mPackageManager.getApplicationInfo(packageName, 0).uid;
      } catch (PackageManager.NameNotFoundException e) {
          Log.e("Package not found", e);
      }
      return 0;
  }

  @Rpc(description = "Kill the specified app.")
  public Boolean appKill(@RpcParameter(name = "name") String name) {
      for (RunningAppProcessInfo info : mActivityManager.getRunningAppProcesses()) {
          if (info.processName.contains(name)) {
              Log.d("Killing " + info.processName);
              android.os.Process.killProcess(info.pid);
              android.os.Process.sendSignal(info.pid, android.os.Process.SIGNAL_KILL);
              mActivityManager.killBackgroundProcesses(info.processName);
              return true;
          }
      }
      return false;
  }

  @Rpc(description = "Returns a list of packages running activities or services.",
          returns = "List of packages running activities.")
  public List<String> getRunningPackages() {
    Set<String> runningPackages = new HashSet<String>();
    List<ActivityManager.RunningAppProcessInfo> appProcesses =
        mActivityManager.getRunningAppProcesses();
    for (ActivityManager.RunningAppProcessInfo info : appProcesses) {
      runningPackages.addAll(Arrays.asList(info.pkgList));
    }
    List<ActivityManager.RunningServiceInfo> serviceProcesses =
        mActivityManager.getRunningServices(Integer.MAX_VALUE);
    for (ActivityManager.RunningServiceInfo info : serviceProcesses) {
      runningPackages.add(info.service.getPackageName());
    }
    return new ArrayList<String>(runningPackages);
  }

  /**
   * Force stops a package. Equivalent to calling `am force-stop "package.name"` as root.
   * <p>
   * If you have access to adb, it is preferred to use the above command instead.
   *
   * @param packageName the name of the package to force stop
   */
  @Rpc(description = "Force stops a package. Equivalent to `adb shell am force-stop "
          + "\"package.name\"`. If possible, use that command instead.")
  public void forceStopPackage(
      @RpcParameter(name = "packageName", description = "name of package") String packageName) {
    mActivityManager.forceStopPackage(packageName);
  }

  @Override
  public void shutdown() {
  }
}
