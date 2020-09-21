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

#define LOG_TAG "GnssHalTestCases"

#include <gnss_hal_test.h>

#include <VtsHalHidlTargetTestBase.h>

#include <android/hardware/gnss/1.1/IGnssConfiguration.h>

using android::hardware::hidl_vec;

using android::hardware::gnss::V1_0::GnssConstellationType;
using android::hardware::gnss::V1_0::GnssLocation;
using android::hardware::gnss::V1_0::IGnssDebug;
using android::hardware::gnss::V1_1::IGnssConfiguration;
using android::hardware::gnss::V1_1::IGnssMeasurement;

/*
 * SetupTeardownCreateCleanup:
 * Requests the gnss HAL then calls cleanup
 *
 * Empty test fixture to verify basic Setup & Teardown
 */
TEST_F(GnssHalTest, SetupTeardownCreateCleanup) {}

/*
 * TestGnssMeasurementCallback:
 * Gets the GnssMeasurementExtension and verify that it returns an actual extension.
 */
TEST_F(GnssHalTest, TestGnssMeasurementCallback) {
    auto gnssMeasurement = gnss_hal_->getExtensionGnssMeasurement_1_1();
    ASSERT_TRUE(gnssMeasurement.isOk());
    if (last_capabilities_ & IGnssCallback::Capabilities::MEASUREMENTS) {
        sp<IGnssMeasurement> iGnssMeas = gnssMeasurement;
        EXPECT_NE(iGnssMeas, nullptr);
    }
}

/*
 * GetLocationLowPower:
 * Turns on location, waits for at least 5 locations allowing max of LOCATION_TIMEOUT_SUBSEQUENT_SEC
 * between one location and the next. Also ensure that MIN_INTERVAL_MSEC is respected by waiting
 * NO_LOCATION_PERIOD_SEC and verfiy that no location is received. Also perform validity checks on
 * each received location.
 */
TEST_F(GnssHalTest, GetLocationLowPower) {
    const int kMinIntervalMsec = 5000;
    const int kLocationTimeoutSubsequentSec = (kMinIntervalMsec / 1000) + 1;
    const int kNoLocationPeriodSec = 2;
    const int kLocationsToCheck = 5;
    const bool kLowPowerMode = true;

    SetPositionMode(kMinIntervalMsec, kLowPowerMode);

    EXPECT_TRUE(StartAndGetSingleLocation());

    for (int i = 1; i < kLocationsToCheck; i++) {
        // Verify that kMinIntervalMsec is respected by waiting kNoLocationPeriodSec and
        // ensure that no location is received yet
        wait(kNoLocationPeriodSec);
        EXPECT_EQ(location_called_count_, i);
        EXPECT_EQ(std::cv_status::no_timeout,
                  wait(kLocationTimeoutSubsequentSec - kNoLocationPeriodSec));
        EXPECT_EQ(location_called_count_, i + 1);
        CheckLocation(last_location_, true);
    }

    StopAndClearLocations();
}

/*
 * FindStrongFrequentNonGpsSource:
 *
 * Search through a GnssSvStatus list for the strongest non-GPS satellite observed enough times
 *
 * returns the strongest source,
 *         or a source with constellation == UNKNOWN if none are found sufficient times
 */

IGnssConfiguration::BlacklistedSource FindStrongFrequentNonGpsSource(
    const list<IGnssCallback::GnssSvStatus> list_gnss_sv_status, const int min_observations) {
    struct ComparableBlacklistedSource {
        IGnssConfiguration::BlacklistedSource id;

        bool operator<(const ComparableBlacklistedSource& compare) const {
            return ((id.svid < compare.id.svid) || ((id.svid == compare.id.svid) &&
                                                    (id.constellation < compare.id.constellation)));
        }
    };

    struct SignalCounts {
        int observations;
        float max_cn0_dbhz;
    };

    std::map<ComparableBlacklistedSource, SignalCounts> mapSignals;

    for (const auto& gnss_sv_status : list_gnss_sv_status) {
        for (uint32_t iSv = 0; iSv < gnss_sv_status.numSvs; iSv++) {
            const auto& gnss_sv = gnss_sv_status.gnssSvList[iSv];
            if ((gnss_sv.svFlag & IGnssCallback::GnssSvFlags::USED_IN_FIX) &&
                (gnss_sv.constellation != GnssConstellationType::GPS)) {
                ComparableBlacklistedSource source;
                source.id.svid = gnss_sv.svid;
                source.id.constellation = gnss_sv.constellation;

                const auto& itSignal = mapSignals.find(source);
                if (itSignal == mapSignals.end()) {
                    SignalCounts counts;
                    counts.observations = 1;
                    counts.max_cn0_dbhz = gnss_sv.cN0Dbhz;
                    mapSignals.insert(
                        std::pair<ComparableBlacklistedSource, SignalCounts>(source, counts));
                } else {
                    itSignal->second.observations++;
                    if (itSignal->second.max_cn0_dbhz < gnss_sv.cN0Dbhz) {
                        itSignal->second.max_cn0_dbhz = gnss_sv.cN0Dbhz;
                    }
                }
            }
        }
    }

    float max_cn0_dbhz_with_sufficient_count = 0.;
    int total_observation_count = 0;
    int blacklisted_source_count_observation = 0;

    ComparableBlacklistedSource source_to_blacklist;  // initializes to zero = UNKNOWN constellation
    for (auto const& pairSignal : mapSignals) {
        total_observation_count += pairSignal.second.observations;
        if ((pairSignal.second.observations >= min_observations) &&
            (pairSignal.second.max_cn0_dbhz > max_cn0_dbhz_with_sufficient_count)) {
            source_to_blacklist = pairSignal.first;
            blacklisted_source_count_observation = pairSignal.second.observations;
            max_cn0_dbhz_with_sufficient_count = pairSignal.second.max_cn0_dbhz;
        }
    }
    ALOGD(
        "Among %d observations, chose svid %d, constellation %d, "
        "with %d observations at %.1f max CNo",
        total_observation_count, source_to_blacklist.id.svid,
        (int)source_to_blacklist.id.constellation, blacklisted_source_count_observation,
        max_cn0_dbhz_with_sufficient_count);

    return source_to_blacklist.id;
}

