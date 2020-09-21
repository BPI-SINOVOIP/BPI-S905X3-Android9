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

package com.android.tv.settings.connectivity.util;

import android.app.Activity;
import android.arch.lifecycle.ViewModel;
import android.support.annotation.IntDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

/**
 * State machine responsible for handling the logic between different states.
 */
public class StateMachine extends ViewModel {

    private Callback mCallback;
    private Map<State, List<Transition>> mTransitionMap = new HashMap<>();
    private LinkedList<State> mStatesList = new LinkedList<>();
    private State.StateCompleteListener mCompletionListener = this::updateState;

    public static final int ADD_START = 0;
    public static final int CANCEL = 1;
    public static final int CONTINUE = 2;
    public static final int FAIL = 3;
    public static final int EARLY_EXIT = 4;
    public static final int CONNECT = 5;
    public static final int SELECT_WIFI = 6;
    public static final int PASSWORD = 7;
    public static final int OTHER_NETWORK = 8;
    public static final int KNOWN_NETWORK = 9;
    public static final int RESULT_REJECTED_BY_AP = 10;
    public static final int RESULT_UNKNOWN_ERROR = 11;
    public static final int RESULT_TIMEOUT = 12;
    public static final int RESULT_BAD_AUTH = 13;
    public static final int RESULT_SUCCESS = 14;
    public static final int RESULT_FAILURE = 15;
    public static final int TRY_AGAIN = 16;
    public static final int ADD_PAGE_BASED_ON_NETWORK_CHOICE = 17;
    public static final int OPTIONS_OR_CONNECT = 18;
    public static final int IP_SETTINGS = 19;
    public static final int IP_SETTINGS_INVALID = 20;
    public static final int PROXY_HOSTNAME = 21;
    public static final int PROXY_SETTINGS_INVALID = 22;
    public static final int ADVANCED_FLOW_COMPLETE = 23;
    public static final int ENTER_ADVANCED_FLOW = 24;
    public static final int EXIT_ADVANCED_FLOW = 25;
    @IntDef({
            ADD_START,
            CANCEL,
            CONTINUE,
            FAIL,
            EARLY_EXIT,
            CONNECT,
            SELECT_WIFI,
            PASSWORD,
            OTHER_NETWORK,
            KNOWN_NETWORK,
            RESULT_REJECTED_BY_AP,
            RESULT_UNKNOWN_ERROR,
            RESULT_TIMEOUT,
            RESULT_BAD_AUTH,
            RESULT_SUCCESS,
            RESULT_FAILURE,
            TRY_AGAIN,
            ADD_PAGE_BASED_ON_NETWORK_CHOICE,
            OPTIONS_OR_CONNECT,
            IP_SETTINGS,
            IP_SETTINGS_INVALID,
            PROXY_HOSTNAME,
            PROXY_SETTINGS_INVALID,
            ADVANCED_FLOW_COMPLETE,
            ENTER_ADVANCED_FLOW,
            EXIT_ADVANCED_FLOW})
    @Retention(RetentionPolicy.SOURCE)
    public @interface Event {
    }

    public StateMachine() {
    }

    public StateMachine(Callback callback) {
        mCallback = callback;
    }

    /**
     * Set the callback for the things need to done when the state machine leaves end state.
     */
    public void setCallback(Callback callback) {
        mCallback = callback;
    }

    /**
     * Add state with transition.
     *
     * @param state       start state.
     * @param event       transition between two states.
     * @param destination destination state.
     */
    public void addState(State state, @Event int event, State destination) {
        if (!mTransitionMap.containsKey(state)) {
            mTransitionMap.put(state, new ArrayList<>());
        }

        mTransitionMap.get(state).add(new Transition(state, event, destination));
    }

    /**
     * Add a state that has no outward transitions, but will end the state machine flow.
     */
    public void addTerminalState(State state) {
        mTransitionMap.put(state, new ArrayList<>());
    }

    /**
     * Enables the activity to be notified when state machine enter end state.
     */
    public interface Callback {
        /**
         * Implement this to define what to do when the activity is finished.
         *
         * @param result the activity result.
         */
        void onFinish(int result);
    }

    /**
     * Set the start state of state machine/
     *
     * @param startState start state.
     */
    public void setStartState(State startState) {
        mStatesList.addLast(startState);
    }

    /**
     * Start the state machine.
     */
    public void start(boolean movingForward) {
        if (mStatesList.isEmpty()) {
            throw new IllegalArgumentException("Start state not set");
        }
        State currentState = getCurrentState();
        if (movingForward) {
            currentState.processForward();
        } else {
            currentState.processBackward();
        }
    }

    /**
     * Initialize the states list.
     */
    public void reset() {
        mStatesList = new LinkedList<>();
    }

    /**
     * Make the state machine go back to the previous state.
     */
    public void back() {
        updateState(CANCEL);
    }

    /**
     * Return the current state of state machine.
     */
    public State getCurrentState() {
        if (!mStatesList.isEmpty()) {
            return mStatesList.getLast();
        } else {
            return null;
        }
    }

    /**
     * Notify state machine that current activity is finished.
     *
     * @param result the result of activity.
     */
    public void finish(int result) {
        mCallback.onFinish(result);
    }

    private void updateState(@Event int event) {
        // Handle early exits first.
        if (event == EARLY_EXIT) {
            finish(Activity.RESULT_OK);
            return;
        } else if (event == FAIL) {
            finish(Activity.RESULT_CANCELED);
            return;
        }

        // Handle Event.CANCEL, it happens when the back button is pressed.
        if (event == CANCEL) {
            if (mStatesList.size() < 2) {
                mCallback.onFinish(Activity.RESULT_CANCELED);
            } else {
                mStatesList.removeLast();
                State prev = mStatesList.getLast();
                prev.processBackward();
            }
            return;
        }

        State next = null;
        State currentState = getCurrentState();

        List<Transition> list = mTransitionMap.get(currentState);
        if (list != null) {
            for (Transition transition : mTransitionMap.get(currentState)) {
                if (transition.event == event) {
                    next = transition.destination;
                }
            }
        }

        if (next == null) {
            if (event == CONTINUE) {
                mCallback.onFinish(Activity.RESULT_OK);
                return;
            }
            throw new IllegalArgumentException(
                    getCurrentState().getClass() + "Invalid transition " + event);
        }

        addToStack(next);
        next.processForward();
    }

    private void addToStack(State state) {
        for (int i = mStatesList.size() - 1; i >= 0; i--) {
            if (state.getClass().equals(mStatesList.get(i).getClass())) {
                for (int j = mStatesList.size() - 1; j >= i; j--) {
                    mStatesList.removeLast();
                }
            }
        }
        mStatesList.addLast(state);
    }

    public State.StateCompleteListener getListener() {
        return mCompletionListener;
    }
}
