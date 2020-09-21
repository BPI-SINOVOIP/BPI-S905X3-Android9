package org.robolectric.shadows;

import static android.os.Build.VERSION_CODES.KITKAT_WATCH;
import static android.os.Build.VERSION_CODES.LOLLIPOP;
import static org.robolectric.RuntimeEnvironment.getApiLevel;
import static org.robolectric.Shadows.shadowOf;
import static org.robolectric.shadow.api.Shadow.directlyOn;
import static org.robolectric.util.ReflectionHelpers.getField;
import static org.robolectric.util.ReflectionHelpers.getStaticField;
import static org.robolectric.util.ReflectionHelpers.setField;
import static org.robolectric.util.ReflectionHelpers.setStaticField;

import android.os.Handler;
import android.os.Message;
import org.robolectric.annotation.HiddenApi;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.annotation.RealObject;
import org.robolectric.annotation.Resetter;

@Implements(Message.class)
public class ShadowMessage {
  @RealObject
  private Message realMessage;
  private Runnable scheduledRunnable;

  private static final Object lock = getStaticField(Message.class, "sPoolSync");

  private void unschedule() {
    Handler target = realMessage.getTarget();

	if (target != null && scheduledRunnable != null) {
	  shadowOf(target.getLooper()).getScheduler().remove(scheduledRunnable);
	  scheduledRunnable = null;
	}
  }

  /**
   * Hook to unscheduled the callback when the message is recycled.
   * Invokes {@link #unschedule()} and then calls through to the
   * package private method {@link Message#recycleUnchecked()}
   * on the real object.
   */
  @Implementation
  @HiddenApi
  public void recycleUnchecked() {
    if (getApiLevel() >= LOLLIPOP) {
      unschedule();
      directlyOn(realMessage, Message.class, "recycleUnchecked");
    } else {
      // provide forward compatibility with SDK 21.
      recycle();
    }
  }

  /**
   * Hook to unscheduled the callback when the message is recycled.
   * Invokes {@link #unschedule()} and then calls through to
   * {@link Message#recycle()} on the real object.
   */
  @Implementation(maxSdk = KITKAT_WATCH)
  public void recycle() {
    unschedule();
    directlyOn(realMessage, Message.class, "recycle");
  }

  /**
   * Stores the {@link Runnable} instance that has been scheduled
   * to invoke this message. This is called when the message is
   * enqueued by {@link ShadowMessageQueue#enqueueMessage} and is used when
   * the message is recycled to ensure that the correct
   * {@link Runnable} instance is removed from the associated scheduler.
   *
   * @param r the {@link Runnable} instance that is scheduled to
   * trigger this message.
   *
#if ($api >= 21)   * @see #recycleUnchecked()
#else   * @see #recycle()
#end
   */
  public void setScheduledRunnable(Runnable r) {
    scheduledRunnable = r;
  }

  /**
   * Convenience method to provide access to the private {@code Message.isInUse()}
   * method. Note that the definition of "in use" changed with API 21:
   *
   * In API 19, a message was only considered "in use" during its dispatch. In API 21, the
   * message is considered "in use" from the time it is enqueued until the time that
   * it is freshly obtained via a call to {@link Message#obtain()}. This means that
   * in API 21 messages that are in the recycled pool will still be marked as "in use". 
   *
   * @return {@code true} if the message is currently "in use", {@code false} otherwise.
   */
  @Implementation
  public boolean isInUse() {
  	return directlyOn(realMessage, Message.class, "isInUse");
  }

  /**
   * Convenience method to provide getter access to the private field
   * {@code Message.next}.
   *
   * @return The next message in the current message chain.
   * @see #setNext(Message) 
   */
  public Message getNext() {
    return getField(realMessage, "next");    
  }
  
  /**
   * Convenience method to provide setter access to the private field
   * {@code Message.next}.
   *
   * @param next the new next message for the current message.
   * @see #getNext() 
   */
  public void setNext(Message next) {
    setField(realMessage, "next", next);
  }
  
  /**
   * Resets the static state of the {@link Message} class by
   * emptying the message pool.
   */
  @Resetter
  public static void reset() {
    synchronized (lock) {
      setStaticField(Message.class, "sPoolSize", 0);
      setStaticField(Message.class, "sPool", null);
    }
  }
}
