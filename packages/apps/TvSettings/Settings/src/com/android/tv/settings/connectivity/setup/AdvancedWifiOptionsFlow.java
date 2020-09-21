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

package com.android.tv.settings.connectivity.setup;

import android.arch.lifecycle.ViewModelProviders;
import android.net.IpConfiguration;
import android.support.annotation.IntDef;
import android.support.v4.app.FragmentActivity;
import android.util.Log;

import com.android.tv.settings.connectivity.NetworkConfiguration;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;


/**
 * Handles the flow of setting advanced options.
 */
public class AdvancedWifiOptionsFlow {

    /** Flag that set advanced flow start with default page */
    public static final int START_DEFAULT_PAGE = 0;
    /** Flag that set advanced flow start with IP settings page */
    public static final int START_IP_SETTINGS_PAGE = 1;
    /** Flag that set advanced flow start with proxy settings page */
    public static final int START_PROXY_SETTINGS_PAGE = 2;
    private static final String TAG = "AdvancedWifiOptionsFlow";

    @IntDef({
            START_DEFAULT_PAGE,
            START_IP_SETTINGS_PAGE,
            START_PROXY_SETTINGS_PAGE
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface START_PAGE {
    }

    /**
     * Create a advanced flow.
     *
     * @param activity             activity that starts the advanced flow.
     * @param askFirst             whether ask user to start advanced flow
     * @param isSettingsFlow       whether advanced flow is started from settings flow
     * @param initialConfiguration the previous {@link NetworkConfiguration} info.
     * @param entranceState        The state that starts the advanced flow, null if there is none.
     * @param exitState            The state where the advanced flow go after it ends.
     * @param startPage            The page where the advanced flow starts with.
     */
    public static void createFlow(FragmentActivity activity,
            boolean askFirst,
            boolean isSettingsFlow,
            NetworkConfiguration initialConfiguration,
            State entranceState,
            State exitState,
            @START_PAGE int startPage) {
        StateMachine stateMachine = ViewModelProviders.of(activity).get(StateMachine.class);
        AdvancedOptionsFlowInfo advancedOptionsFlowInfo = ViewModelProviders.of(activity).get(
                AdvancedOptionsFlowInfo.class);
        advancedOptionsFlowInfo.setSettingsFlow(isSettingsFlow);
        IpConfiguration ipConfiguration = (initialConfiguration != null)
                ? initialConfiguration.getIpConfiguration()
                : new IpConfiguration();
        advancedOptionsFlowInfo.setIpConfiguration(ipConfiguration);
        State advancedOptionsState = new AdvancedOptionsState(activity);
        State proxySettingsState = new ProxySettingsState(activity);
        State ipSettingsState = new IpSettingsState(activity);
        State proxyHostNameState = new ProxyHostNameState(activity);
        State proxyPortState = new ProxyPortState(activity);
        State proxyBypassState = new ProxyBypassState(activity);
        State proxySettingsInvalidState = new ProxySettingsInvalidState(activity);
        State ipAddressState = new IpAddressState(activity);
        State gatewayState = new GatewayState(activity);
        State networkPrefixLengthState = new NetworkPrefixLengthState(activity);
        State dns1State = new Dns1State(activity);
        State dns2State = new Dns2State(activity);
        State ipSettingsInvalidState = new IpSettingsInvalidState(activity);
        State advancedFlowCompleteState = new AdvancedFlowCompleteState(activity);

        // Define the transitions between external states and internal states for advanced options
        // flow.
        State startState = null;
        switch (startPage) {
            case START_DEFAULT_PAGE :
                if (askFirst) {
                    startState = advancedOptionsState;
                } else {
                    startState = proxySettingsState;
                }
                break;
            case START_IP_SETTINGS_PAGE :
                startState = ipSettingsState;
                break;
            case START_PROXY_SETTINGS_PAGE :
                startState = proxySettingsState;
                break;
            default:
                Log.wtf(TAG, "Got a wrong start state");
                break;
        }

        /* Entrance */
        if (entranceState != null) {
            stateMachine.addState(
                    entranceState,
                    StateMachine.ENTER_ADVANCED_FLOW,
                    startState
            );
        } else {
            stateMachine.setStartState(startState);
        }

        /* Exit */
        stateMachine.addState(
                advancedFlowCompleteState,
                StateMachine.EXIT_ADVANCED_FLOW,
                exitState
        );

        // Define the transitions between different states in advanced options flow.
        /* Advanced Options */
        stateMachine.addState(
                advancedOptionsState,
                StateMachine.ADVANCED_FLOW_COMPLETE,
                advancedFlowCompleteState
        );
        stateMachine.addState(
                advancedOptionsState,
                StateMachine.CONTINUE,
                proxySettingsState
        );

        /* Proxy Settings */
        stateMachine.addState(
                proxySettingsState,
                StateMachine.IP_SETTINGS,
                ipSettingsState
        );
        stateMachine.addState(
                proxySettingsState,
                StateMachine.ADVANCED_FLOW_COMPLETE,
                advancedFlowCompleteState
        );
        stateMachine.addState(
                proxySettingsState,
                StateMachine.PROXY_HOSTNAME,
                proxyHostNameState
        );

        /* Proxy Hostname */
        stateMachine.addState(
                proxyHostNameState,
                StateMachine.CONTINUE,
                proxyPortState
        );

        /* Proxy Port */
        stateMachine.addState(
                proxyPortState,
                StateMachine.CONTINUE,
                proxyBypassState
        );

        /* Proxy Bypass */
        stateMachine.addState(
                proxyBypassState,
                StateMachine.ADVANCED_FLOW_COMPLETE,
                advancedFlowCompleteState
        );
        stateMachine.addState(
                proxyBypassState,
                StateMachine.IP_SETTINGS,
                ipSettingsState
        );
        stateMachine.addState(
                proxyBypassState,
                StateMachine.PROXY_SETTINGS_INVALID,
                proxySettingsInvalidState
        );

        /* Proxy Settings Invalid */
        stateMachine.addState(
                proxySettingsInvalidState,
                StateMachine.CONTINUE,
                proxySettingsState
        );

        /* Ip Settings */
        stateMachine.addState(
                ipSettingsState,
                StateMachine.ADVANCED_FLOW_COMPLETE,
                advancedFlowCompleteState
        );
        stateMachine.addState(
                ipSettingsState,
                StateMachine.CONTINUE,
                ipAddressState
        );

        /* Ip Address */
        stateMachine.addState(
                ipAddressState,
                StateMachine.CONTINUE,
                gatewayState
        );

        /* Gateway */
        stateMachine.addState(
                gatewayState,
                StateMachine.CONTINUE,
                networkPrefixLengthState
        );

        /* Network Prefix Length */
        stateMachine.addState(
                networkPrefixLengthState,
                StateMachine.CONTINUE,
                dns1State
        );

        /* Dns1 */
        stateMachine.addState(
                dns1State,
                StateMachine.CONTINUE,
                dns2State
        );

        /* Dns2 */
        stateMachine.addState(
                dns2State,
                StateMachine.ADVANCED_FLOW_COMPLETE,
                advancedFlowCompleteState);
        stateMachine.addState(
                dns2State,
                StateMachine.IP_SETTINGS_INVALID,
                ipSettingsInvalidState
        );

        /* Ip Settings Invalid */
        stateMachine.addState(
                ipSettingsInvalidState,
                StateMachine.CONTINUE,
                ipSettingsState
        );
    }
}
