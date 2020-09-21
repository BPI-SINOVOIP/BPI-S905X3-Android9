# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides utilities to detect some artifacts and measure the
    quality of audio."""

import logging
import math
import numpy

# Normal autotest environment.
try:
    import common
    from autotest_lib.client.cros.audio import audio_analysis
# Standalone execution without autotest environment.
except ImportError:
    import audio_analysis


# The input signal should be one sine wave with fixed frequency which
# can have silence before and/or after sine wave.
# For example:
#   silence      sine wave      silence
#  -----------|VVVVVVVVVVVVV|-----------
#     (a)           (b)           (c)
# This module detects these artifacts:
#   1. Detect noise in (a) and (c).
#   2. Detect delay in (b).
#   3. Detect burst in (b).
# Assume the transitions between (a)(b) and (b)(c) are smooth and
# amplitude increases/decreases linearly.
# This module will detect artifacts in the sine wave.
# This module also estimates the equivalent noise level by teager operator.
# This module also detects volume changes in the sine wave. However, volume
# changes may be affected by delay or burst.
# Some artifacts may cause each other.

# In this module, amplitude and frequency are derived from Hilbert transform.
# Both amplitude and frequency are a function of time.

# To detect each artifact, each point will be compared with
# average amplitude of its block. The block size will be 1.5 ms.
# Using average amplitude can mitigate the error caused by
# Hilbert transform and noise.
# In some case, for more accuracy, the block size may be modified
# to other values.
DEFAULT_BLOCK_SIZE_SECS = 0.0015

# If the difference between average frequency of this block and
# dominant frequency of full signal is less than 0.5 times of
# dominant frequency, this block is considered to be within the
# sine wave. In most cases, if there is no sine wave(only noise),
# average frequency will be much greater than 5 times of
# dominant frequency.
# Also, for delay during playback, the frequency will be about 0
# in perfect situation or much greater than 5 times of dominant
# frequency if it's noised.
DEFAULT_FREQUENCY_ERROR = 0.5

# If the amplitude of some sample is less than 0.6 times of the
# average amplitude of its left/right block, it will be considered
# as a delay during playing.
DEFAULT_DELAY_AMPLITUDE_THRESHOLD = 0.6

# If the average amplitude of the block before or after playing
# is more than 0.5 times to the average amplitude of the wave,
# it will be considered as a noise artifact.
DEFAULT_NOISE_AMPLITUDE_THRESHOLD = 0.5

# In the sine wave, if the amplitude is more than 1.4 times of
# its left side and its right side, it will be considered as
# a burst.
DEFAULT_BURST_AMPLITUDE_THRESHOLD = 1.4

# When detecting burst, if the amplitude is lower than 0.5 times
# average amplitude, we ignore it.
DEFAULT_BURST_TOO_SMALL = 0.5

# For a signal which is the combination of sine wave with fixed frequency f and
# amplitude 1 and standard noise with amplitude k, the average teager value is
# nearly linear to the noise level k.
# Given frequency f, we simulate a sine wave with default noise level and
# calculate its average teager value. Then, we can estimate the equivalent
# noise level of input signal by the average teager value of input signal.
DEFAULT_STANDARD_NOISE = 0.005

# For delay, burst, volume increasing/decreasing, if two delay(
# burst, volume increasing/decreasing) happen within
# DEFAULT_SAME_EVENT_SECS seconds, we consider they are the
# same event.
DEFAULT_SAME_EVENT_SECS = 0.001

# When detecting increasing/decreasing volume of signal, if the amplitude
# is lower than 0.1 times average amplitude, we ignore it.
DEFAULT_VOLUME_CHANGE_TOO_SMALL = 0.1

# If average amplitude of right block is less/more than average
# amplitude of left block times DEFAULT_VOLUME_CHANGE_AMPLITUDE, it will be
# considered as decreasing/increasing on volume.
DEFAULT_VOLUME_CHANGE_AMPLITUDE = 0.1

# If the increasing/decreasing volume event is too close to the start or the end
# of sine wave, we consider its volume change as part of rising/falling phase in
# the start/end.
NEAR_START_OR_END_SECS = 0.01

# After applying Hilbert transform, the resulting amplitude and frequency may be
# extremely large in the start and/or the end part. Thus, we will append zeros
# before and after the whole wave for 0.1 secs.
APPEND_ZEROS_SECS = 0.1

# If the noise event is too close to the start or the end of the data, we
# consider its noise as part of artifacts caused by edge effect of Hilbert
# transform.
# For example, originally, the data duration is 10 seconds.
# We append 0.1 seconds of zeros in the beginning and the end of the data, so
# the data becomes 10.2 seocnds long.
# Then, we apply Hilbert transform to 10.2 seconds of data.
# Near 0.1 seconds and 10.1 seconds, there will be edge effect of Hilbert
# transform. We do not want these be treated as noise.
# If NEAR_DATA_START_OR_END_SECS is set to 0.01, then the noise happened
# at [0, 0.11] and [10.09, 10.1] will be ignored.
NEAR_DATA_START_OR_END_SECS = 0.01

# If the noise event is too close to the start or the end of the sine wave in
# the data, we consider its noise as part of artifacts caused by edge effect of
# Hilbert transform.
# A |-------------|vvvvvvvvvvvvvvvvvvvvvvv|-------------|
# B |ooooooooo| d |                       | d |ooooooooo|
#
# A is full signal. It contains a sine wave and silence before and after sine
# wave.
# In B, |oooo| shows the parts that we are going to check for noise before/after
# sine wave. | d | is determined by NEAR_SINE_START_OR_END_SECS.
NEAR_SINE_START_OR_END_SECS = 0.01


class SineWaveNotFound(Exception):
    """Error when there's no sine wave found in the signal"""
    pass


