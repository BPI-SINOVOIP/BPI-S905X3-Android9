package js.kbars;

import android.content.Context;
import android.util.Log;
import android.view.MotionEvent;

public class SystemGestureDebugger {
    private static final boolean DEBUG = false;
    private static final int MAX_TRACKED_POINTERS = 32;
    private static final int SWIPE_FROM_BOTTOM = 2;
    private static final int SWIPE_FROM_RIGHT = 3;
    private static final int SWIPE_FROM_TOP = 1;
    private static final int SWIPE_NONE = 0;
    private static final long SWIPE_TIMEOUT_MS = 500;
    private static final String TAG = Util.logTag(SystemGestureDebugger.class);
    private static final int UNTRACKED_POINTER = -1;
    private final Callbacks mCallbacks;
    private final float[] mCurrentX = new float[MAX_TRACKED_POINTERS];
    private final float[] mCurrentY = new float[MAX_TRACKED_POINTERS];
    private boolean mDebugFireable;
    private final int[] mDownPointerId = new int[MAX_TRACKED_POINTERS];
    private int mDownPointers;
    private final long[] mDownTime = new long[MAX_TRACKED_POINTERS];
    private final float[] mDownX = new float[MAX_TRACKED_POINTERS];
    private final float[] mDownY = new float[MAX_TRACKED_POINTERS];
    private final long[] mElapsedThreshold = new long[MAX_TRACKED_POINTERS];
    private final int mSwipeDistanceThreshold;
    private boolean mSwipeFireable;
    private final int mSwipeStartThreshold;
    int screenHeight;
    int screenWidth;

    interface Callbacks {
        void onDebug();

        void onSwipeFromBottom();

        void onSwipeFromRight();

        void onSwipeFromTop();
    }

    public static class DownPointerInfo {
        public float currentX;
        public float currentY;
        public int downPointerId;
        public long downTime;
        public float downX;
        public float downY;
        public long elapsedThreshold;
    }

    public static class ThresholdInfo {
        public int distance;
        public long elapsed;
        public int start;
    }

    public SystemGestureDebugger(Context context, Callbacks callbacks) {
        this.mCallbacks = (Callbacks) checkNull("callbacks", callbacks);
        this.mSwipeStartThreshold = ((Context) checkNull("context", context)).getResources().getDimensionPixelSize(((Integer) Util.getField("com.android.internal.R$dimen", "status_bar_height")).intValue());
        this.mSwipeDistanceThreshold = this.mSwipeStartThreshold;
        Log.d(TAG, String.format("startThreshold=%s endThreshold=%s", new Object[]{Integer.valueOf(this.mSwipeStartThreshold), Integer.valueOf(this.mSwipeDistanceThreshold)}));
    }

    private static <T> T checkNull(String name, T arg) {
        if (arg != null) {
            return arg;
        }
        throw new IllegalArgumentException(new StringBuilder(String.valueOf(name)).append(" must not be null").toString());
    }

    public void onPointerEvent(MotionEvent event) {
        boolean z = true;
        boolean z2 = DEBUG;
        switch (event.getActionMasked()) {
            case SWIPE_NONE /*0*/:
                this.mSwipeFireable = true;
                this.mDebugFireable = true;
                this.mDownPointers = SWIPE_NONE;
                captureDown(event, SWIPE_NONE);
                return;
            case 1:
            case SWIPE_FROM_RIGHT /*3*/:
                this.mSwipeFireable = DEBUG;
                this.mDebugFireable = DEBUG;
                return;
            case 2:
                if (this.mSwipeFireable) {
                    int swipe = detectSwipe(event);
                    if (swipe == 0) {
                        z2 = true;
                    }
                    this.mSwipeFireable = z2;
                    if (swipe == 1) {
                        this.mCallbacks.onSwipeFromTop();
                        return;
                    } else if (swipe == 2) {
                        this.mCallbacks.onSwipeFromBottom();
                        return;
                    } else if (swipe == SWIPE_FROM_RIGHT) {
                        this.mCallbacks.onSwipeFromRight();
                        return;
                    } else {
                        return;
                    }
                }
                return;
            case 5:
                captureDown(event, event.getActionIndex());
                if (this.mDebugFireable) {
                    if (event.getPointerCount() >= 5) {
                        z = DEBUG;
                    }
                    this.mDebugFireable = z;
                    if (!this.mDebugFireable) {
                        this.mCallbacks.onDebug();
                        return;
                    }
                    return;
                }
                return;
            default:
                return;
        }
    }

