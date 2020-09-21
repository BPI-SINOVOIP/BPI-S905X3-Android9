/*
 * Copyright 2018 The Android Open Source Project
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

package androidx.fragment.app;

import static androidx.annotation.RestrictTo.Scope.LIBRARY_GROUP;

import android.os.Bundle;
import android.view.View;

import androidx.annotation.AnimRes;
import androidx.annotation.AnimatorRes;
import androidx.annotation.IdRes;
import androidx.annotation.IntDef;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import androidx.annotation.StringRes;
import androidx.annotation.StyleRes;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Static library support version of the framework's {@link android.app.FragmentTransaction}.
 * Used to write apps that run on platforms prior to Android 3.0.  When running
 * on Android 3.0 or above, this implementation is still used; it does not try
 * to switch to the framework's implementation.  See the framework SDK
 * documentation for a class overview.
 */
public abstract class FragmentTransaction {
    /**
     * Calls {@link #add(int, Fragment, String)} with a 0 containerViewId.
     */
    @NonNull
    public abstract FragmentTransaction add(@NonNull Fragment fragment, @Nullable String tag);

    /**
     * Calls {@link #add(int, Fragment, String)} with a null tag.
     */
    @NonNull
    public abstract FragmentTransaction add(@IdRes int containerViewId, @NonNull Fragment fragment);

    /**
     * Add a fragment to the activity state.  This fragment may optionally
     * also have its view (if {@link Fragment#onCreateView Fragment.onCreateView}
     * returns non-null) into a container view of the activity.
     *
     * @param containerViewId Optional identifier of the container this fragment is
     * to be placed in.  If 0, it will not be placed in a container.
     * @param fragment The fragment to be added.  This fragment must not already
     * be added to the activity.
     * @param tag Optional tag name for the fragment, to later retrieve the
     * fragment with {@link FragmentManager#findFragmentByTag(String)
     * FragmentManager.findFragmentByTag(String)}.
     *
     * @return Returns the same FragmentTransaction instance.
     */
    @NonNull
    public abstract FragmentTransaction add(@IdRes int containerViewId, @NonNull Fragment fragment,
            @Nullable String tag);

    /**
     * Calls {@link #replace(int, Fragment, String)} with a null tag.
     */
    @NonNull
    public abstract FragmentTransaction replace(@IdRes int containerViewId,
            @NonNull Fragment fragment);

    /**
     * Replace an existing fragment that was added to a container.  This is
     * essentially the same as calling {@link #remove(Fragment)} for all
     * currently added fragments that were added with the same containerViewId
     * and then {@link #add(int, Fragment, String)} with the same arguments
     * given here.
     *
     * @param containerViewId Identifier of the container whose fragment(s) are
     * to be replaced.
     * @param fragment The new fragment to place in the container.
     * @param tag Optional tag name for the fragment, to later retrieve the
     * fragment with {@link FragmentManager#findFragmentByTag(String)
     * FragmentManager.findFragmentByTag(String)}.
     *
     * @return Returns the same FragmentTransaction instance.
     */
    @NonNull
    public abstract FragmentTransaction replace(@IdRes int containerViewId,
            @NonNull Fragment fragment, @Nullable String tag);

    /**
     * Remove an existing fragment.  If it was added to a container, its view
     * is also removed from that container.
     *
     * @param fragment The fragment to be removed.
     *
     * @return Returns the same FragmentTransaction instance.
     */
    @NonNull
    public abstract FragmentTransaction remove(@NonNull Fragment fragment);

    /**
     * Hides an existing fragment.  This is only relevant for fragments whose
     * views have been added to a container, as this will cause the view to
     * be hidden.
     *
     * @param fragment The fragment to be hidden.
     *
     * @return Returns the same FragmentTransaction instance.
     */
    @NonNull
    public abstract FragmentTransaction hide(@NonNull Fragment fragment);

    /**
     * Shows a previously hidden fragment.  This is only relevant for fragments whose
     * views have been added to a container, as this will cause the view to
     * be shown.
     *
     * @param fragment The fragment to be shown.
     *
     * @return Returns the same FragmentTransaction instance.
     */
    @NonNull
    public abstract FragmentTransaction show(@NonNull Fragment fragment);

    /**
     * Detach the given fragment from the UI.  This is the same state as
     * when it is put on the back stack: the fragment is removed from
     * the UI, however its state is still being actively managed by the
     * fragment manager.  When going into this state its view hierarchy
     * is destroyed.
     *
     * @param fragment The fragment to be detached.
     *
     * @return Returns the same FragmentTransaction instance.
     */
    @NonNull
    public abstract FragmentTransaction detach(@NonNull Fragment fragment);

