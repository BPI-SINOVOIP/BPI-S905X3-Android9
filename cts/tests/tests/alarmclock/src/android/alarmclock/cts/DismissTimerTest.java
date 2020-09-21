package android.alarmclock.cts;

import android.alarmclock.common.Utils;
import android.content.Context;
import android.media.AudioManager;

public class DismissTimerTest extends AlarmClockTestBase {

    private int mSavedVolume;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        // The timer may ring between expiration and dismissal; silence this.
        final AudioManager audioManager = (AudioManager) getInstrumentation().getTargetContext()
                .getSystemService(Context.AUDIO_SERVICE);
        mSavedVolume = audioManager.getStreamVolume(AudioManager.STREAM_ALARM);
        audioManager.setStreamVolume(AudioManager.STREAM_ALARM, 0, 0);
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
        final AudioManager audioManager = (AudioManager) getInstrumentation().getTargetContext()
                .getSystemService(Context.AUDIO_SERVICE);
        audioManager.setStreamVolume(AudioManager.STREAM_ALARM, mSavedVolume, 0);
    }

    public void testAll() throws Exception {
        assertEquals(Utils.COMPLETION_RESULT, runTest(Utils.TestcaseType.SET_TIMER_FOR_DISMISSAL));
        try {
            Thread.sleep(5000);
        } catch (InterruptedException ignored) {
        }
        assertEquals(Utils.COMPLETION_RESULT, runTest(Utils.TestcaseType.DISMISS_TIMER));
    }
}