    private void captureDown(MotionEvent event, int pointerIndex) {
        int i = findIndex(event.getPointerId(pointerIndex));
        if (i != UNTRACKED_POINTER) {
            this.mDownX[i] = event.getX(pointerIndex);
            this.mDownY[i] = event.getY(pointerIndex);
            this.mDownTime[i] = event.getEventTime();
        }
    }

    private int findIndex(int pointerId) {
        for (int i = SWIPE_NONE; i < this.mDownPointers; i++) {
            if (this.mDownPointerId[i] == pointerId) {
                return i;
            }
        }
        if (this.mDownPointers == MAX_TRACKED_POINTERS || pointerId == UNTRACKED_POINTER) {
            return UNTRACKED_POINTER;
        }
        int[] iArr = this.mDownPointerId;
        int i2 = this.mDownPointers;
        this.mDownPointers = i2 + 1;
        iArr[i2] = pointerId;
        return this.mDownPointers + UNTRACKED_POINTER;
    }

    private int detectSwipe(MotionEvent move) {
        int historySize = move.getHistorySize();
        int pointerCount = move.getPointerCount();
        for (int p = SWIPE_NONE; p < pointerCount; p++) {
            int i = findIndex(move.getPointerId(p));
            if (i != UNTRACKED_POINTER) {
                int swipe;
                for (int h = SWIPE_NONE; h < historySize; h++) {
                    swipe = detectSwipe(i, move.getHistoricalEventTime(h), move.getHistoricalX(p, h), move.getHistoricalY(p, h));
                    if (swipe != 0) {
                        return swipe;
                    }
                }
                swipe = detectSwipe(i, move.getEventTime(), move.getX(p), move.getY(p));
                if (swipe != 0) {
                    return swipe;
                }
            }
        }
        return SWIPE_NONE;
    }

    private int detectSwipe(int i, long time, float x, float y) {
        float fromX = this.mDownX[i];
        float fromY = this.mDownY[i];
        long elapsed = time - this.mDownTime[i];
        this.mCurrentX[i] = x;
        this.mCurrentY[i] = y;
        boolean reachedThreshold = ((fromY > ((float) this.mSwipeStartThreshold) || y <= ((float) this.mSwipeDistanceThreshold) + fromY) && ((fromY < ((float) (this.screenHeight - this.mSwipeStartThreshold)) || y >= fromY - ((float) this.mSwipeDistanceThreshold)) && (fromX < ((float) (this.screenWidth - this.mSwipeStartThreshold)) || x >= fromX - ((float) this.mSwipeDistanceThreshold)))) ? DEBUG : true;
        if (!reachedThreshold) {
            this.mElapsedThreshold[i] = elapsed;
        }
        if (fromY <= ((float) this.mSwipeStartThreshold) && y > ((float) this.mSwipeDistanceThreshold) + fromY && elapsed < SWIPE_TIMEOUT_MS) {
            return 1;
        }
        if (fromY >= ((float) (this.screenHeight - this.mSwipeStartThreshold)) && y < fromY - ((float) this.mSwipeDistanceThreshold) && elapsed < SWIPE_TIMEOUT_MS) {
            return 2;
        }
        if (fromX < ((float) (this.screenWidth - this.mSwipeStartThreshold)) || x >= fromX - ((float) this.mSwipeDistanceThreshold) || elapsed >= SWIPE_TIMEOUT_MS) {
            return SWIPE_NONE;
        }
        return SWIPE_FROM_RIGHT;
    }

    public int getDownPointers() {
        return this.mDownPointers;
    }

    public void getDownPointerInfo(int index, DownPointerInfo out) {
        out.downPointerId = this.mDownPointerId[index];
        out.downX = this.mDownX[index];
        out.downY = this.mDownY[index];
        out.downTime = this.mDownTime[index];
        out.currentX = this.mCurrentX[index];
        out.currentY = this.mCurrentY[index];
        out.elapsedThreshold = this.mElapsedThreshold[index];
    }

    public void getThresholdInfo(ThresholdInfo out) {
        out.start = this.mSwipeStartThreshold;
        out.distance = this.mSwipeDistanceThreshold;
        out.elapsed = SWIPE_TIMEOUT_MS;
    }
}
