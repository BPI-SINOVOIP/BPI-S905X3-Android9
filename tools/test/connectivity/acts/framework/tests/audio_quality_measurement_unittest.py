#!/usr/bin/env python3
#
#   Copyright 2017 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import logging
import math
import numpy
import unittest

import acts.test_utils.audio_analysis_lib.audio_data as audio_data
import acts.test_utils.audio_analysis_lib.audio_analysis as audio_analysis
import acts.test_utils.audio_analysis_lib.audio_quality_measurement as \
    audio_quality_measurement


class NoiseLevelTest(unittest.TestCase):
    def setUp(self):
        """Uses the same seed to generate noise for each test."""
        numpy.random.seed(0)

    def testNoiseLevel(self):
        # Generates the standard sin wave with standard_noise portion of noise.
        rate = 48000
        length_in_secs = 2
        frequency = 440
        amplitude = 1
        standard_noise = 0.05

        wave = []
        for index in range(0, rate * length_in_secs):
            phase = 2.0 * math.pi * frequency * float(index) / float(rate)
            sine_wave = math.sin(phase)
            noise = standard_noise * numpy.random.standard_normal()
            wave.append(float(amplitude) * (sine_wave + noise))

        # Calculates the average value after applying teager operator.
        teager_value_of_wave, length = 0, len(wave)
        for i in range(1, length - 1):
            ith_teager_value = abs(wave[i] * wave[i] - wave[i - 1] * wave[i +
                                                                          1])
            ith_teager_value *= max(1, abs(wave[i]))
            teager_value_of_wave += ith_teager_value
        teager_value_of_wave /= float(length * (amplitude**2))

        noise = audio_quality_measurement.noise_level(
            amplitude, frequency, rate, teager_value_of_wave)

        self.assertTrue(abs(noise - standard_noise) < 0.01)


class ErrorTest(unittest.TestCase):
    def testError(self):
        value1 = [0.2, 0.4, 0.1, 0.01, 0.01, 0.01]
        value2 = [0.3, 0.3, 0.08, 0.0095, 0.0098, 0.0099]
        error = [0.5, 0.25, 0.2, 0.05, 0.02, 0.01]
        for i in range(len(value1)):
            ret = audio_quality_measurement.error(value1[i], value2[i])
            self.assertTrue(abs(ret - error[i]) < 0.001)


