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
package com.android.tradefed.invoker.shard;

import com.android.annotations.VisibleForTesting;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.IRescheduler;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IInvocationContextReceiver;
import com.android.tradefed.testtype.IMultiDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.IRuntimeHintProvider;
import com.android.tradefed.testtype.IShardableTest;
import com.android.tradefed.testtype.IStrictShardableTest;
import com.android.tradefed.testtype.suite.ITestSuite;
import com.android.tradefed.testtype.suite.ModuleMerger;
import com.android.tradefed.util.TimeUtil;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.List;

/** Sharding strategy to create strict shards that do not report together, */
public class StrictShardHelper extends ShardHelper {

    /** {@inheritDoc} */
    @Override
    public boolean shardConfig(
            IConfiguration config, IInvocationContext context, IRescheduler rescheduler) {
        Integer shardCount = config.getCommandOptions().getShardCount();
        Integer shardIndex = config.getCommandOptions().getShardIndex();

        if (shardIndex == null) {
            return super.shardConfig(config, context, rescheduler);
        }
        if (shardCount == null) {
            throw new RuntimeException("shard-count is null while shard-index is " + shardIndex);
        }

        // Split tests in place, without actually sharding.
        if (!config.getCommandOptions().shouldUseTfSharding()) {
            // TODO: remove when IStrictShardableTest is removed.
            updateConfigIfSharded(config, shardCount, shardIndex);
        } else {
            List<IRemoteTest> listAllTests = getAllTests(config, shardCount, context);
            // We cannot shuffle to get better average results
            normalizeDistribution(listAllTests, shardCount);
            List<IRemoteTest> splitList;
            if (shardCount == 1) {
                // not sharded
                splitList = listAllTests;
            } else {
                splitList = splitTests(listAllTests, shardCount).get(shardIndex);
            }
            aggregateSuiteModules(splitList);
            config.setTests(splitList);
        }
        return false;
    }

    // TODO: Retire IStrictShardableTest for IShardableTest and have TF balance the list of tests.
    private void updateConfigIfSharded(IConfiguration config, int shardCount, int shardIndex) {
        List<IRemoteTest> testShards = new ArrayList<>();
        for (IRemoteTest test : config.getTests()) {
            if (!(test instanceof IStrictShardableTest)) {
                CLog.w(
                        "%s is not shardable; the whole test will run in shard 0",
                        test.getClass().getName());
                if (shardIndex == 0) {
                    testShards.add(test);
                }
                continue;
            }
            IRemoteTest testShard =
                    ((IStrictShardableTest) test).getTestShard(shardCount, shardIndex);
            testShards.add(testShard);
        }
        config.setTests(testShards);
    }

    /**
     * Helper to return the full list of {@link IRemoteTest} based on {@link IShardableTest} split.
     *
     * @param config the {@link IConfiguration} describing the invocation.
     * @param shardCount the shard count hint to be provided to some tests.
     * @param context the {@link IInvocationContext} of the parent invocation.
     * @return the list of all {@link IRemoteTest}.
     */
    private List<IRemoteTest> getAllTests(
            IConfiguration config, Integer shardCount, IInvocationContext context) {
        List<IRemoteTest> allTests = new ArrayList<>();
        for (IRemoteTest test : config.getTests()) {
            if (test instanceof IShardableTest) {
                // Inject current information to help with sharding
                if (test instanceof IBuildReceiver) {
                    ((IBuildReceiver) test).setBuild(context.getBuildInfos().get(0));
                }
                if (test instanceof IDeviceTest) {
                    ((IDeviceTest) test).setDevice(context.getDevices().get(0));
                }
                if (test instanceof IMultiDeviceTest) {
                    ((IMultiDeviceTest) test).setDeviceInfos(context.getDeviceBuildMap());
                }
                if (test instanceof IInvocationContextReceiver) {
                    ((IInvocationContextReceiver) test).setInvocationContext(context);
                }

                // Handling of the ITestSuite is a special case, we do not allow pool of tests
                // since each shard needs to be independent.
                if (test instanceof ITestSuite) {
                    ((ITestSuite) test).setShouldMakeDynamicModule(false);
                }

                Collection<IRemoteTest> subTests = ((IShardableTest) test).split(shardCount);
                if (subTests == null) {
                    // test did not shard so we add it as is.
                    allTests.add(test);
                } else {
                    allTests.addAll(subTests);
                }
            } else {
                // if test is not shardable we add it as is.
                allTests.add(test);
            }
        }
        return allTests;
    }