/*
 * BlacklistIndividualSatellites:
 *
 * 1) Turns on location, waits for 3 locations, ensuring they are valid, and checks corresponding
 * GnssStatus for common satellites (strongest and one other.)
 * 2a & b) Turns off location, and blacklists common satellites.
 * 3) Restart location, wait for 3 locations, ensuring they are valid, and checks corresponding
 * GnssStatus does not use those satellites.
 * 4a & b) Turns off location, and send in empty blacklist.
 * 5) Restart location, wait for 3 locations, ensuring they are valid, and checks corresponding
 * GnssStatus does re-use at least the previously strongest satellite
 */
TEST_F(GnssHalTest, BlacklistIndividualSatellites) {
    const int kLocationsToAwait = 3;

    StartAndCheckLocations(kLocationsToAwait);

    EXPECT_GE((int)list_gnss_sv_status_.size(), kLocationsToAwait);
    ALOGD("Observed %d GnssSvStatus, while awaiting %d Locations", (int)list_gnss_sv_status_.size(),
          kLocationsToAwait);

    /*
     * Identify strongest SV seen at least kLocationsToAwait -1 times
     * Why -1?  To avoid test flakiness in case of (plausible) slight flakiness in strongest signal
     * observability (one epoch RF null)
     */

    IGnssConfiguration::BlacklistedSource source_to_blacklist =
        FindStrongFrequentNonGpsSource(list_gnss_sv_status_, kLocationsToAwait - 1);

    if (source_to_blacklist.constellation == GnssConstellationType::UNKNOWN) {
        // Cannot find a non-GPS satellite. Let the test pass.
        return;
    }

    // Stop locations, blacklist the common SV
    StopAndClearLocations();

    auto gnss_configuration_hal_return = gnss_hal_->getExtensionGnssConfiguration_1_1();
    ASSERT_TRUE(gnss_configuration_hal_return.isOk());
    sp<IGnssConfiguration> gnss_configuration_hal = gnss_configuration_hal_return;
    ASSERT_NE(gnss_configuration_hal, nullptr);

    hidl_vec<IGnssConfiguration::BlacklistedSource> sources;
    sources.resize(1);
    sources[0] = source_to_blacklist;

    auto result = gnss_configuration_hal->setBlacklist(sources);
    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);

    // retry and ensure satellite not used
    list_gnss_sv_status_.clear();

    location_called_count_ = 0;
    StartAndCheckLocations(kLocationsToAwait);

    EXPECT_GE((int)list_gnss_sv_status_.size(), kLocationsToAwait);
    ALOGD("Observed %d GnssSvStatus, while awaiting %d Locations", (int)list_gnss_sv_status_.size(),
          kLocationsToAwait);
    for (const auto& gnss_sv_status : list_gnss_sv_status_) {
        for (uint32_t iSv = 0; iSv < gnss_sv_status.numSvs; iSv++) {
            const auto& gnss_sv = gnss_sv_status.gnssSvList[iSv];
            EXPECT_FALSE((gnss_sv.svid == source_to_blacklist.svid) &&
                         (gnss_sv.constellation == source_to_blacklist.constellation) &&
                         (gnss_sv.svFlag & IGnssCallback::GnssSvFlags::USED_IN_FIX));
        }
    }

    // clear blacklist and restart - this time updating the blacklist while location is still on
    sources.resize(0);

    result = gnss_configuration_hal->setBlacklist(sources);
    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);

    location_called_count_ = 0;
    StopAndClearLocations();
    list_gnss_sv_status_.clear();

    StartAndCheckLocations(kLocationsToAwait);

    EXPECT_GE((int)list_gnss_sv_status_.size(), kLocationsToAwait);
    ALOGD("Observed %d GnssSvStatus, while awaiting %d Locations", (int)list_gnss_sv_status_.size(),
          kLocationsToAwait);

    bool strongest_sv_is_reobserved = false;
    for (const auto& gnss_sv_status : list_gnss_sv_status_) {
        for (uint32_t iSv = 0; iSv < gnss_sv_status.numSvs; iSv++) {
            const auto& gnss_sv = gnss_sv_status.gnssSvList[iSv];
            if ((gnss_sv.svid == source_to_blacklist.svid) &&
                (gnss_sv.constellation == source_to_blacklist.constellation) &&
                (gnss_sv.svFlag & IGnssCallback::GnssSvFlags::USED_IN_FIX)) {
                strongest_sv_is_reobserved = true;
                break;
            }
        }
        if (strongest_sv_is_reobserved) break;
    }
    EXPECT_TRUE(strongest_sv_is_reobserved);
    StopAndClearLocations();
}