class QualityMeasurementTest(unittest.TestCase):
    def setUp(self):
        """Creates a test signal of sine wave."""
        numpy.random.seed(0)

        self.rate = 48000
        self.freq = 440
        self.amplitude = 1
        length_in_secs = 2
        self.samples = length_in_secs * self.rate
        self.y = []
        for index in range(self.samples):
            phase = 2.0 * math.pi * self.freq * float(index) / float(self.rate)
            sine_wave = math.sin(phase)
            self.y.append(float(self.amplitude) * sine_wave)

    def add_noise(self):
        """Adds noise to the test signal."""
        noise_amplitude = 0.01 * self.amplitude
        for index in range(self.samples):
            noise = noise_amplitude * numpy.random.standard_normal()
            self.y[index] += noise

    def generate_delay(self):
        """Generates some delays during playing."""
        self.delay_start_time = [0.200, 0.375, 0.513, 0.814, 1.000, 1.300]
        self.delay_end_time = [0.201, 0.377, 0.516, 0.824, 1.100, 1.600]

        for i in range(len(self.delay_start_time)):
            start_index = int(self.delay_start_time[i] * self.rate)
            end_index = int(self.delay_end_time[i] * self.rate)
            for j in range(start_index, end_index):
                self.y[j] = 0

    def generate_artifacts_before_playback(self):
        """Generates artifacts before playing."""
        silence_before_playback_end_time = 0.2
        end_index = int(silence_before_playback_end_time * self.rate)
        for i in range(0, end_index):
            self.y[i] = 0
        noise_start_index = int(0.1 * self.rate)
        noise_end_index = int(0.1005 * self.rate)
        for i in range(noise_start_index, noise_end_index):
            self.y[i] = 3 * self.amplitude

    def generate_artifacts_after_playback(self):
        """Generates artifacts after playing."""
        silence_after_playback_start_time = int(1.9 * self.rate)
        noise_start_index = int(1.95 * self.rate)
        noise_end_index = int((1.95 + 0.02) * self.rate)

        for i in range(silence_after_playback_start_time, self.samples):
            self.y[i] = 0
        for i in range(noise_start_index, noise_end_index):
            self.y[i] = self.amplitude

    def generate_burst_during_playback(self):
        """Generates bursts during playing."""
        self.burst_start_time = [0.300, 0.475, 0.613, 0.814, 1.300]
        self.burst_end_time = [0.301, 0.476, 0.614, 0.815, 1.301]

        for i in range(len(self.burst_start_time)):
            start_index = int(self.burst_start_time[i] * self.rate)
            end_index = int(self.burst_end_time[i] * self.rate)
            for j in range(start_index, end_index):
                self.y[j] = self.amplitude * (3 + numpy.random.uniform(-1, 1))

    def generate_volume_changing(self):
        "Generates volume changing during playing."
        start_time = [0.300, 1.400]
        end_time = [0.600, 1.700]
        for i in range(len(start_time)):
            start_index = int(start_time[i] * self.rate)
            end_index = int(end_time[i] * self.rate)
            for j in range(start_index, end_index):
                self.y[j] *= 1.4
        self.volume_changing = [+1, -1, +1, -1]
        self.volume_changing_time = [0.3, 0.6, 1.4, 1.7]

    def testGoodSignal(self):
        """Sine wave signal with no noise or artifacts."""
        result = audio_quality_measurement.quality_measurement(self.y,
                                                               self.rate)
        self.assertTrue(len(result['artifacts']['noise_before_playback']) == 0)
        self.assertTrue(len(result['artifacts']['noise_after_playback']) == 0)
        self.assertTrue(len(result['artifacts']['delay_during_playback']) == 0)
        self.assertTrue(len(result['artifacts']['burst_during_playback']) == 0)
        self.assertTrue(len(result['volume_changes']) == 0)
        self.assertTrue(result['equivalent_noise_level'] < 0.005)

    def testGoodSignalNoise(self):
        """Sine wave signal with noise."""
        self.add_noise()
        result = audio_quality_measurement.quality_measurement(self.y,
                                                               self.rate)
        self.assertTrue(len(result['artifacts']['noise_before_playback']) == 0)
        self.assertTrue(len(result['artifacts']['noise_after_playback']) == 0)
        self.assertTrue(len(result['artifacts']['delay_during_playback']) == 0)
        self.assertTrue(len(result['artifacts']['burst_during_playback']) == 0)
        self.assertTrue(len(result['volume_changes']) == 0)
        self.assertTrue(0.009 < result['equivalent_noise_level'] and
                        result['equivalent_noise_level'] < 0.011)

    def testDelay(self):
        """Sine wave with delay during playing."""
        self.generate_delay()
        result = audio_quality_measurement.quality_measurement(self.y,
                                                               self.rate)
        self.assertTrue(len(result['artifacts']['noise_before_playback']) == 0)
        self.assertTrue(len(result['artifacts']['noise_after_playback']) == 0)
        self.assertTrue(
            len(result['volume_changes']) == 2 * len(self.delay_start_time))
        self.assertTrue(result['equivalent_noise_level'] < 0.005)

        self.assertTrue(
            len(result['artifacts']['delay_during_playback']) ==
            len(self.delay_start_time))
        for i in range(len(result['artifacts']['delay_during_playback'])):
            delta = abs(result['artifacts']['delay_during_playback'][i][0] -
                        self.delay_start_time[i])
            self.assertTrue(delta < 0.001)
            duration = self.delay_end_time[i] - self.delay_start_time[i]
            delta = abs(result['artifacts']['delay_during_playback'][i][1] -
                        duration)
            self.assertTrue(delta < 0.001)

    def testArtifactsBeforePlayback(self):
        """Sine wave with artifacts before playback."""
        self.generate_artifacts_before_playback()
        result = audio_quality_measurement.quality_measurement(self.y,
                                                               self.rate)
        self.assertTrue(len(result['artifacts']['noise_before_playback']) == 1)
        delta = abs(result['artifacts']['noise_before_playback'][0][0] - 0.1)
        self.assertTrue(delta < 0.01)
        delta = abs(result['artifacts']['noise_before_playback'][0][1] - 0.005)
        self.assertTrue(delta < 0.004)
        self.assertTrue(len(result['artifacts']['noise_after_playback']) == 0)
        self.assertTrue(len(result['artifacts']['delay_during_playback']) == 0)
        self.assertTrue(len(result['artifacts']['burst_during_playback']) == 0)
        self.assertTrue(len(result['volume_changes']) == 0)
        self.assertTrue(result['equivalent_noise_level'] < 0.005)

    def testArtifactsAfterPlayback(self):
        """Sine wave with artifacts after playback."""
        self.generate_artifacts_after_playback()
        result = audio_quality_measurement.quality_measurement(self.y,
                                                               self.rate)
        self.assertTrue(len(result['artifacts']['noise_before_playback']) == 0)
        self.assertTrue(len(result['artifacts']['noise_after_playback']) == 1)
        delta = abs(result['artifacts']['noise_after_playback'][0][0] - 1.95)
        self.assertTrue(delta < 0.01)
        delta = abs(result['artifacts']['noise_after_playback'][0][1] - 0.02)
        self.assertTrue(delta < 0.001)
        self.assertTrue(len(result['artifacts']['delay_during_playback']) == 0)
        self.assertTrue(len(result['artifacts']['burst_during_playback']) == 0)
        self.assertTrue(len(result['volume_changes']) == 0)
        self.assertTrue(result['equivalent_noise_level'] < 0.005)

    def testBurstDuringPlayback(self):
        """Sine wave with burst during playback."""
        self.generate_burst_during_playback()
        result = audio_quality_measurement.quality_measurement(self.y,
                                                               self.rate)
        self.assertTrue(len(result['artifacts']['noise_before_playback']) == 0)
        self.assertTrue(len(result['artifacts']['noise_after_playback']) == 0)
        self.assertTrue(len(result['artifacts']['delay_during_playback']) == 0)
        self.assertTrue(len(result['artifacts']['burst_during_playback']) == 5)
        self.assertTrue(len(result['volume_changes']) == 10)
        self.assertTrue(result['equivalent_noise_level'] > 0.02)
        for i in range(len(result['artifacts']['burst_during_playback'])):
            delta = abs(self.burst_start_time[i] - result['artifacts'][
                'burst_during_playback'][i])
            self.assertTrue(delta < 0.002)

    def testVolumeChanging(self):
        """Sine wave with volume changing during playback."""
        self.generate_volume_changing()
        result = audio_quality_measurement.quality_measurement(self.y,
                                                               self.rate)
        self.assertTrue(len(result['artifacts']['noise_before_playback']) == 0)
        self.assertTrue(len(result['artifacts']['noise_after_playback']) == 0)
        self.assertTrue(len(result['artifacts']['delay_during_playback']) == 0)
        self.assertTrue(len(result['artifacts']['burst_during_playback']) == 0)
        self.assertTrue(result['equivalent_noise_level'] < 0.005)
        self.assertTrue(
            len(result['volume_changes']) == len(self.volume_changing))
        for i in range(len(self.volume_changing)):
            self.assertTrue(
                abs(self.volume_changing_time[i] - result['volume_changes'][i][
                    0]) < 0.01)
            self.assertTrue(
                self.volume_changing[i] == result['volume_changes'][i][1])


if __name__ == '__main__':
    unittest.main()