def hilbert(x):
    """Hilbert transform copied from scipy.

    More information can be found here:
    http://docs.scipy.org/doc/scipy/reference/generated/scipy.signal.hilbert.html

    @param x: Real signal data to transform.

    @returns: Analytic signal of x, we can further extract amplitude and
              frequency from it.

    """
    x = numpy.asarray(x)
    if numpy.iscomplexobj(x):
        raise ValueError("x must be real.")
    axis = -1
    N = x.shape[axis]
    if N <= 0:
        raise ValueError("N must be positive.")

    Xf = numpy.fft.fft(x, N, axis=axis)
    h = numpy.zeros(N)
    if N % 2 == 0:
        h[0] = h[N // 2] = 1
        h[1:N // 2] = 2
    else:
        h[0] = 1
        h[1:(N + 1) // 2] = 2

    if len(x.shape) > 1:
        ind = [newaxis] * x.ndim
        ind[axis] = slice(None)
        h = h[ind]
    x = numpy.fft.ifft(Xf * h, axis=axis)
    return x


def noised_sine_wave(frequency, rate, noise_level):
    """Generates a sine wave of 2 second with specified noise level.

    @param frequency: Frequency of sine wave.
    @param rate: Sampling rate.
    @param noise_level: Required noise level.

    @returns: A sine wave with specified noise level.

    """
    wave = []
    for index in xrange(0, rate * 2):
        sample = 2.0 * math.pi * frequency * float(index) / float(rate)
        sine_wave = math.sin(sample)
        noise = noise_level * numpy.random.standard_normal()
        wave.append(sine_wave + noise)
    return wave


def average_teager_value(wave, amplitude):
    """Computes the normalized average teager value.

    After averaging the teager value, we will normalize the value by
    dividing square of amplitude.

    @param wave: Wave to apply teager operator.
    @param amplitude: Average amplitude of given wave.

    @returns: Average teager value.

    """
    teager_value, length = 0, len(wave)
    for i in range(1, length - 1):
        ith_teager_value = abs(wave[i] * wave[i] - wave[i - 1] * wave[i + 1])
        ith_teager_value *= max(1, abs(wave[i]))
        teager_value += ith_teager_value
    teager_value = (float(teager_value) / length) / (amplitude ** 2)
    return teager_value


def noise_level(amplitude, frequency, rate, teager_value_of_input):
    """Computes the noise level compared with standard_noise.

    For a signal which is the combination of sine wave with fixed frequency f
    and amplitude 1 and standard noise with amplitude k, the average teager
    value is nearly linear to the noise level k.
    Thus, we can compute the average teager value of a sine wave with
    standard_noise. Then, we can estimate the noise level of given input.

    @param amplitude: Amplitude of input audio.
    @param frequency: Dominant frequency of input audio.
    @param rate: Sampling rate.
    @param teager_value_of_input: Average teager value of input audio.

    @returns: A float value denotes the audio is equivalent to have
              how many times of noise compared with its amplitude.
              For example, 0.02 denotes that the wave has a noise which
              has standard distribution with standard deviation being
              0.02 times the amplitude of the wave.

    """
    standard_noise = DEFAULT_STANDARD_NOISE

    # Generates the standard sine wave with stdandard_noise level of noise.
    standard_wave = noised_sine_wave(frequency, rate, standard_noise)

    # Calculates the average teager value.
    teager_value_of_std_wave = average_teager_value(standard_wave, amplitude)

    return (teager_value_of_input / teager_value_of_std_wave) * standard_noise


def error(f1, f2):
    """Calculates the relative error between f1 and f2.

    @param f1: Exact value.
    @param f2: Test value.

    @returns: Relative error between f1 and f2.

    """
    return abs(float(f1) - float(f2)) / float(f1)


def hilbert_analysis(signal, rate, block_size):
    """Finds amplitude and frequency of each time of signal by Hilbert transform.

    @param signal: The wave to analyze.
    @param rate: Sampling rate.
    @param block_size: The size of block to transform.

    @returns: A tuple of list: (amplitude, frequency) composed of
              amplitude and frequency of each time.

    """
    # To apply Hilbert transform, the wave will be transformed
    # segment by segment. For each segment, its size will be
    # block_size and we will only take middle part of it.
    # Thus, each segment looks like: |-----|=====|=====|-----|.
    # "=...=" part will be taken while "-...-" part will be ignored.
    #
    # The whole size of taken part will be half of block_size
    # which will be hilbert_block.
    # The size of each ignored part will be half of hilbert_block
    # which will be half_hilbert_block.
    hilbert_block = block_size // 2
    half_hilbert_block = hilbert_block // 2
    # As mentioned above, for each block, we will only take middle
    # part of it. Thus, the whole transformation will be completed as:
    # |=====|=====|-----|           |-----|=====|=====|-----|
    #       |-----|=====|=====|-----|           |-----|=====|=====|
    #                   |-----|=====|=====|-----|
    # Specially, beginning and ending part may not have ignored part.
    length = len(signal)
    result = []
    for left_border in xrange(0, length, hilbert_block):
        right_border = min(length, left_border + hilbert_block)
        temp_left_border = max(0, left_border - half_hilbert_block)
        temp_right_border = min(length, right_border + half_hilbert_block)
        temp = hilbert(signal[temp_left_border:temp_right_border])
        for index in xrange(left_border, right_border):
            result.append(temp[index - temp_left_border])
    result = numpy.asarray(result)
    amplitude = numpy.abs(result)
    phase = numpy.unwrap(numpy.angle(result))
    frequency = numpy.diff(phase) / (2.0 * numpy.pi) * rate
    #frequency.append(frequency[len(frequency)-1])
    frequecny = numpy.append(frequency, frequency[len(frequency) - 1])
    return (amplitude, frequency)


def find_block_average_value(arr, side_block_size, block_size):
    """For each index, finds average value of its block, left block, right block.

    It will find average value for each index in the range.

    For each index, the range of its block is
        [max(0, index - block_size / 2), min(length - 1, index + block_size / 2)]
    For each index, the range of its left block is
        [max(0, index - size_block_size), index]
    For each index, the range of its right block is
        [index, min(length - 1, index + side_block_size)]

    @param arr: The array to be computed.
    @param side_block_size: the size of the left_block and right_block.
    @param block_size: the size of the block.

    @returns: A tuple of lists: (left_block_average_array,
                                 right_block_average_array,
                                 block_average_array)
    """
    length = len(arr)
    left_border, right_border = 0, 1
    left_block_sum = arr[0]
    right_block_sum = arr[0]
    left_average_array = numpy.zeros(length)
    right_average_array = numpy.zeros(length)
    block_average_array = numpy.zeros(length)
    for index in xrange(0, length):
        while left_border < index - side_block_size:
            left_block_sum -= arr[left_border]
            left_border += 1
        while right_border < min(length, index + side_block_size):
            right_block_sum += arr[right_border]
            right_border += 1

        left_average_value = float(left_block_sum) / (index - left_border + 1)
        right_average_value = float(right_block_sum) / (right_border - index)
        left_average_array[index] = left_average_value
        right_average_array[index] = right_average_value

        if index + 1 < length:
            left_block_sum += arr[index + 1]
        right_block_sum -= arr[index]
    left_border, right_border = 0, 1
    block_sum = 0
    for index in xrange(0, length):
        while left_border < index - block_size / 2:
            block_sum -= arr[left_border]
            left_border += 1
        while right_border < min(length, index + block_size / 2):
            block_sum += arr[right_border]
            right_border += 1

        average_value = float(block_sum) / (right_border - left_border)
        block_average_array[index] = average_value
    return (left_average_array, right_average_array, block_average_array)


def find_start_end_index(dominant_frequency,
                         block_frequency_delta,
                         block_size,
                         frequency_error_threshold):
    """Finds start and end index of sine wave.

    For each block with size of block_size, we check that whether its frequency
    is close enough to the dominant_frequency. If yes, we will consider this
    block to be within the sine wave.
    Then, it will return the start and end index of sine wave indicating that
    sine wave is between [start_index, end_index)
    It's okay if the whole signal only contains sine wave.

    @param dominant_frequency: Dominant frequency of signal.
    @param block_frequency_delta: Average absolute difference between dominant
                                  frequency and frequency of each block. For
                                  each index, its block is
                                  [max(0, index - block_size / 2),
                                   min(length - 1, index + block_size / 2)]
    @param block_size: Block size in samples.

    @returns: A tuple composed of (start_index, end_index)

    """
    length = len(block_frequency_delta)

    # Finds the start/end time index of playing based on dominant frequency
    start_index, end_index = length - 1 , 0
    for index in xrange(0, length):
        left_border = max(0, index - block_size / 2)
        right_border = min(length - 1, index + block_size / 2)
        frequency_error = block_frequency_delta[index] / dominant_frequency
        if frequency_error < frequency_error_threshold:
            start_index = min(start_index, left_border)
            end_index = max(end_index, right_border + 1)
    return (start_index, end_index)


def noise_detection(start_index, end_index,
                    block_amplitude, average_amplitude,
                    rate, noise_amplitude_threshold):
    """Detects noise before/after sine wave.

    If average amplitude of some sample's block before start of wave or after
    end of wave is more than average_amplitude times noise_amplitude_threshold,
    it will be considered as a noise.

    @param start_index: Start index of sine wave.
    @param end_index: End index of sine wave.
    @param block_amplitude: An array for average amplitude of each block, where
                            amplitude is computed from Hilbert transform.
    @param average_amplitude: Average amplitude of sine wave.
    @param rate: Sampling rate
    @param noise_amplitude_threshold: If the average amplitude of a block is
                        higher than average amplitude of the wave times
                        noise_amplitude_threshold, it will be considered as
                        noise before/after playback.

    @returns: A tuple of lists indicating the time that noise happens:
            (noise_before_playing, noise_after_playing).

    """
    length = len(block_amplitude)
    amplitude_threshold = average_amplitude * noise_amplitude_threshold
    same_event_samples = rate * DEFAULT_SAME_EVENT_SECS

    # Detects noise before playing.
    noise_time_point = []
    last_noise_end_time_point = []
    previous_noise_index = None
    times = 0
    for index in xrange(0, length):
        # Ignore noise too close to the beginning or the end of sine wave.
        # Check the docstring of NEAR_SINE_START_OR_END_SECS.
        if ((start_index - rate * NEAR_SINE_START_OR_END_SECS) <= index and
            (index < end_index + rate * NEAR_SINE_START_OR_END_SECS)):
            continue

        # Ignore noise too close to the beginning or the end of original data.
        # Check the docstring of NEAR_DATA_START_OR_END_SECS.
        if (float(index) / rate <=
            NEAR_DATA_START_OR_END_SECS + APPEND_ZEROS_SECS):
            continue
        if (float(length - index) / rate <=
            NEAR_DATA_START_OR_END_SECS + APPEND_ZEROS_SECS):
            continue
        if block_amplitude[index] > amplitude_threshold:
            same_event = False
            if previous_noise_index:
                same_event = (index - previous_noise_index) < same_event_samples
            if not same_event:
                index_start_sec = float(index) / rate - APPEND_ZEROS_SECS
                index_end_sec = float(index + 1) / rate - APPEND_ZEROS_SECS
                noise_time_point.append(index_start_sec)
                last_noise_end_time_point.append(index_end_sec)
                times += 1
            index_end_sec = float(index + 1) / rate - APPEND_ZEROS_SECS
            last_noise_end_time_point[times - 1] = index_end_sec
            previous_noise_index = index

    noise_before_playing, noise_after_playing = [], []
    for i in xrange(times):
        duration = last_noise_end_time_point[i] - noise_time_point[i]
        if noise_time_point[i] < float(start_index) / rate - APPEND_ZEROS_SECS:
            noise_before_playing.append((noise_time_point[i], duration))
        else:
            noise_after_playing.append((noise_time_point[i], duration))

    return (noise_before_playing, noise_after_playing)


def delay_detection(start_index, end_index,
                    block_amplitude, average_amplitude,
                    dominant_frequency,
                    rate,
                    left_block_amplitude,
                    right_block_amplitude,
                    block_frequency_delta,
                    delay_amplitude_threshold,
                    frequency_error_threshold):
    """Detects delay during playing.

    For each sample, we will check whether the average amplitude of its block
    is less than average amplitude of its left block and its right block times
    delay_amplitude_threshold. Also, we will check whether the frequency of
    its block is far from the dominant frequency.
    If at least one constraint fulfilled, it will be considered as a delay.

    @param start_index: Start index of sine wave.
    @param end_index: End index of sine wave.
    @param block_amplitude: An array for average amplitude of each block, where
                            amplitude is computed from Hilbert transform.
    @param average_amplitude: Average amplitude of sine wave.
    @param dominant_frequency: Dominant frequency of signal.
    @param rate: Sampling rate
    @param left_block_amplitude: Average amplitude of left block of each index.
                                Ref to find_block_average_value function.
    @param right_block_amplitude: Average amplitude of right block of each index.
                                Ref to find_block_average_value function.
    @param block_frequency_delta: Average absolute difference frequency to
                                dominant frequency of block of each index.
                                Ref to find_block_average_value function.
    @param delay_amplitude_threshold: If the average amplitude of a block is
                        lower than average amplitude of the wave times
                        delay_amplitude_threshold, it will be considered
                        as delay.
    @param frequency_error_threshold: Ref to DEFAULT_FREQUENCY_ERROR

    @returns: List of delay occurrence:
                [(time_1, duration_1), (time_2, duration_2), ...],
              where time and duration are in seconds.

    """
    delay_time_points = []
    last_delay_end_time_points = []
    previous_delay_index = None
    times = 0
    same_event_samples = rate * DEFAULT_SAME_EVENT_SECS
    start_time = float(start_index) / rate - APPEND_ZEROS_SECS
    end_time = float(end_index) / rate - APPEND_ZEROS_SECS
    for index in xrange(start_index, end_index):
        if block_amplitude[index] > average_amplitude * delay_amplitude_threshold:
            continue
        now_time = float(index) / rate - APPEND_ZEROS_SECS
        if abs(now_time - start_time) < NEAR_START_OR_END_SECS:
            continue
        if abs(now_time - end_time) < NEAR_START_OR_END_SECS:
            continue
        # If amplitude less than its left/right side and small enough,
        # it will be considered as a delay.
        amp_threshold = average_amplitude * delay_amplitude_threshold
        left_threshold = delay_amplitude_threshold * left_block_amplitude[index]
        amp_threshold = min(amp_threshold, left_threshold)
        right_threshold = delay_amplitude_threshold * right_block_amplitude[index]
        amp_threshold = min(amp_threshold, right_threshold)

        frequency_error = block_frequency_delta[index] / dominant_frequency

        amplitude_too_small = block_amplitude[index] < amp_threshold
        frequency_not_match = frequency_error > frequency_error_threshold

        if amplitude_too_small or frequency_not_match:
            same_event = False
            if previous_delay_index:
                same_event = (index - previous_delay_index) < same_event_samples
            if not same_event:
                index_start_sec = float(index) / rate - APPEND_ZEROS_SECS
                index_end_sec = float(index + 1) / rate - APPEND_ZEROS_SECS
                delay_time_points.append(index_start_sec)
                last_delay_end_time_points.append(index_end_sec)
                times += 1
            previous_delay_index = index
            index_end_sec = float(index + 1) / rate - APPEND_ZEROS_SECS
            last_delay_end_time_points[times - 1] = index_end_sec

    delay_list = []
    for i in xrange(len(delay_time_points)):
        duration = last_delay_end_time_points[i] - delay_time_points[i]
        delay_list.append( (delay_time_points[i], duration) )
    return delay_list


def burst_detection(start_index, end_index,
                    block_amplitude, average_amplitude,
                    dominant_frequency,
                    rate,
                    left_block_amplitude,
                    right_block_amplitude,
                    block_frequency_delta,
                    burst_amplitude_threshold,
                    frequency_error_threshold):
    """Detects burst during playing.

    For each sample, we will check whether the average amplitude of its block is
    more than average amplitude of its left block and its right block times
    burst_amplitude_threshold. Also, we will check whether the frequency of
    its block is not compatible to the dominant frequency.
    If at least one constraint fulfilled, it will be considered as a burst.

    @param start_index: Start index of sine wave.
    @param end_index: End index of sine wave.
    @param block_amplitude: An array for average amplitude of each block, where
                            amplitude is computed from Hilbert transform.
    @param average_amplitude: Average amplitude of sine wave.
    @param dominant_frequency: Dominant frequency of signal.
    @param rate: Sampling rate
    @param left_block_amplitude: Average amplitude of left block of each index.
                                Ref to find_block_average_value function.
    @param right_block_amplitude: Average amplitude of right block of each index.
                                Ref to find_block_average_value function.
    @param block_frequency_delta: Average absolute difference frequency to
                                dominant frequency of block of each index.
    @param burst_amplitude_threshold: If the amplitude is higher than average
                            amplitude of its left block and its right block
                            times burst_amplitude_threshold. It will be
                            considered as a burst.
    @param frequency_error_threshold: Ref to DEFAULT_FREQUENCY_ERROR

    @returns: List of burst occurence: [time_1, time_2, ...],
              where time is in seconds.

    """
    burst_time_points = []
    previous_burst_index = None
    same_event_samples = rate * DEFAULT_SAME_EVENT_SECS
    for index in xrange(start_index, end_index):
        # If amplitude higher than its left/right side and large enough,
        # it will be considered as a burst.
        if block_amplitude[index] <= average_amplitude * DEFAULT_BURST_TOO_SMALL:
            continue
        if abs(index - start_index) < rate * NEAR_START_OR_END_SECS:
            continue
        if abs(index - end_index) < rate * NEAR_START_OR_END_SECS:
            continue
        amp_threshold = average_amplitude * DEFAULT_BURST_TOO_SMALL
        left_threshold = burst_amplitude_threshold * left_block_amplitude[index]
        amp_threshold = max(amp_threshold, left_threshold)
        right_threshold = burst_amplitude_threshold * right_block_amplitude[index]
        amp_threshold = max(amp_threshold, right_threshold)

        frequency_error = block_frequency_delta[index] / dominant_frequency

        amplitude_too_large = block_amplitude[index] > amp_threshold
        frequency_not_match = frequency_error > frequency_error_threshold

        if amplitude_too_large or frequency_not_match:
            same_event = False
            if previous_burst_index:
                same_event = index - previous_burst_index < same_event_samples
            if not same_event:
                burst_time_points.append(float(index) / rate - APPEND_ZEROS_SECS)
            previous_burst_index = index

    return burst_time_points


def changing_volume_detection(start_index, end_index,
                              average_amplitude,
                              rate,
                              left_block_amplitude,
                              right_block_amplitude,
                              volume_changing_amplitude_threshold):
    """Finds volume changing during playback.

    For each index, we will compare average amplitude of its left block and its
    right block. If average amplitude of right block is more than average
    amplitude of left block times (1 + DEFAULT_VOLUME_CHANGE_AMPLITUDE), it will
    be considered as an increasing volume. If the one of right block is less
    than that of left block times (1 - DEFAULT_VOLUME_CHANGE_AMPLITUDE), it will
    be considered as a decreasing volume.

    @param start_index: Start index of sine wave.
    @param end_index: End index of sine wave.
    @param average_amplitude: Average amplitude of sine wave.
    @param rate: Sampling rate
    @param left_block_amplitude: Average amplitude of left block of each index.
                                Ref to find_block_average_value function.
    @param right_block_amplitude: Average amplitude of right block of each index.
                                Ref to find_block_average_value function.
    @param volume_changing_amplitude_threshold: If the average amplitude of right
                                                block is higher or lower than
                                                that of left one times this
                                                value, it will be considered as
                                                a volume change.
                                                Also refer to
                                                DEFAULT_VOLUME_CHANGE_AMPLITUDE

    @returns: List of volume changing composed of 1 for increasing and
              -1 for decreasing.

    """
    length = len(left_block_amplitude)

    # Detects rising and/or falling volume.
    previous_rising_index, previous_falling_index = None, None
    changing_time = []
    changing_events = []
    amplitude_threshold = average_amplitude * DEFAULT_VOLUME_CHANGE_TOO_SMALL
    same_event_samples = rate * DEFAULT_SAME_EVENT_SECS
    for index in xrange(start_index, end_index):
        # Skips if amplitude is too small.
        if left_block_amplitude[index] < amplitude_threshold:
            continue
        if right_block_amplitude[index] < amplitude_threshold:
            continue
        # Skips if changing is from start or end time
        if float(abs(start_index - index)) / rate < NEAR_START_OR_END_SECS:
            continue
        if float(abs(end_index   - index)) / rate < NEAR_START_OR_END_SECS:
            continue

        delta_margin = volume_changing_amplitude_threshold
        if left_block_amplitude[index] > 0:
            delta_margin *= left_block_amplitude[index]

        increasing_threshold = left_block_amplitude[index] + delta_margin
        decreasing_threshold = left_block_amplitude[index] - delta_margin

        if right_block_amplitude[index] > increasing_threshold:
            same_event = False
            if previous_rising_index:
                same_event = index - previous_rising_index < same_event_samples
            if not same_event:
                changing_time.append(float(index) / rate - APPEND_ZEROS_SECS)
                changing_events.append(+1)
            previous_rising_index = index
        if right_block_amplitude[index] < decreasing_threshold:
            same_event = False
            if previous_falling_index:
                same_event = index - previous_falling_index < same_event_samples
            if not same_event:
                changing_time.append(float(index) / rate - APPEND_ZEROS_SECS)
                changing_events.append(-1)
            previous_falling_index = index

    # Combines consecutive increasing/decreasing event.
    combined_changing_events, prev = [], 0
    for i in xrange(len(changing_events)):
        if changing_events[i] == prev:
            continue
        combined_changing_events.append((changing_time[i], changing_events[i]))
        prev = changing_events[i]
    return combined_changing_events


def quality_measurement(
        signal, rate,
        dominant_frequency=None,
        block_size_secs=DEFAULT_BLOCK_SIZE_SECS,
        frequency_error_threshold=DEFAULT_FREQUENCY_ERROR,
        delay_amplitude_threshold=DEFAULT_DELAY_AMPLITUDE_THRESHOLD,
        noise_amplitude_threshold=DEFAULT_NOISE_AMPLITUDE_THRESHOLD,
        burst_amplitude_threshold=DEFAULT_BURST_AMPLITUDE_THRESHOLD,
        volume_changing_amplitude_threshold=DEFAULT_VOLUME_CHANGE_AMPLITUDE):
    """Detects several artifacts and estimates the noise level.

    This method detects artifact before playing, after playing, and delay
    during playing. Also, it estimates the noise level of the signal.
    To avoid the influence of noise, it calculates amplitude and frequency
    block by block.

    @param signal: A list of numbers for one-channel PCM data. The data should
                   be normalized to [-1,1].
    @param rate: Sampling rate
    @param dominant_frequency: Dominant frequency of signal. Set None to
                               recalculate the frequency in this function.
    @param block_size_secs: Block size in seconds. The measurement will be done
                            block-by-block using average amplitude and frequency
                            in each block to avoid noise.
    @param frequency_error_threshold: Ref to DEFAULT_FREQUENCY_ERROR.
    @param delay_amplitude_threshold: If the average amplitude of a block is
                                      lower than average amplitude of the wave
                                      times delay_amplitude_threshold, it will
                                      be considered as delay.
                                      Also refer to delay_detection and
                                      DEFAULT_DELAY_AMPLITUDE_THRESHOLD.
    @param noise_amplitude_threshold: If the average amplitude of a block is
                                      higher than average amplitude of the wave
                                      times noise_amplitude_threshold, it will
                                      be considered as noise before/after
                                      playback.
                                      Also refer to noise_detection and
                                      DEFAULT_NOISE_AMPLITUDE_THRESHOLD.
    @param burst_amplitude_threshold: If the average amplitude of a block is
                                      higher than average amplitude of its left
                                      block and its right block times
                                      burst_amplitude_threshold. It will be
                                      considered as a burst.
                                      Also refer to burst_detection and
                                      DEFAULT_BURST_AMPLITUDE_THRESHOLD.
    @param volume_changing_amplitude_threshold: If the average amplitude of right
                                                block is higher or lower than
                                                that of left one times this
                                                value, it will be considered as
                                                a volume change.
                                                Also refer to
                                                changing_volume_detection and
                                                DEFAULT_VOLUME_CHANGE_AMPLITUDE

    @returns: A dictoinary of detection/estimation:
              {'artifacts':
                {'noise_before_playback':
                    [(time_1, duration_1), (time_2, duration_2), ...],
                 'noise_after_playback':
                    [(time_1, duration_1), (time_2, duration_2), ...],
                 'delay_during_playback':
                    [(time_1, duration_1), (time_2, duration_2), ...],
                 'burst_during_playback':
                    [time_1, time_2, ...]
                },
               'volume_changes':
                 [(time_1, flag_1), (time_2, flag_2), ...],
               'equivalent_noise_level': level
              }
              where durations and time points are in seconds. And,
              equivalence_noise_level is the quotient of noise and
              wave which refers to DEFAULT_STANDARD_NOISE.
              volume_changes is a list of tuples containing time
              stamps and decreasing/increasing flags for volume
              change events.

    """
    # Calculates the block size, from seconds to samples.
    block_size = int(block_size_secs * rate)

    signal = numpy.concatenate((numpy.zeros(int(rate * APPEND_ZEROS_SECS)),
                                signal,
                                numpy.zeros(int(rate * APPEND_ZEROS_SECS))))
    signal = numpy.array(signal, dtype=float)
    length = len(signal)

    # Calculates the amplitude and frequency.
    amplitude, frequency = hilbert_analysis(signal, rate, block_size)

    # Finds the dominant frequency.
    if not dominant_frequency:
        dominant_frequency = audio_analysis.spectral_analysis(signal, rate)[0][0]

    # Finds the array which contains absolute difference between dominant
    # frequency and frequency at each time point.
    frequency_delta = abs(frequency - dominant_frequency)

    # Computes average amplitude of each type of block
    res = find_block_average_value(amplitude, block_size * 2, block_size)
    left_block_amplitude, right_block_amplitude, block_amplitude = res

    # Computes average absolute difference of frequency and dominant frequency
    # of the block of each index
    _, _, block_frequency_delta = find_block_average_value(frequency_delta,
                                                           block_size * 2,
                                                           block_size)

    # Finds start and end index of sine wave.
    start_index, end_index = find_start_end_index(dominant_frequency,
                                                  block_frequency_delta,
                                                  block_size,
                                                  frequency_error_threshold)

    if start_index > end_index:
        raise SineWaveNotFound('No sine wave found in signal')

    logging.debug('Found sine wave: start: %s, end: %s',
                  float(start_index) / rate - APPEND_ZEROS_SECS,
                  float(end_index) / rate - APPEND_ZEROS_SECS)

    sum_of_amplitude = float(sum(amplitude[start_index:end_index]))
    # Finds average amplitude of sine wave.
    average_amplitude = sum_of_amplitude / (end_index - start_index)

    # Finds noise before and/or after playback.
    noise_before_playing, noise_after_playing = noise_detection(
            start_index, end_index,
            block_amplitude, average_amplitude,
            rate,
            noise_amplitude_threshold)

    # Finds delay during playback.
    delays = delay_detection(start_index, end_index,
                             block_amplitude, average_amplitude,
                             dominant_frequency,
                             rate,
                             left_block_amplitude,
                             right_block_amplitude,
                             block_frequency_delta,
                             delay_amplitude_threshold,
                             frequency_error_threshold)

    # Finds burst during playback.
    burst_time_points = burst_detection(start_index, end_index,
                                        block_amplitude, average_amplitude,
                                        dominant_frequency,
                                        rate,
                                        left_block_amplitude,
                                        right_block_amplitude,
                                        block_frequency_delta,
                                        burst_amplitude_threshold,
                                        frequency_error_threshold)

    # Finds volume changing during playback.
    volume_changes = changing_volume_detection(
            start_index, end_index,
            average_amplitude,
            rate,
            left_block_amplitude,
            right_block_amplitude,
            volume_changing_amplitude_threshold)

    # Calculates the average teager value.
    teager_value = average_teager_value(signal[start_index:end_index],
                                        average_amplitude)

    # Finds out the noise level.
    noise = noise_level(average_amplitude, dominant_frequency,
                        rate,
                        teager_value)

    return {'artifacts':
            {'noise_before_playback': noise_before_playing,
             'noise_after_playback': noise_after_playing,
             'delay_during_playback': delays,
             'burst_during_playback': burst_time_points
            },
            'volume_changes': volume_changes,
            'equivalent_noise_level': noise
           }