/*
 * BlacklistConstellation:
 *
 * 1) Turns on location, waits for 3 locations, ensuring they are valid, and checks corresponding
 * GnssStatus for any non-GPS constellations.
 * 2a & b) Turns off location, and blacklist first non-GPS constellations.
 * 3) Restart location, wait for 3 locations, ensuring they are valid, and checks corresponding
 * GnssStatus does not use any constellation but GPS.
 * 4a & b) Clean up by turning off location, and send in empty blacklist.
 */
TEST_F(GnssHalTest, BlacklistConstellation) {
    const int kLocationsToAwait = 3;

    StartAndCheckLocations(kLocationsToAwait);

    EXPECT_GE((int)list_gnss_sv_status_.size(), kLocationsToAwait);
    ALOGD("Observed %d GnssSvStatus, while awaiting %d Locations", (int)list_gnss_sv_status_.size(),
          kLocationsToAwait);

    // Find first non-GPS constellation to blacklist
    GnssConstellationType constellation_to_blacklist = GnssConstellationType::UNKNOWN;
    for (const auto& gnss_sv_status : list_gnss_sv_status_) {
        for (uint32_t iSv = 0; iSv < gnss_sv_status.numSvs; iSv++) {
            const auto& gnss_sv = gnss_sv_status.gnssSvList[iSv];
            if ((gnss_sv.svFlag & IGnssCallback::GnssSvFlags::USED_IN_FIX) &&
                (gnss_sv.constellation != GnssConstellationType::UNKNOWN) &&
                (gnss_sv.constellation != GnssConstellationType::GPS)) {
                // found a non-GPS constellation
                constellation_to_blacklist = gnss_sv.constellation;
                break;
            }
        }
        if (constellation_to_blacklist != GnssConstellationType::UNKNOWN) {
            break;
        }
    }

    if (constellation_to_blacklist == GnssConstellationType::UNKNOWN) {
        ALOGI("No non-GPS constellations found, constellation blacklist test less effective.");
        // Proceed functionally to blacklist something.
        constellation_to_blacklist = GnssConstellationType::GLONASS;
    }
    IGnssConfiguration::BlacklistedSource source_to_blacklist;
    source_to_blacklist.constellation = constellation_to_blacklist;
    source_to_blacklist.svid = 0;  // documented wildcard for all satellites in this constellation

    auto gnss_configuration_hal_return = gnss_hal_->getExtensionGnssConfiguration_1_1();
    ASSERT_TRUE(gnss_configuration_hal_return.isOk());
    sp<IGnssConfiguration> gnss_configuration_hal = gnss_configuration_hal_return;
    ASSERT_NE(gnss_configuration_hal, nullptr);

    hidl_vec<IGnssConfiguration::BlacklistedSource> sources;
    sources.resize(1);
    sources[0] = source_to_blacklist;

    auto result = gnss_configuration_hal->setBlacklist(sources);
    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);

    // retry and ensure constellation not used
    list_gnss_sv_status_.clear();

    location_called_count_ = 0;
    StartAndCheckLocations(kLocationsToAwait);

    EXPECT_GE((int)list_gnss_sv_status_.size(), kLocationsToAwait);
    ALOGD("Observed %d GnssSvStatus, while awaiting %d Locations", (int)list_gnss_sv_status_.size(),
          kLocationsToAwait);
    for (const auto& gnss_sv_status : list_gnss_sv_status_) {
        for (uint32_t iSv = 0; iSv < gnss_sv_status.numSvs; iSv++) {
            const auto& gnss_sv = gnss_sv_status.gnssSvList[iSv];
            EXPECT_FALSE((gnss_sv.constellation == source_to_blacklist.constellation) &&
                         (gnss_sv.svFlag & IGnssCallback::GnssSvFlags::USED_IN_FIX));
        }
    }

    // clean up
    StopAndClearLocations();
    sources.resize(0);
    result = gnss_configuration_hal->setBlacklist(sources);
    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);
}

