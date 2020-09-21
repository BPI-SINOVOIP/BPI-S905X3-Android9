package js.kbars;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Paint.Cap;
import android.graphics.Paint.Style;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnLayoutChangeListener;
import android.widget.FrameLayout;
import java.util.ArrayList;
import java.util.List;
import js.kbars.SystemGestureDebugger.DownPointerInfo;
import js.kbars.SystemGestureDebugger.ThresholdInfo;

public class TouchTrackingLayout extends FrameLayout {
    private static final boolean DEBUG_EVENTS = true;
    private static final int MAX_INFO_LINES = 6;
    private static final int MAX_POINTERS = 5;
    private static final int MAX_POINTS = 5000;
    private static final String TAG = Util.logTag(TouchTrackingLayout.class);
    private boolean mDebug;
    private final DownPointerInfo mDownPointerInfo = new DownPointerInfo();
    private final SystemGestureDebugger mGestures;
    private final Paint mInfoPaint = new Paint();
    private final StringBuilder[] mInfoText = allocateInfoText();
    private final int mInfoTextLineHeight;
    private final int mInfoTextOffset;
    private final int[] mScribbleCounts = new int[MAX_POINTERS];
    private final Paint mScribblePaint = new Paint();
    private final List<float[]> mScribblePoints = allocatePoints();
    private final int mTextSize;
    private final ThresholdInfo mThresholdInfo = new ThresholdInfo();
    private final float[] mThresholdLines = new float[16];
    private final Paint mThresholdPaint = new Paint();

