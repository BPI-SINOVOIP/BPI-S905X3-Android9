package com.xtremelabs.robolectric.shadows;

import android.app.Dialog;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;
import com.xtremelabs.robolectric.internal.Implementation;
import com.xtremelabs.robolectric.internal.Implements;
import com.xtremelabs.robolectric.internal.RealObject;
import com.xtremelabs.robolectric.tester.android.util.TestFragmentManager;
import com.xtremelabs.robolectric.tester.android.util.TestFragmentTransaction;

import static com.xtremelabs.robolectric.Robolectric.shadowOf;

/**
 * Shadow implementation of the {@link DialogFragment}.
 */
@Implements(DialogFragment.class)
public class ShadowDialogFragment extends ShadowFragment {
    private static DialogFragment latestDialogFragment;

    private Dialog dialog;
    private boolean cancelable = true;  // defaults to true

    @RealObject
    private DialogFragment realDialogFragment;

    @Implementation
    public int show(FragmentTransaction transaction, String tag) {
        show(((TestFragmentTransaction)transaction).getManager(), tag);
        return 0;
    }

    @Implementation
    public void show(FragmentManager manager, String tag) {
        TestFragmentManager testFragmentManager = (TestFragmentManager) manager;
        FragmentActivity activityFromManager = testFragmentManager.getActivity();

        shadowOf(realDialogFragment).setActivity(activityFromManager);

        realDialogFragment.onAttach(activity);
        realDialogFragment.onCreate(null);
        dialog = realDialogFragment.onCreateDialog(null);
        view = realDialogFragment.onCreateView(ShadowLayoutInflater.from(activity), null, null);
        if (dialog == null) {
            dialog = new Dialog(activityFromManager);
            dialog.setContentView(view);
        }
        dialog.setCancelable(cancelable);
        testFragmentManager.addDialogFragment(tag, realDialogFragment);
        realDialogFragment.onViewCreated(view, null);
        realDialogFragment.onActivityCreated(null);
        realDialogFragment.onStart();
        realDialogFragment.onResume();

        latestDialogFragment = realDialogFragment;
    }

    @Implementation
    public void onStart() {
        if (dialog != null) {
            dialog.show();
        }
    }

    @Implementation
    public void dismiss() {
        if (dialog != null) {
            dialog.dismiss();
        }
    }

    @Implementation
    public Dialog getDialog() {
        return dialog;
    }

    @Implementation
    public boolean isCancelable() {
        return cancelable;
    }

    @Implementation
    public void setCancelable(boolean cancelable) {
        this.cancelable = cancelable;
        if (dialog != null) {
            dialog.setCancelable(cancelable);
        }
    }

    public static DialogFragment getLatestDialogFragment() {
        return latestDialogFragment;
    }
}
