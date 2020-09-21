package org.robolectric.android.controller;

import static org.robolectric.util.ReflectionHelpers.ClassParameter.from;

import android.app.ActivityThread;
import android.app.Application;
import android.app.IntentService;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.util.ReflectionHelpers;

public class IntentServiceController<T extends IntentService> extends ComponentController<IntentServiceController<T>, T> {

  public static <T extends IntentService> IntentServiceController<T> of(final T service, final Intent intent) {
    final IntentServiceController<T> controller = new IntentServiceController<>(service, intent);
    controller.attach();
    return controller;
  }

  private IntentServiceController(final T service, final Intent intent) {
    super(service, intent);
  }

  private IntentServiceController<T> attach() {
    if (attached) {
      return this;
    }

    ReflectionHelpers.callInstanceMethod(Service.class, component, "attach",
       from(Context.class, RuntimeEnvironment.application.getBaseContext()),
       from(ActivityThread.class, null),
       from(String.class, component.getClass().getSimpleName()),
       from(IBinder.class, null),
       from(Application.class, RuntimeEnvironment.application),
       from(Object.class, null));

    attached = true;
    return this;
  }

  public IntentServiceController<T> bind() {
    invokeWhilePaused("onBind", from(Intent.class, getIntent()));
    return this;
  }

  @Override public IntentServiceController<T> create() {
    invokeWhilePaused("onCreate");
    return this;
  }

  @Override public IntentServiceController<T> destroy() {
    invokeWhilePaused("onDestroy");
    return this;
  }

  public IntentServiceController<T> rebind() {
    invokeWhilePaused("onRebind", from(Intent.class, getIntent()));
    return this;
  }

  public IntentServiceController<T> startCommand(final int flags, final int startId) {
    final IntentServiceController<T> intentServiceController = handleIntent();
    get().stopSelf(startId);
    return intentServiceController;
  }

  public IntentServiceController<T> unbind() {
    invokeWhilePaused("onUnbind", from(Intent.class, getIntent()));
    return this;
  }

  public IntentServiceController<T> handleIntent() {
    invokeWhilePaused("onHandleIntent", from(Intent.class, getIntent()));
    return this;
  }

  /**
   * @deprecated Use the appropriate builder in {@link org.robolectric.Robolectric} instead.
   *
   * This method will be removed in Robolectric 3.6.
   */
  @Deprecated
  public IntentServiceController<T> withIntent(Intent intent) {
    this.intent = intent;
    return this;
  }
}
