/*
 * Copyright (C) 2019 Amlogic Corporation.
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

package com.droidlogic.tvinput.services;

interface IAudioEffectsService{
    void createAudioEffects();
    boolean isSupportVirtualX();
    void setDtsVirtualXMode(int virtalXMode);
    int getDtsVirtualXMode();
    void setDtsTruVolumeHdEnable(boolean enable);
    boolean getDtsTruVolumeHdEnable();
    int getSoundModeStatus();
    int getSoundModule();
    int getTrebleStatus();
    int getBassStatus();
    int getBalanceStatus();
    boolean getAgcEnableStatus();
    int getAgcMaxLevelStatus();
    int getAgcAttrackTimeStatus();
    int getAgcReleaseTimeStatus();
    int getAgcSourceIdStatus();
    int getVirtualSurroundStatus();
    void setSoundMode(int mode);
    void setSoundModeByObserver(int mode);
    void setUserSoundModeParam(int bandNumber, int value);
    int getUserSoundModeParam(int bandNumber);
    void setTreble(int step);
    void setBass(int step);
    void setBalance(int step);
    void setSurroundEnable(boolean enable);
    boolean getSurroundEnable();
    void setDialogClarityMode(int mode);
    int getDialogClarityMode();
    void setTruBassEnable(boolean enable);
    boolean getTruBassEnable();
    void setAgcEnable(boolean enable);
    void setAgcMaxLevel(int step);
    void setAgcAttrackTime(int step);
    void setAgcReleaseTime(int step);
    void setSourceIdForAvl(int step);
    void setVirtualSurround(int mode);
    void setDbxEnable(boolean enable);
    boolean getDbxEnable();
    void setDbxSoundMode(int dbxMode);
    int getDbxSoundMode();
    void setDbxAdvancedModeParam(int paramType, int value);
    int getDbxAdvancedModeParam(int paramType);
    void setAudioOutputSpeakerDelay(int source, int delayMs);
    int getAudioOutputSpeakerDelay(int source);
    void setAudioOutputSpdifDelay(int source, int delayMs);
    int getAudioOutputSpdifDelay(int source);
    void setAudioPrescale(int source,int value);
    int getAudioPrescale(int source);
    void cleanupAudioEffects();
    void initSoundEffectSettings();
    void resetSoundEffectSettings();
}
