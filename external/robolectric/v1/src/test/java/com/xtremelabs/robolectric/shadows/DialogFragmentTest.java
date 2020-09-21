package com.xtremelabs.robolectric.shadows;

import android.app.Activity;
import android.app.Dialog;
import android.os.Bundle;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentTransaction;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import com.xtremelabs.robolectric.R;
import com.xtremelabs.robolectric.WithTestDefaultsRunner;
import com.xtremelabs.robolectric.tester.android.util.TestFragmentManager;
import com.xtremelabs.robolectric.util.Transcript;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import static com.xtremelabs.robolectric.Robolectric.shadowOf;
import static org.junit.Assert.*;

@RunWith(WithTestDefaultsRunner.class)
public class DialogFragmentTest {

    private FragmentActivity activity;
    private TestDialogFragment dialogFragment;
    private TestFragmentManager fragmentManager;

    @Before
    public void setUp() throws Exception {
        activity = new FragmentActivity();
        dialogFragment = new TestDialogFragment();
        fragmentManager = new TestFragmentManager(activity);
    }

    @Test
    public void show_shouldCallLifecycleMethods() throws Exception {
        dialogFragment.show(fragmentManager, "this is a tag");

        dialogFragment.transcript.assertEventsSoFar(
                "onAttach",
                "onCreate",
                "onCreateDialog",
                "onCreateView",
                "onViewCreated",
                "onActivityCreated",
                "onStart",
                "onResume"
        );

        assertNotNull(dialogFragment.getActivity());
        assertSame(activity, dialogFragment.onAttachActivity);
    }

    @Test
    public void show_whenPassedATransaction_shouldCallShowWithManager() throws Exception {
        dialogFragment.show(fragmentManager.beginTransaction(), "this is a tag");

        dialogFragment.transcript.assertEventsSoFar(
                "onAttach",
                "onCreate",
                "onCreateDialog",
                "onCreateView",
                "onViewCreated",
                "onActivityCreated",
                "onStart",
                "onResume"
        );

        assertNotNull(dialogFragment.getActivity());
        assertSame(activity, dialogFragment.onAttachActivity);
    }

    @Test
    public void show_shouldShowDialogThatWasReturnedFromOnCreateDialog_whenOnCreateDialogReturnsADialog() throws Exception {
        Dialog dialogFromOnCreateDialog = new Dialog(activity);
        dialogFragment.returnThisDialogFromOnCreateDialog(dialogFromOnCreateDialog);
        dialogFragment.show(fragmentManager, "this is a tag");

        Dialog dialog = ShadowDialog.getLatestDialog();
        assertSame(dialogFromOnCreateDialog, dialog);
        assertSame(dialogFromOnCreateDialog, dialogFragment.getDialog());
        assertSame(dialogFragment, fragmentManager.findFragmentByTag("this is a tag"));
    }

    @Test
    public void show_shouldShowDialogThatWasAutomaticallyCreated_whenOnCreateDialogReturnsNull() throws Exception {
        dialogFragment.show(fragmentManager, "this is a tag");

        Dialog dialog = ShadowDialog.getLatestDialog();
        assertNotNull(dialog);
        assertSame(dialog, dialogFragment.getDialog());
        assertNotNull(dialog.findViewById(R.id.title));
        assertSame(dialogFragment, fragmentManager.findFragmentByTag("this is a tag"));
    }

    @Test
    public void dismiss_shouldDismissTheDialog() throws Exception {
        dialogFragment.show(fragmentManager, "tag");

        dialogFragment.dismiss();

        Dialog dialog = ShadowDialog.getLatestDialog();
        assertFalse(dialog.isShowing());
        assertTrue(shadowOf(dialog).hasBeenDismissed());
    }

    @Test
    public void removeUsingTransaction_shouldDismissTheDialog() throws Exception {
        dialogFragment.show(fragmentManager, null);

        FragmentTransaction t = fragmentManager.beginTransaction();
        t.remove(dialogFragment);
        t.commit();

        Dialog dialog = ShadowDialog.getLatestDialog();
        assertFalse(dialog.isShowing());
        assertTrue(shadowOf(dialog).hasBeenDismissed());
    }

    @Test
    public void shouldDefaultToCancelable() throws Exception {
        DialogFragment dialogFragment = new DialogFragment();

        assertTrue(dialogFragment.isCancelable());
    }

    @Test
    public void shouldStoreCancelable() throws Exception {
        DialogFragment dialogFragment = new DialogFragment();

        dialogFragment.setCancelable(false);

        assertFalse(dialogFragment.isCancelable());
    }

    @Test
    public void shouldSetCancelableOnDialog() throws Exception {
        DialogFragment dialogFragment = new DialogFragment();
        dialogFragment.show(fragmentManager, "TAG");

        Dialog dialog = dialogFragment.getDialog();

        assertTrue(shadowOf(dialog).isCancelable());
    }

    @Test
    public void shouldSetNotCancelableOnDialogBeforeShow() throws Exception {
        DialogFragment dialogFragment = new DialogFragment();
        dialogFragment.setCancelable(false);
        dialogFragment.show(fragmentManager, "TAG");

        Dialog dialog = dialogFragment.getDialog();

        assertFalse(shadowOf(dialog).isCancelable());
    }

    @Test
    public void shouldSetNotCancelableOnDialogAfterShow() throws Exception {
        DialogFragment dialogFragment = new DialogFragment();
        dialogFragment.show(fragmentManager, "TAG");
        dialogFragment.setCancelable(false);

        Dialog dialog = dialogFragment.getDialog();

        assertFalse(shadowOf(dialog).isCancelable());
    }

    private class TestDialogFragment extends DialogFragment {
        final Transcript transcript = new Transcript();
        Activity onAttachActivity;
        private Dialog returnThisDialogFromOnCreateDialog;

        @Override
        public void onAttach(Activity activity) {
            transcript.add("onAttach");
            onAttachActivity = activity;
            super.onAttach(activity);
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            transcript.add("onCreate");
        }

        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            transcript.add("onCreateDialog");
            return returnThisDialogFromOnCreateDialog;
        }

        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
            transcript.add("onCreateView");
            return inflater.inflate(R.layout.main, null);
        }

        @Override
        public void onViewCreated(View view, Bundle savedInstanceState) {
            transcript.add("onViewCreated");
            super.onViewCreated(view, savedInstanceState);
        }

        @Override
        public void onActivityCreated(Bundle savedInstanceState) {
            transcript.add("onActivityCreated");
            super.onActivityCreated(savedInstanceState);
        }

        @Override
        public void onStart() {
            transcript.add("onStart");
            super.onStart();
        }

        @Override
        public void onResume() {
            transcript.add("onResume");
            super.onResume();
        }

        public void returnThisDialogFromOnCreateDialog(Dialog dialog) {
            returnThisDialogFromOnCreateDialog = dialog;
        }
    }
}