    /**
     * Re-attach a fragment after it had previously been detached from
     * the UI with {@link #detach(Fragment)}.  This
     * causes its view hierarchy to be re-created, attached to the UI,
     * and displayed.
     *
     * @param fragment The fragment to be attached.
     *
     * @return Returns the same FragmentTransaction instance.
     */
    @NonNull
    public abstract FragmentTransaction attach(@NonNull Fragment fragment);

    /**
     * Set a currently active fragment in this FragmentManager as the primary navigation fragment.
     *
     * <p>The primary navigation fragment's
     * {@link Fragment#getChildFragmentManager() child FragmentManager} will be called first
     * to process delegated navigation actions such as {@link FragmentManager#popBackStack()}
     * if no ID or transaction name is provided to pop to. Navigation operations outside of the
     * fragment system may choose to delegate those actions to the primary navigation fragment
     * as returned by {@link FragmentManager#getPrimaryNavigationFragment()}.</p>
     *
     * <p>The fragment provided must currently be added to the FragmentManager to be set as
     * a primary navigation fragment, or previously added as part of this transaction.</p>
     *
     * @param fragment the fragment to set as the primary navigation fragment
     * @return the same FragmentTransaction instance
     */
    @NonNull
    public abstract FragmentTransaction setPrimaryNavigationFragment(@Nullable Fragment fragment);

    /**
     * @return <code>true</code> if this transaction contains no operations,
     * <code>false</code> otherwise.
     */
    public abstract boolean isEmpty();

    /**
     * Bit mask that is set for all enter transitions.
     */
    public static final int TRANSIT_ENTER_MASK = 0x1000;

    /**
     * Bit mask that is set for all exit transitions.
     */
    public static final int TRANSIT_EXIT_MASK = 0x2000;

    /** @hide */
    @RestrictTo(LIBRARY_GROUP)
    @IntDef({TRANSIT_NONE, TRANSIT_FRAGMENT_OPEN, TRANSIT_FRAGMENT_CLOSE, TRANSIT_FRAGMENT_FADE})
    @Retention(RetentionPolicy.SOURCE)
    private @interface Transit {}

    /** Not set up for a transition. */
    public static final int TRANSIT_UNSET = -1;
    /** No animation for transition. */
    public static final int TRANSIT_NONE = 0;
    /** Fragment is being added onto the stack */
    public static final int TRANSIT_FRAGMENT_OPEN = 1 | TRANSIT_ENTER_MASK;
    /** Fragment is being removed from the stack */
    public static final int TRANSIT_FRAGMENT_CLOSE = 2 | TRANSIT_EXIT_MASK;
    /** Fragment should simply fade in or out; that is, no strong navigation associated
     * with it except that it is appearing or disappearing for some reason. */
    public static final int TRANSIT_FRAGMENT_FADE = 3 | TRANSIT_ENTER_MASK;

    /**
     * Set specific animation resources to run for the fragments that are
     * entering and exiting in this transaction. These animations will not be
     * played when popping the back stack.
     *
     * @param enter An animation or animator resource ID used for the enter animation on the
     *              view of the fragment being added or attached.
     * @param exit An animation or animator resource ID used for the exit animation on the
     *             view of the fragment being removed or detached.
     */
    @NonNull
    public abstract FragmentTransaction setCustomAnimations(@AnimatorRes @AnimRes int enter,
            @AnimatorRes @AnimRes int exit);

    /**
     * Set specific animation resources to run for the fragments that are
     * entering and exiting in this transaction. The <code>popEnter</code>
     * and <code>popExit</code> animations will be played for enter/exit
     * operations specifically when popping the back stack.
     *
     * @param enter An animation or animator resource ID used for the enter animation on the
     *              view of the fragment being added or attached.
     * @param exit An animation or animator resource ID used for the exit animation on the
     *             view of the fragment being removed or detached.
     * @param popEnter An animation or animator resource ID used for the enter animation on the
     *                 view of the fragment being readded or reattached caused by
     *                 {@link FragmentManager#popBackStack()} or similar methods.
     * @param popExit An animation or animator resource ID used for the enter animation on the
     *                view of the fragment being removed or detached caused by
     *                {@link FragmentManager#popBackStack()} or similar methods.
     */
    @NonNull
    public abstract FragmentTransaction setCustomAnimations(@AnimatorRes @AnimRes int enter,
            @AnimatorRes @AnimRes int exit, @AnimatorRes @AnimRes int popEnter,
            @AnimatorRes @AnimRes int popExit);