    public TouchTrackingLayout(Context context) {
        super(context);
        setWillNotDraw(false);
        int densityDpi = Util.getDensityDpi(context);
        this.mTextSize = (int) (((float) densityDpi) / 12.0f);
        this.mInfoTextLineHeight = (int) (((float) densityDpi) / 11.0f);
        this.mInfoTextOffset = (int) (((float) densityDpi) / 1.7f);
        Log.d(TAG, "mTextSize=" + this.mTextSize + " mInfoTextLineHeight=" + this.mInfoTextLineHeight + " mInfoTextOffset=" + this.mInfoTextOffset);
        this.mScribblePaint.setColor(-13388315);
        this.mScribblePaint.setStrokeWidth((float) this.mTextSize);
        this.mScribblePaint.setStrokeCap(Cap.ROUND);
        this.mScribblePaint.setStyle(Style.STROKE);
        this.mGestures = new SystemGestureDebugger(context, new SystemGestureDebugger.Callbacks() {
            public void onSwipeFromTop() {
                Log.d(TouchTrackingLayout.TAG, "GestureCallbacks.onSwipeFromTop");
            }

            public void onSwipeFromRight() {
                Log.d(TouchTrackingLayout.TAG, "GestureCallbacks.onSwipeFromRight");
            }

            public void onSwipeFromBottom() {
                Log.d(TouchTrackingLayout.TAG, "GestureCallbacks.onSwipeFromBottom");
            }

            public void onDebug() {
                Log.d(TouchTrackingLayout.TAG, "GestureCallbacks.onDebug");
            }
        });
        this.mGestures.getThresholdInfo(this.mThresholdInfo);
        this.mInfoPaint.setTypeface(Typeface.MONOSPACE);
        this.mInfoPaint.setTextSize((float) this.mTextSize);
        this.mInfoPaint.setStyle(Style.STROKE);
        this.mInfoPaint.setColor(-13388315);
        this.mThresholdPaint.setColor(-13388315);
        this.mThresholdPaint.setStrokeWidth(1.0f);
        this.mThresholdPaint.setStyle(Style.STROKE);
        addOnLayoutChangeListener(new OnLayoutChangeListener() {
            public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft, int oldTop, int oldRight, int oldBottom) {
                TouchTrackingLayout.this.mGestures.screenWidth = right - left;
                TouchTrackingLayout.this.mGestures.screenHeight = bottom - top;
                TouchTrackingLayout.this.horLine(0, TouchTrackingLayout.this.mThresholdInfo.start);
                TouchTrackingLayout.this.horLine(1, TouchTrackingLayout.this.mGestures.screenHeight - TouchTrackingLayout.this.mThresholdInfo.start);
                TouchTrackingLayout.this.verLine(2, TouchTrackingLayout.this.mThresholdInfo.start);
                TouchTrackingLayout.this.verLine(3, TouchTrackingLayout.this.mGestures.screenWidth - TouchTrackingLayout.this.mThresholdInfo.start);
            }
        });
    }

    public boolean getDebug() {
        return this.mDebug;
    }

    public void setDebug(boolean debug) {
        this.mDebug = debug;
        invalidate();
    }

    protected boolean fitSystemWindows(Rect insets) {
        Rect insetsBefore = new Rect(insets);
        boolean rt = super.fitSystemWindows(insets);
        Log.d(TAG, String.format("fitSystemWindows insetsBefore=%s insetsAfter=%s rt=%s", new Object[]{insetsBefore.toShortString(), insets.toShortString(), Boolean.valueOf(rt)}));
        return rt;
    }

    private void horLine(int lineNum, int y) {
        this.mThresholdLines[(lineNum * 4) + 0] = 0.0f;
        float[] fArr = this.mThresholdLines;
        int i = (lineNum * 4) + 1;
        float f = (float) y;
        this.mThresholdLines[(lineNum * 4) + 3] = f;
        fArr[i] = f;
        this.mThresholdLines[(lineNum * 4) + 2] = (float) this.mGestures.screenWidth;
    }

    private void verLine(int lineNum, int x) {
        float[] fArr = this.mThresholdLines;
        int i = (lineNum * 4) + 0;
        float f = (float) x;
        this.mThresholdLines[(lineNum * 4) + 2] = f;
        fArr[i] = f;
        this.mThresholdLines[(lineNum * 4) + 1] = 0.0f;
        this.mThresholdLines[(lineNum * 4) + 3] = (float) this.mGestures.screenHeight;
    }

    private static StringBuilder[] allocateInfoText() {
        StringBuilder[] rt = new StringBuilder[MAX_INFO_LINES];
        for (int i = 0; i < rt.length; i++) {
            rt[i] = new StringBuilder();
        }
        return rt;
    }

    public boolean onTouchEvent(MotionEvent ev) {
        super.onTouchEvent(ev);
        this.mGestures.onPointerEvent(ev);
        computeInfoText();
        onMotionEvent(ev);
        return true;
    }

    protected void onDraw(Canvas canvas) {
        int i;
        for (i = 0; i < this.mScribbleCounts.length; i++) {
            int c = this.mScribbleCounts[i];
            if (c > 0) {
                canvas.drawLines((float[]) this.mScribblePoints.get(i), 0, c - 2, this.mScribblePaint);
            }
        }
        if (this.mDebug) {
            int leftMargin = this.mThresholdInfo.start + (this.mTextSize / 4);
            for (i = 0; i < this.mInfoText.length; i++) {
                StringBuilder sb = this.mInfoText[i];
                canvas.drawText(sb, 0, sb.length(), (float) leftMargin, (float) (this.mInfoTextOffset + (this.mInfoTextLineHeight * i)), this.mInfoPaint);
            }
            canvas.drawLines(this.mThresholdLines, this.mThresholdPaint);
        }
    }

    private void computeInfoText() {
        int dpc = this.mGestures.getDownPointers();
        for (int i = 0; i < this.mInfoText.length; i++) {
            StringBuilder sb = this.mInfoText[i];
            sb.delete(0, sb.length());
            if (i == 0) {
                sb.append("th  ");
                pad(sb, 9, this.mThresholdInfo.start);
                sb.append("   ");
                pad(sb, 9, this.mThresholdInfo.distance);
                sb.append(' ');
                pad(sb, MAX_POINTERS, (int) this.mThresholdInfo.elapsed);
                sb.append("ms");
            } else if (i - 1 < dpc) {
                this.mGestures.getDownPointerInfo(i - 1, this.mDownPointerInfo);
                sb.append('p');
                sb.append(this.mDownPointerInfo.downPointerId);
                sb.append(' ');
                sb.append('(');
                pad(sb, 4, (int) this.mDownPointerInfo.downX);
                sb.append(',');
                pad(sb, 4, (int) this.mDownPointerInfo.downY);
                sb.append(')');
                sb.append(' ');
                sb.append('(');
                pad(sb, 4, (int) (this.mDownPointerInfo.currentX - this.mDownPointerInfo.downX));
                sb.append(',');
                pad(sb, 4, (int) (this.mDownPointerInfo.currentY - this.mDownPointerInfo.downY));
                sb.append(')');
                sb.append(' ');
                pad(sb, 4, (int) this.mDownPointerInfo.elapsedThreshold);
                sb.append("ms");
            }
        }
    }

    private static void pad(StringBuilder sb, int len, int num) {
        int n = num;
        if (num < 0) {
            n = Math.abs(num);
            len--;
        }
        int nl = n > 0 ? ((int) Math.log10((double) n)) + 1 : 1;
        while (true) {
            int nl2 = nl + 1;
            if (nl >= len) {
                break;
            }
            sb.append(' ');
            nl = nl2;
        }
        if (num < 0) {
            sb.append('-');
        }
        sb.append(n);
    }

    private static List<float[]> allocatePoints() {
        List<float[]> points = new ArrayList();
        for (int i = 0; i < MAX_POINTERS; i++) {
            points.add(new float[MAX_POINTS]);
        }
        return points;
    }

    private void onMotionEvent(MotionEvent ev) {
        long evt;
        int p;
        Log.d(TAG, "EVENT " + ev);
        if (ev.getActionMasked() == 0) {
            clearScribbles();
        }
        int hs = ev.getHistorySize();
        int ps = ev.getPointerCount();
        for (int h = 0; h < hs; h++) {
            evt = ev.getHistoricalEventTime(h);
            for (p = 0; p < ps; p++) {
                int pid = ev.getPointerId(p);
                float hx = ev.getHistoricalX(p, h);
                float hy = ev.getHistoricalY(p, h);
                String str = TAG;
                Object[] objArr = new Object[MAX_POINTERS];
                objArr[0] = Long.valueOf(evt);
                objArr[1] = Integer.valueOf(p);
                objArr[2] = Integer.valueOf(pid);
                objArr[3] = Float.valueOf(hx);
                objArr[4] = Float.valueOf(hy);
                Log.d(str, String.format("h.evt=%s p=%s pid=%s x=%s y=%s", objArr));
                recordScribblePoint(pid, hx, hy);
            }
        }
        evt = ev.getEventTime();
        for (p = 0; p < ps; p++) {
            int pid = ev.getPointerId(p);
            float x = ev.getX(p);
            float y = ev.getY(p);
            String str = TAG;
            Object[] objArr = new Object[MAX_POINTERS];
            objArr[0] = Long.valueOf(evt);
            objArr[1] = Integer.valueOf(p);
            objArr[2] = Integer.valueOf(pid);
            objArr[3] = Float.valueOf(x);
            objArr[4] = Float.valueOf(y);
            Log.d(str, String.format("c.evt=%s p=%s pid=%s x=%s y=%s", objArr));
            recordScribblePoint(pid, x, y);
        }
        invalidate();
    }

    private void clearScribbles() {
        for (int i = 0; i < this.mScribbleCounts.length; i++) {
            this.mScribbleCounts[i] = 0;
        }
    }

    private void recordScribblePoint(int pid, float x, float y) {
        if (pid < MAX_POINTERS) {
            float[] pts = (float[]) this.mScribblePoints.get(pid);
            int oldCount = this.mScribbleCounts[pid];
            if (oldCount + 4 >= MAX_POINTS) {
                return;
            }
            int[] iArr;
            if (oldCount == 0) {
                pts[oldCount + 0] = x;
                pts[oldCount + 1] = y;
                iArr = this.mScribbleCounts;
                iArr[pid] = iArr[pid] + 2;
                return;
            }
            pts[oldCount + 0] = x;
            pts[oldCount + 1] = y;
            pts[oldCount + 2] = x;
            pts[oldCount + 3] = y;
            iArr = this.mScribbleCounts;
            iArr[pid] = iArr[pid] + 4;
        }
    }
}