    /**
     * Split the list of tests to run however the implementation see fit. Sharding needs to be
     * consistent. It is acceptable to return an empty list if no tests can be run in the shard.
     *
     * <p>Implement this in order to provide a test suite specific sharding. The default
     * implementation attempts to balance the number of IRemoteTest per shards as much as possible
     * as a first step, then use a minor criteria or run-hint to adjust the lists a bit more.
     *
     * @param fullList the initial full list of {@link IRemoteTest} containing all the tests that
     *     need to run.
     * @param shardCount the total number of shard that need to run.
     * @return a list of list {@link IRemoteTest}s that have been assigned to each shard. The list
     *     size will be the shardCount.
     */
    @VisibleForTesting
    protected List<List<IRemoteTest>> splitTests(List<IRemoteTest> fullList, int shardCount) {
        List<List<IRemoteTest>> shards = new ArrayList<>();
        // We are using Match.ceil to avoid the last shard having too much extra.
        int numPerShard = (int) Math.ceil(fullList.size() / (float) shardCount);

        boolean needsCorrection = false;
        float correctionRatio = 0f;
        if (fullList.size() > shardCount) {
            // In some cases because of the Math.ceil, some combination might run out of tests
            // before the last shard, in that case we populate a correction to rebalance the tests.
            needsCorrection = (numPerShard * (shardCount - 1)) > fullList.size();
            correctionRatio = numPerShard - ((fullList.size() / (float) shardCount));
        }
        // Recalculate the number of tests per shard with the correction taken into account.
        numPerShard = (int) Math.floor(numPerShard - correctionRatio);
        // Based of the parameters, distribute the tests accross shards.
        shards = balancedDistrib(fullList, shardCount, numPerShard, needsCorrection);
        // Do last minute rebalancing
        topBottom(shards, shardCount);
        return shards;
    }

    private List<List<IRemoteTest>> balancedDistrib(
            List<IRemoteTest> fullList, int shardCount, int numPerShard, boolean needsCorrection) {
        List<List<IRemoteTest>> shards = new ArrayList<>();
        List<IRemoteTest> correctionList = new ArrayList<>();
        int correctionSize = 0;

        // Generate all the shards
        for (int i = 0; i < shardCount; i++) {
            List<IRemoteTest> shardList;
            if (i >= fullList.size()) {
                // Return empty list when we don't have enough tests for all the shards.
                shardList = new ArrayList<IRemoteTest>();
                shards.add(shardList);
                continue;
            }

            if (i == shardCount - 1) {
                // last shard take everything remaining except the correction:
                if (needsCorrection) {
                    // We omit the size of the correction needed.
                    correctionSize = fullList.size() - (numPerShard + (i * numPerShard));
                    correctionList =
                            fullList.subList(fullList.size() - correctionSize, fullList.size());
                }
                shardList = fullList.subList(i * numPerShard, fullList.size() - correctionSize);
                shards.add(new ArrayList<>(shardList));
                continue;
            }
            shardList = fullList.subList(i * numPerShard, numPerShard + (i * numPerShard));
            shards.add(new ArrayList<>(shardList));
        }

        // If we have correction omitted tests, disperse them on each shard, at this point the
        // number of tests in correction is ensured to be bellow the number of shards.
        for (int i = 0; i < shardCount; i++) {
            if (i < correctionList.size()) {
                shards.get(i).add(correctionList.get(i));
            } else {
                break;
            }
        }
        return shards;
    }