    /**
     * Used with custom Transitions to map a View from a removed or hidden
     * Fragment to a View from a shown or added Fragment.
     * <var>sharedElement</var> must have a unique transitionName in the View hierarchy.
     *
     * @param sharedElement A View in a disappearing Fragment to match with a View in an
     *                      appearing Fragment.
     * @param name The transitionName for a View in an appearing Fragment to match to the shared
     *             element.
     * @see Fragment#setSharedElementReturnTransition(Object)
     * @see Fragment#setSharedElementEnterTransition(Object)
     */
    @NonNull
    public abstract FragmentTransaction addSharedElement(@NonNull View sharedElement,
            @NonNull String name);

    /**
     * Select a standard transition animation for this transaction.  May be
     * one of {@link #TRANSIT_NONE}, {@link #TRANSIT_FRAGMENT_OPEN},
     * {@link #TRANSIT_FRAGMENT_CLOSE}, or {@link #TRANSIT_FRAGMENT_FADE}.
     */
    @NonNull
    public abstract FragmentTransaction setTransition(@Transit int transit);

    /**
     * Set a custom style resource that will be used for resolving transit
     * animations.
     */
    @NonNull
    public abstract FragmentTransaction setTransitionStyle(@StyleRes int styleRes);

    /**
     * Add this transaction to the back stack.  This means that the transaction
     * will be remembered after it is committed, and will reverse its operation
     * when later popped off the stack.
     *
     * @param name An optional name for this back stack state, or null.
     */
    @NonNull
    public abstract FragmentTransaction addToBackStack(@Nullable String name);

    /**
     * Returns true if this FragmentTransaction is allowed to be added to the back
     * stack. If this method would return false, {@link #addToBackStack(String)}
     * will throw {@link IllegalStateException}.
     *
     * @return True if {@link #addToBackStack(String)} is permitted on this transaction.
     */
    public abstract boolean isAddToBackStackAllowed();

    /**
     * Disallow calls to {@link #addToBackStack(String)}. Any future calls to
     * addToBackStack will throw {@link IllegalStateException}. If addToBackStack
     * has already been called, this method will throw IllegalStateException.
     */
    @NonNull
    public abstract FragmentTransaction disallowAddToBackStack();

    /**
     * Set the full title to show as a bread crumb when this transaction
     * is on the back stack.
     *
     * @param res A string resource containing the title.
     */
    @NonNull
    public abstract FragmentTransaction setBreadCrumbTitle(@StringRes int res);

    /**
     * Like {@link #setBreadCrumbTitle(int)} but taking a raw string; this
     * method is <em>not</em> recommended, as the string can not be changed
     * later if the locale changes.
     */
    @NonNull
    public abstract FragmentTransaction setBreadCrumbTitle(@Nullable CharSequence text);

    /**
     * Set the short title to show as a bread crumb when this transaction
     * is on the back stack.
     *
     * @param res A string resource containing the title.
     */
    @NonNull
    public abstract FragmentTransaction setBreadCrumbShortTitle(@StringRes int res);

    /**
     * Like {@link #setBreadCrumbShortTitle(int)} but taking a raw string; this
     * method is <em>not</em> recommended, as the string can not be changed
     * later if the locale changes.
     */
    @NonNull
    public abstract FragmentTransaction setBreadCrumbShortTitle(@Nullable CharSequence text);

    /**
     * Sets whether or not to allow optimizing operations within and across
     * transactions. This will remove redundant operations, eliminating
     * operations that cancel. For example, if two transactions are executed
     * together, one that adds a fragment A and the next replaces it with fragment B,
     * the operations will cancel and only fragment B will be added. That means that
     * fragment A may not go through the creation/destruction lifecycle.
     * <p>
     * The side effect of removing redundant operations is that fragments may have state changes
     * out of the expected order. For example, one transaction adds fragment A,
     * a second adds fragment B, then a third removes fragment A. Without removing the redundant
     * operations, fragment B could expect that while it is being created, fragment A will also
     * exist because fragment A will be removed after fragment B was added.
     * With removing redundant operations, fragment B cannot expect fragment A to exist when
     * it has been created because fragment A's add/remove will be optimized out.
     * <p>
     * It can also reorder the state changes of Fragments to allow for better Transitions.
     * Added Fragments may have {@link Fragment#onCreate(Bundle)} called before replaced
     * Fragments have {@link Fragment#onDestroy()} called.
     * <p>
     * {@link Fragment#postponeEnterTransition()} requires {@code setReorderingAllowed(true)}.
     * <p>
     * The default is {@code false}.
     *
     * @param reorderingAllowed {@code true} to enable optimizing out redundant operations
     *                          or {@code false} to disable optimizing out redundant
     *                          operations on this transaction.
     */
    @NonNull
    public abstract FragmentTransaction setReorderingAllowed(boolean reorderingAllowed);

