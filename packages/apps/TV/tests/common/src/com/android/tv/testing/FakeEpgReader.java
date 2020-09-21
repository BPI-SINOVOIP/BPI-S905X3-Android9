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

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.Range;
import com.android.tv.data.ChannelImpl;
import com.android.tv.data.ChannelNumber;
import com.android.tv.data.Lineup;
import com.android.tv.data.Program;
import com.android.tv.data.api.Channel;
import com.android.tv.data.epg.EpgReader;
import com.android.tv.dvr.data.SeriesInfo;
import com.google.common.base.Function;
import com.google.common.base.Predicate;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.Iterables;
import com.google.common.collect.LinkedListMultimap;
import com.google.common.collect.ListMultimap;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/** Fake {@link EpgReader} for testing. */
public final class FakeEpgReader implements EpgReader {
    public final ListMultimap<String, Lineup> zip2lineups = LinkedListMultimap.create(2);
    public final ListMultimap<String, Channel> lineup2Channels = LinkedListMultimap.create(2);
    public final ListMultimap<String, Program> epgChannelId2Programs = LinkedListMultimap.create(2);
    public final FakeClock fakeClock;

    public FakeEpgReader(FakeClock fakeClock) {
        this.fakeClock = fakeClock;
    }

    @Override
    public boolean isAvailable() {
        return true;
    }

    @Override
    public long getEpgTimestamp() {
        return fakeClock.currentTimeMillis();
    }

    @Override
    public void setRegionCode(String regionCode) {}

    @Override
    public List<Lineup> getLineups(@NonNull String postalCode) {
        return zip2lineups.get(postalCode);
    }

    @Override
    public List<String> getChannelNumbers(@NonNull String lineupId) {
        return null;
    }

    @Override
    public Set<EpgChannel> getChannels(Set<Channel> inputChannels, @NonNull String lineupId) {
        Set<EpgChannel> result = new HashSet<>();
        List<Channel> lineupChannels = lineup2Channels.get(lineupId);
        for (Channel channel : lineupChannels) {
            Channel match =
                    Iterables.find(
                            inputChannels,
                            new Predicate<Channel>() {
                                @Override
                                public boolean apply(@Nullable Channel inputChannel) {
                                    return ChannelNumber.equivalent(
                                            inputChannel.getDisplayNumber(),
                                            channel.getDisplayNumber());
                                }
                            },
                            null);
            if (match != null) {
                ChannelImpl updatedChannel = new ChannelImpl.Builder(match).build();
                updatedChannel.setLogoUri(channel.getLogoUri());
                result.add(EpgChannel.createEpgChannel(updatedChannel, channel.getDisplayNumber()));
            }
        }
        return result;
    }

    @Override
    public void preloadChannels(@NonNull String lineupId) {}

    @Override
    public void clearCachedChannels(@NonNull String lineupId) {}

    @Override
    public List<Program> getPrograms(EpgChannel epgChannel) {
        return ImmutableList.copyOf(
                Iterables.transform(
                        epgChannelId2Programs.get(epgChannel.getEpgChannelId()),
                        updateWith(epgChannel)));
    }

    @Override
    public Map<EpgChannel, Collection<Program>> getPrograms(
            @NonNull Set<EpgChannel> epgChannels, long duration) {
        Range<Long> validRange =
                Range.create(
                        fakeClock.currentTimeMillis(), fakeClock.currentTimeMillis() + duration);
        ImmutableMap.Builder<EpgChannel, Collection<Program>> mapBuilder = ImmutableMap.builder();
        for (EpgChannel epgChannel : epgChannels) {
            Iterable<Program> programs = getPrograms(epgChannel);

            mapBuilder.put(
                    epgChannel,
                    ImmutableList.copyOf(Iterables.filter(programs, isProgramDuring(validRange))));
        }
        return mapBuilder.build();
    }

    protected Function<Program, Program> updateWith(final EpgChannel channel) {
        return new Function<Program, Program>() {
            @Nullable
            @Override
            public Program apply(@Nullable Program program) {
                return new Program.Builder(program)
                        .setChannelId(channel.getChannel().getId())
                        .setPackageName(channel.getChannel().getPackageName())
                        .build();
            }
        };
    }

    /**
     * True if the start time or the end time is {@link Range#contains contained (inclusive)} in the
     * range
     */
    protected Predicate<Program> isProgramDuring(final Range<Long> validRange) {
        return new Predicate<Program>() {
            @Override
            public boolean apply(@Nullable Program program) {
                return validRange.contains(program.getStartTimeUtcMillis())
                        || validRange.contains(program.getEndTimeUtcMillis());
            }
        };
    }

    @Override
    public SeriesInfo getSeriesInfo(@NonNull String seriesId) {
        return null;
    }
}