/*
 * InjectBestLocation
 *
 * Ensure successfully injecting a location.
 */
TEST_F(GnssHalTest, InjectBestLocation) {
    GnssLocation gnssLocation = {.gnssLocationFlags = 0,  // set below
                                 .latitudeDegrees = 43.0,
                                 .longitudeDegrees = -180,
                                 .altitudeMeters = 1000,
                                 .speedMetersPerSec = 0,
                                 .bearingDegrees = 0,
                                 .horizontalAccuracyMeters = 0.1,
                                 .verticalAccuracyMeters = 0.1,
                                 .speedAccuracyMetersPerSecond = 0.1,
                                 .bearingAccuracyDegrees = 0.1,
                                 .timestamp = 1534567890123L};
    gnssLocation.gnssLocationFlags |=
        GnssLocationFlags::HAS_LAT_LONG | GnssLocationFlags::HAS_ALTITUDE |
        GnssLocationFlags::HAS_SPEED | GnssLocationFlags::HAS_HORIZONTAL_ACCURACY |
        GnssLocationFlags::HAS_VERTICAL_ACCURACY | GnssLocationFlags::HAS_SPEED_ACCURACY |
        GnssLocationFlags::HAS_BEARING | GnssLocationFlags::HAS_BEARING_ACCURACY;

    CheckLocation(gnssLocation, true);

    auto result = gnss_hal_->injectBestLocation(gnssLocation);

    ASSERT_TRUE(result.isOk());
    EXPECT_TRUE(result);

    auto resultVoid = gnss_hal_->deleteAidingData(IGnss::GnssAidingData::DELETE_ALL);

    ASSERT_TRUE(resultVoid.isOk());
}

/*
 * GnssDebugValuesSanityTest:
 * Ensures that GnssDebug values make sense.
 */
TEST_F(GnssHalTest, GnssDebugValuesSanityTest) {
    auto gnssDebug = gnss_hal_->getExtensionGnssDebug();
    ASSERT_TRUE(gnssDebug.isOk());
    if (info_called_count_ > 0 && last_info_.yearOfHw >= 2017) {
        sp<IGnssDebug> iGnssDebug = gnssDebug;
        EXPECT_NE(iGnssDebug, nullptr);

        IGnssDebug::DebugData data;
        iGnssDebug->getDebugData(
            [&data](const IGnssDebug::DebugData& debugData) { data = debugData; });

        if (data.position.valid) {
            EXPECT_GE(data.position.latitudeDegrees, -90);
            EXPECT_LE(data.position.latitudeDegrees, 90);

            EXPECT_GE(data.position.longitudeDegrees, -180);
            EXPECT_LE(data.position.longitudeDegrees, 180);

            EXPECT_GE(data.position.altitudeMeters, -1000);  // Dead Sea: -414m
            EXPECT_LE(data.position.altitudeMeters, 20000);  // Mount Everest: 8850m

            EXPECT_GE(data.position.speedMetersPerSec, 0);
            EXPECT_LE(data.position.speedMetersPerSec, 600);

            EXPECT_GE(data.position.bearingDegrees, -360);
            EXPECT_LE(data.position.bearingDegrees, 360);

            EXPECT_GT(data.position.horizontalAccuracyMeters, 0);
            EXPECT_LE(data.position.horizontalAccuracyMeters, 20000000);

            EXPECT_GT(data.position.verticalAccuracyMeters, 0);
            EXPECT_LE(data.position.verticalAccuracyMeters, 20000);

            EXPECT_GT(data.position.speedAccuracyMetersPerSecond, 0);
            EXPECT_LE(data.position.speedAccuracyMetersPerSecond, 500);

            EXPECT_GT(data.position.bearingAccuracyDegrees, 0);
            EXPECT_LE(data.position.bearingAccuracyDegrees, 180);

            EXPECT_GE(data.position.ageSeconds, 0);
        }

        EXPECT_GE(data.time.timeEstimate, 1514764800000);  // Jan 01 2018 00:00:00

        EXPECT_GT(data.time.timeUncertaintyNs, 0);

        EXPECT_GT(data.time.frequencyUncertaintyNsPerSec, 0);
        EXPECT_LE(data.time.frequencyUncertaintyNsPerSec, 2.0e5);  // 200 ppm
    }
}