    /**
     * @deprecated This has been renamed {@link #setReorderingAllowed(boolean)}.
     */
    @Deprecated
    public abstract FragmentTransaction setAllowOptimization(boolean allowOptimization);

    /**
     * Add a Runnable to this transaction that will be run after this transaction has
     * been committed. If fragment transactions are {@link #setReorderingAllowed(boolean) optimized}
     * this may be after other subsequent fragment operations have also taken place, or operations
     * in this transaction may have been optimized out due to the presence of a subsequent
     * fragment transaction in the batch.
     *
     * <p>If a transaction is committed using {@link #commitAllowingStateLoss()} this runnable
     * may be executed when the FragmentManager is in a state where new transactions may not
     * be committed without allowing state loss.</p>
     *
     * <p><code>runOnCommit</code> may not be used with transactions
     * {@link #addToBackStack(String) added to the back stack} as Runnables cannot be persisted
     * with back stack state. {@link IllegalStateException} will be thrown if
     * {@link #addToBackStack(String)} has been previously called for this transaction
     * or if it is called after a call to <code>runOnCommit</code>.</p>
     *
     * @param runnable Runnable to add
     * @return this FragmentTransaction
     * @throws IllegalStateException if {@link #addToBackStack(String)} has been called
     */
    @NonNull
    public abstract FragmentTransaction runOnCommit(@NonNull Runnable runnable);

    /**
     * Schedules a commit of this transaction.  The commit does
     * not happen immediately; it will be scheduled as work on the main thread
     * to be done the next time that thread is ready.
     *
     * <p class="note">A transaction can only be committed with this method
     * prior to its containing activity saving its state.  If the commit is
     * attempted after that point, an exception will be thrown.  This is
     * because the state after the commit can be lost if the activity needs to
     * be restored from its state.  See {@link #commitAllowingStateLoss()} for
     * situations where it may be okay to lose the commit.</p>
     *
     * @return Returns the identifier of this transaction's back stack entry,
     * if {@link #addToBackStack(String)} had been called.  Otherwise, returns
     * a negative number.
     */
    public abstract int commit();

    /**
     * Like {@link #commit} but allows the commit to be executed after an
     * activity's state is saved.  This is dangerous because the commit can
     * be lost if the activity needs to later be restored from its state, so
     * this should only be used for cases where it is okay for the UI state
     * to change unexpectedly on the user.
     */
    public abstract int commitAllowingStateLoss();

    /**
     * Commits this transaction synchronously. Any added fragments will be
     * initialized and brought completely to the lifecycle state of their host
     * and any removed fragments will be torn down accordingly before this
     * call returns. Committing a transaction in this way allows fragments
     * to be added as dedicated, encapsulated components that monitor the
     * lifecycle state of their host while providing firmer ordering guarantees
     * around when those fragments are fully initialized and ready. Fragments
     * that manage views will have those views created and attached.
     *
     * <p>Calling <code>commitNow</code> is preferable to calling
     * {@link #commit()} followed by {@link FragmentManager#executePendingTransactions()}
     * as the latter will have the side effect of attempting to commit <em>all</em>
     * currently pending transactions whether that is the desired behavior
     * or not.</p>
     *
     * <p>Transactions committed in this way may not be added to the
     * FragmentManager's back stack, as doing so would break other expected
     * ordering guarantees for other asynchronously committed transactions.
     * This method will throw {@link IllegalStateException} if the transaction
     * previously requested to be added to the back stack with
     * {@link #addToBackStack(String)}.</p>
     *
     * <p class="note">A transaction can only be committed with this method
     * prior to its containing activity saving its state.  If the commit is
     * attempted after that point, an exception will be thrown.  This is
     * because the state after the commit can be lost if the activity needs to
     * be restored from its state.  See {@link #commitAllowingStateLoss()} for
     * situations where it may be okay to lose the commit.</p>
     */
    public abstract void commitNow();

    /**
     * Like {@link #commitNow} but allows the commit to be executed after an
     * activity's state is saved.  This is dangerous because the commit can
     * be lost if the activity needs to later be restored from its state, so
     * this should only be used for cases where it is okay for the UI state
     * to change unexpectedly on the user.
     */
    public abstract void commitNowAllowingStateLoss();
}
