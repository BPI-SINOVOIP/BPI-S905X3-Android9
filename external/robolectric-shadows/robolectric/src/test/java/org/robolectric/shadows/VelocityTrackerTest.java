package org.robolectric.shadows;

import static org.assertj.core.api.Assertions.assertThat;

import android.view.MotionEvent;
import android.view.VelocityTracker;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.Shadows;

@RunWith(RobolectricTestRunner.class)
public class VelocityTrackerTest {
  VelocityTracker velocityTracker;

  @Before
  public void setUp() {
    velocityTracker = VelocityTracker.obtain();
  }

  @Test
  public void handlesXMovement() {
    velocityTracker.addMovement(doMotion(0, 0, 0));
    velocityTracker.addMovement(doMotion(20, 20, 0));
    velocityTracker.computeCurrentVelocity(1);

    // active pointer
    assertThat(velocityTracker.getXVelocity()).isEqualTo(1);
    assertThat(velocityTracker.getXVelocity(0)).isEqualTo(1);
    // inactive pointer
    assertThat(velocityTracker.getXVelocity(10)).isEqualTo(0);
  }

  @Test
  public void handlesYMovement() {
    velocityTracker.addMovement(doMotion(0, 0, 0));
    velocityTracker.addMovement(doMotion(20, 0, 20));
    velocityTracker.computeCurrentVelocity(1);

    // active pointer
    assertThat(velocityTracker.getYVelocity()).isEqualTo(1);
    assertThat(velocityTracker.getYVelocity(0)).isEqualTo(1);
    // inactive pointer
    assertThat(velocityTracker.getYVelocity(10)).isEqualTo(0);
  }

  @Test
  public void handlesXAndYMovement() {
    velocityTracker.addMovement(doMotion(0, 0, 0));
    velocityTracker.addMovement(doMotion(20, 20, 40));
    velocityTracker.computeCurrentVelocity(1);

    assertThat(velocityTracker.getXVelocity()).isEqualTo(1);
    assertThat(velocityTracker.getYVelocity()).isEqualTo(2);
  }

  @Test
  public void handlesWindowing_positive() {
    velocityTracker.addMovement(doMotion(0, 0, 0));
    velocityTracker.addMovement(doMotion(20, 10000, 10000));
    velocityTracker.computeCurrentVelocity(1, 10);

    assertThat(velocityTracker.getXVelocity()).isEqualTo(10);
    assertThat(velocityTracker.getYVelocity()).isEqualTo(10);
  }

  @Test
  public void handlesWindowing_negative() {
    velocityTracker.addMovement(doMotion(0, 0, 0));
    velocityTracker.addMovement(doMotion(20, -10000, -10000));
    velocityTracker.computeCurrentVelocity(1, 10);

    assertThat(velocityTracker.getXVelocity()).isEqualTo(-10);
    assertThat(velocityTracker.getYVelocity()).isEqualTo(-10);
  }

  @Test
  public void handlesMultiplePointers() {
    // pointer 0 active
    velocityTracker.addMovement(doMotion(0, 0, 0));
    // pointer 1 active
    velocityTracker.addMovement(doMotion(20, 40, 40, 0, 0));
    velocityTracker.addMovement(doMotion(40, 80, 80, 20, 20));
    velocityTracker.computeCurrentVelocity(1);

    // active pointer
    assertThat(velocityTracker.getXVelocity()).isEqualTo(1);
    assertThat(velocityTracker.getXVelocity(1)).isEqualTo(1);
    // inactive pointer
    assertThat(velocityTracker.getXVelocity(0)).isEqualTo(2f);
  }

  @Test
  public void handlesClearing() {
    velocityTracker.addMovement(doMotion(0, 0, 0));
    velocityTracker.addMovement(doMotion(20, 20, 20));
    velocityTracker.computeCurrentVelocity(1);
    velocityTracker.clear();

    assertThat(velocityTracker.getXVelocity()).isEqualTo(0);
    assertThat(velocityTracker.getYVelocity()).isEqualTo(0);
    velocityTracker.computeCurrentVelocity(1);
    assertThat(velocityTracker.getXVelocity()).isEqualTo(0);
    assertThat(velocityTracker.getYVelocity()).isEqualTo(0);
  }

  @Test
  public void clearsOnDown() {
    velocityTracker.addMovement(doMotion(0, 0, 0));
    velocityTracker.addMovement(doMotion(20, 20, 20));
    velocityTracker.computeCurrentVelocity(1);
    velocityTracker.addMovement(doPointerDown(40, 40, 40));
    velocityTracker.computeCurrentVelocity(1);

    assertThat(velocityTracker.getXVelocity()).isEqualTo(0);
    assertThat(velocityTracker.getYVelocity()).isEqualTo(0);
  }

  private static MotionEvent doMotion(long time, float x, float y) {
    return MotionEvent.obtain(0, time, MotionEvent.ACTION_MOVE, x, y, 0);
  }

  private static MotionEvent doPointerDown(long time, float x, float y) {
    return MotionEvent.obtain(time, time, MotionEvent.ACTION_DOWN, x, y, 0);
  }

  /**
   * Construct a new MotionEvent involving two pointers at {@code time}. Pointer 2 will be
   * considered active.
   */
  private static MotionEvent doMotion(
      long time, float pointer1X, float pointer1Y, float pointer2X, float pointer2Y) {
    MotionEvent event =
        MotionEvent.obtain(0, time, MotionEvent.ACTION_MOVE, pointer2X, pointer2Y, 0);
    ShadowMotionEvent shadowEvent = Shadows.shadowOf(event);
    shadowEvent.setPointer2(pointer1X, pointer1Y);
    shadowEvent.setPointerIndex(0);
    // we put our active pointer (the second one down) first, so flip the IDs so that they match up
    // properly
    shadowEvent.setPointerIds(1, 0);

    return event;
  }
}
