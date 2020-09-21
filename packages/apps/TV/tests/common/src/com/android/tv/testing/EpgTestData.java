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
 * limitations under the License
 */

package com.android.tv.testing;

import com.android.tv.data.ChannelImpl;
import com.android.tv.data.Lineup;
import com.android.tv.data.Program;
import com.android.tv.data.api.Channel;
import com.google.common.base.Function;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.Iterables;
import com.google.common.collect.ListMultimap;
import java.util.concurrent.TimeUnit;

/** EPG data for use in tests. */
public abstract class EpgTestData {

    public static final android.support.media.tv.Channel CHANNEL_10 =
            new android.support.media.tv.Channel.Builder()
                    .setDisplayName("Channel TEN")
                    .setDisplayNumber("10")
                    .build();
    public static final android.support.media.tv.Channel CHANNEL_11 =
            new android.support.media.tv.Channel.Builder()
                    .setDisplayName("Channel Eleven")
                    .setDisplayNumber("11")
                    .build();
    public static final android.support.media.tv.Channel CHANNEL_90_2 =
            new android.support.media.tv.Channel.Builder()
                    .setDisplayName("Channel Ninety dot Two")
                    .setDisplayNumber("90.2")
                    .build();

    public static final Lineup LINEUP_1 =
            new Lineup(
                    "lineup1",
                    Lineup.LINEUP_SATELLITE,
                    "Lineup one",
                    "Location one",
                    ImmutableList.of("1", "2.2"));
    public static final Lineup LINEUP_2 =
            new Lineup(
                    "lineup2",
                    Lineup.LINEUP_SATELLITE,
                    "Lineup two",
                    "Location two",
                    ImmutableList.of("1", "2.3"));

    public static final Lineup LINEUP_90210 =
            new Lineup(
                    "test90210",
                    Lineup.LINEUP_BROADCAST_DIGITAL,
                    "Test 90210",
                    "Beverly Hills",
                    ImmutableList.of("90.2", "10"));

    // Programs start and end times are set relative to 0.
    // Then when loaded they are offset by the {@link #getStartTimeMs}.
    // Start and end time may be negative meaning they happen before "now".

    public static final Program PROGRAM_1 =
            new Program.Builder()
                    .setTitle("Program 1")
                    .setStartTimeUtcMillis(0)
                    .setEndTimeUtcMillis(TimeUnit.MINUTES.toMillis(30))
                    .build();

    public static final Program PROGRAM_2 =
            new Program.Builder()
                    .setTitle("Program 2")
                    .setStartTimeUtcMillis(TimeUnit.MINUTES.toMillis(30))
                    .setEndTimeUtcMillis(TimeUnit.MINUTES.toMillis(60))
                    .build();

    public static final EpgTestData DATA_90210 =
            new EpgTestData() {

                //  Thursday, June 1, 2017 4:00:00 PM GMT-07:00
                private final long testStartTimeMs = 1496358000000L;

                @Override
                public ListMultimap<String, Lineup> getLineups() {
                    ImmutableListMultimap.Builder<String, Lineup> builder =
                            ImmutableListMultimap.builder();
                    return builder.putAll("90210", LINEUP_1, LINEUP_2, LINEUP_90210).build();
                }

                @Override
                public ListMultimap<String, Channel> getLineupChannels() {
                    ImmutableListMultimap.Builder<String, Channel> builder =
                            ImmutableListMultimap.builder();
                    return builder.putAll(
                                    LINEUP_90210.getId(), toTvChannels(CHANNEL_90_2, CHANNEL_10))
                            .putAll(LINEUP_1.getId(), toTvChannels(CHANNEL_10, CHANNEL_11))
                            .build();
                }

                @Override
                public ListMultimap<String, Program> getEpgPrograms() {
                    ImmutableListMultimap.Builder<String, Program> builder =
                            ImmutableListMultimap.builder();
                    return builder.putAll(
                                    CHANNEL_10.getDisplayNumber(),
                                    EpgTestData.updateTime(getStartTimeMs(), PROGRAM_1))
                            .putAll(
                                    CHANNEL_11.getDisplayNumber(),
                                    EpgTestData.updateTime(getStartTimeMs(), PROGRAM_2))
                            .build();
                }

                @Override
                public long getStartTimeMs() {
                    return testStartTimeMs;
                }
            };

    public abstract ListMultimap<String, Lineup> getLineups();

    public abstract ListMultimap<String, Channel> getLineupChannels();

    public abstract ListMultimap<String, Program> getEpgPrograms();

    /** The starting time for this test data */
    public abstract long getStartTimeMs();

    /**
     * Loads test data
     *
     * <p>
     *
     * <ul>
     *   <li>Sets clock to {@link #getStartTimeMs()} and boot time to 12 hours before that
     *   <li>Loads lineups
     *   <li>Loads lineupChannels
     *   <li>Loads epgPrograms
     * </ul>
     */
    public final void loadData(FakeClock clock, FakeEpgReader epgReader) {
        clock.setBootTimeMillis(getStartTimeMs() + TimeUnit.HOURS.toMillis(-12));
        clock.setCurrentTimeMillis(getStartTimeMs());
        epgReader.zip2lineups.putAll(getLineups());
        epgReader.lineup2Channels.putAll(getLineupChannels());
        epgReader.epgChannelId2Programs.putAll(getEpgPrograms());
    }

    public final void loadData(TestSingletonApp testSingletonApp) {
        loadData(testSingletonApp.fakeClock, testSingletonApp.epgReader);
    }

    private static Iterable<Channel> toTvChannels(android.support.media.tv.Channel... channels) {
        return Iterables.transform(
                ImmutableList.copyOf(channels),
                new Function<android.support.media.tv.Channel, Channel>() {
                    @Override
                    public Channel apply(android.support.media.tv.Channel original) {
                        return toTvChannel(original);
                    }
                });
    }

    public static Channel toTvChannel(android.support.media.tv.Channel original) {
        return new ChannelImpl.Builder()
                .setDisplayName(original.getDisplayName())
                .setDisplayNumber(original.getDisplayNumber())
                // TODO implement the reset
                .build();
    }

    /** Add time to the startTime and stopTime of each program */
    private static Iterable<Program> updateTime(long time, Program... programs) {
        return Iterables.transform(
                ImmutableList.copyOf(programs),
                new Function<Program, Program>() {
                    @Override
                    public Program apply(Program p) {
                        return new Program.Builder(p)
                                .setStartTimeUtcMillis(p.getStartTimeUtcMillis() + time)
                                .setEndTimeUtcMillis(p.getEndTimeUtcMillis() + time)
                                .build();
                    }
                });
    }
}