    /**
     * Move around predictably the tests in order to have a better uniformization of the tests in
     * each shard.
     */
    private void normalizeDistribution(List<IRemoteTest> listAllTests, int shardCount) {
        final int numRound = shardCount;
        final int distance = shardCount - 1;
        for (int i = 0; i < numRound; i++) {
            for (int j = 0; j < listAllTests.size(); j = j + distance) {
                // Push the test at the end
                IRemoteTest push = listAllTests.remove(j);
                listAllTests.add(push);
            }
        }
    }

    /**
     * Special handling for suite from {@link ITestSuite}. We aggregate the tests in the same shard
     * in order to optimize target_preparation step.
     *
     * @param tests the {@link List} of {@link IRemoteTest} for that shard.
     */
    private void aggregateSuiteModules(List<IRemoteTest> tests) {
        List<IRemoteTest> dupList = new ArrayList<>(tests);
        for (int i = 0; i < dupList.size(); i++) {
            if (dupList.get(i) instanceof ITestSuite) {
                // We iterate the other tests to see if we can find another from the same module.
                for (int j = i + 1; j < dupList.size(); j++) {
                    // If the test was not already merged
                    if (tests.contains(dupList.get(j))) {
                        if (dupList.get(j) instanceof ITestSuite) {
                            if (ModuleMerger.arePartOfSameSuite(
                                    (ITestSuite) dupList.get(i), (ITestSuite) dupList.get(j))) {
                                ModuleMerger.mergeSplittedITestSuite(
                                        (ITestSuite) dupList.get(i), (ITestSuite) dupList.get(j));
                                tests.remove(dupList.get(j));
                            }
                        }
                    }
                }
            }
        }
    }

    private void topBottom(List<List<IRemoteTest>> allShards, int shardCount) {
        // We only attempt this when the number of shard is pretty high
        if (shardCount < 4) {
            return;
        }
        // Generate approximate RuntimeHint for each shard
        int index = 0;
        List<SortShardObj> shardTimes = new ArrayList<>();
        CLog.e("============================");
        for (List<IRemoteTest> shard : allShards) {
            long aggTime = 0l;
            CLog.e("++++++++++++++++++ SHARD %s +++++++++++++++", index);
            for (IRemoteTest test : shard) {
                if (test instanceof IRuntimeHintProvider) {
                    aggTime += ((IRuntimeHintProvider) test).getRuntimeHint();
                }
            }
            CLog.e("Shard %s approximate time: %s", index, TimeUtil.formatElapsedTime(aggTime));
            shardTimes.add(new SortShardObj(index, aggTime));
            index++;
            CLog.e("+++++++++++++++++++++++++++++++++++++++++++");
        }
        CLog.e("============================");

        Collections.sort(shardTimes);
        if ((shardTimes.get(0).mAggTime - shardTimes.get(shardTimes.size() - 1).mAggTime)
                < 60 * 60 * 1000l) {
            return;
        }

        // take 30% top shard (10 shard = top 3 shards)
        for (int i = 0; i < (shardCount * 0.3); i++) {
            CLog.e(
                    "Top shard %s is index %s with %s",
                    i,
                    shardTimes.get(i).mIndex,
                    TimeUtil.formatElapsedTime(shardTimes.get(i).mAggTime));
            int give = shardTimes.get(i).mIndex;
            int receive = shardTimes.get(shardTimes.size() - 1 - i).mIndex;
            CLog.e("Giving from shard %s to shard %s", give, receive);
            for (int j = 0; j < (allShards.get(give).size() * (0.2f / (i + 1))); j++) {
                IRemoteTest givetest = allShards.get(give).remove(0);
                allShards.get(receive).add(givetest);
            }
        }
    }

    /** Object holder for shard, their index and their aggregated execution time. */
    private class SortShardObj implements Comparable<SortShardObj> {
        public final int mIndex;
        public final Long mAggTime;

        public SortShardObj(int index, long aggTime) {
            mIndex = index;
            mAggTime = aggTime;
        }

        @Override
        public int compareTo(SortShardObj obj) {
            return obj.mAggTime.compareTo(mAggTime);
        }
    }
}
