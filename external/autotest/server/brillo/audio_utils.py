# Copyright (c) 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Server side audio utilities functions for Brillo."""

import contextlib
import logging
import numpy
import os
import struct
import subprocess
import tempfile
import wave

from autotest_lib.client.common_lib import error


_BITS_PER_BYTE=8

# Thresholds used when comparing files.
#
# The frequency threshold used when comparing files. The frequency of the
# recorded audio has to be within _FREQUENCY_THRESHOLD percent of the frequency
# of the original audio.
_FREQUENCY_THRESHOLD = 0.01
# Noise threshold controls how much noise is allowed as a fraction of the
# magnitude of the peak frequency after taking an FFT. The power of all the
# other frequencies in the signal should be within _FFT_NOISE_THRESHOLD percent
# of the power of the main frequency.
_FFT_NOISE_THRESHOLD = 0.05

# Command used to encode audio. If you want to test with something different,
# this should be changed.
_ENCODING_CMD = 'sox'


def extract_wav_frames(wave_file):
    """Extract all frames from a WAV file.

    @param wave_file: A Wave_read object representing a WAV file opened for
                      reading.

    @return: A list containing the frames in the WAV file.
    """
    num_frames = wave_file.getnframes()
    sample_width = wave_file.getsampwidth()
    if sample_width == 1:
        fmt = '%iB'  # Read 1 byte.
    elif sample_width == 2:
        fmt = '%ih'  # Read 2 bytes.
    elif sample_width == 4:
        fmt = '%ii'  # Read 4 bytes.
    else:
        raise ValueError('Unsupported sample width')
    frames =  list(struct.unpack(fmt % num_frames * wave_file.getnchannels(),
                                 wave_file.readframes(num_frames)))

    # Since 8-bit PCM is unsigned with an offset of 128, we subtract the offset
    # to make it signed since the rest of the code assumes signed numbers.
    if sample_width == 1:
        frames = [val - 128 for val in frames]

    return frames


def check_wav_file(filename, num_channels=None, sample_rate=None,
                   sample_width=None):
    """Checks a WAV file and returns its peak PCM values.

    @param filename: Input WAV file to analyze.
    @param num_channels: Number of channels to expect (None to not check).
    @param sample_rate: Sample rate to expect (None to not check).
    @param sample_width: Sample width to expect (None to not check).

    @return A list of the absolute maximum PCM values for each channel in the
            WAV file.

    @raise ValueError: Failed to process the WAV file or validate an attribute.
    """
    chk_file = None
    try:
        chk_file = wave.open(filename, 'r')
        if num_channels is not None and chk_file.getnchannels() != num_channels:
            raise ValueError('Expected %d channels but got %d instead.',
                             num_channels, chk_file.getnchannels())
        if sample_rate is not None and chk_file.getframerate() != sample_rate:
            raise ValueError('Expected sample rate %d but got %d instead.',
                             sample_rate, chk_file.getframerate())
        if sample_width is not None and chk_file.getsampwidth() != sample_width:
            raise ValueError('Expected sample width %d but got %d instead.',
                             sample_width, chk_file.getsampwidth())
        frames = extract_wav_frames(chk_file)
    except wave.Error as e:
        raise ValueError('Error processing WAV file: %s' % e)
    finally:
        if chk_file is not None:
            chk_file.close()

    peaks = []
    for i in range(chk_file.getnchannels()):
        peaks.append(max(map(abs, frames[i::chk_file.getnchannels()])))
    return peaks;


def generate_sine_file(host, num_channels, sample_rate, sample_width,
                       duration_secs, sine_frequency, temp_dir,
                       file_format='wav'):
    """Generate a sine file and push it to the DUT.

    @param host: An object representing the DUT.
    @param num_channels: Number of channels to use.
    @param sample_rate: Sample rate to use for sine wave generation.
    @param sample_width: Sample width to use for sine wave generation.
    @param duration_secs: Duration in seconds to generate sine wave for.
    @param sine_frequency: Frequency to generate sine wave at.
    @param temp_dir: A temporary directory on the host.
    @param file_format: A string representing the encoding for the audio file.

    @return A tuple of the filename on the server and the DUT.
    """;
    _, local_filename = tempfile.mkstemp(
        prefix='sine-', suffix='.' + file_format, dir=temp_dir)
    if sample_width == 1:
        byte_format = '-e unsigned'
    else:
        byte_format = '-e signed'
    gen_file_cmd = ('sox -n -t wav -c %d %s -b %d -r %d %s synth %d sine %d '
                    'vol 0.9' % (num_channels, byte_format,
                                 sample_width * _BITS_PER_BYTE, sample_rate,
                                 local_filename, duration_secs, sine_frequency))
    logging.info('Command to generate sine wave: %s', gen_file_cmd)
    subprocess.call(gen_file_cmd, shell=True)
    if file_format != 'wav':
        # Convert the file to the appropriate format.
        logging.info('Converting file to %s', file_format)
        _, local_encoded_filename = tempfile.mkstemp(
                prefix='sine-', suffix='.' + file_format, dir=temp_dir)
        cvt_file_cmd = '%s %s %s' % (_ENCODING_CMD, local_filename,
                                     local_encoded_filename)
        logging.info('Command to convert file: %s', cvt_file_cmd)
        subprocess.call(cvt_file_cmd, shell=True)
    else:
        local_encoded_filename = local_filename
    dut_tmp_dir = '/data'
    remote_filename = os.path.join(dut_tmp_dir, 'sine.' + file_format)
    logging.info('Send file to DUT.')
    # TODO(ralphnathan): Find a better place to put this file once the SELinux
    # issues are resolved.
    logging.info('remote_filename %s', remote_filename)
    host.send_file(local_encoded_filename, remote_filename)
    return local_filename, remote_filename


def _is_outside_frequency_threshold(freq_reference, freq_rec):
    """Compares the frequency of the recorded audio with the reference audio.

    This function checks to see if the frequencies corresponding to the peak
    FFT values are similiar meaning that the dominant frequency in the audio
    signal is the same for the recorded audio as that in the audio played.

    @param req_reference: The dominant frequency in the reference audio file.
    @param freq_rec: The dominant frequency in the recorded audio file.

    @return: True is freq_rec is with _FREQUENCY_THRESHOLD percent of
              freq_reference.
    """
    ratio = float(freq_rec) / freq_reference
    if ratio > 1 + _FREQUENCY_THRESHOLD or ratio < 1 - _FREQUENCY_THRESHOLD:
        return True
    return False


def _compare_frames(reference_file_frames, rec_file_frames, num_channels,
                    sample_rate):
    """Compares audio frames from the reference file and the recorded file.

    This method checks for two things:
      1. That the main frequency is the same in both the files. This is done
         using the FFT and observing the frequency corresponding to the
         peak.
      2. That there is no other dominant frequency in the recorded file.
         This is done by sweeping the frequency domain and checking that the
         frequency is always less than _FFT_NOISE_THRESHOLD percentage of
         the peak.

    The key assumption here is that the reference audio file contains only
    one frequency.

    @param reference_file_frames: Audio frames from the reference file.
    @param rec_file_frames: Audio frames from the recorded file.
    @param num_channels: Number of channels in the files.
    @param sample_rate: Sample rate of the files.

    @raise error.TestFail: The frequency of the recorded signal doesn't
                           match that of the reference signal.
    @raise error.TestFail: There is too much noise in the recorded signal.
    """
    for channel in range(num_channels):
        reference_data = reference_file_frames[channel::num_channels]
        rec_data = rec_file_frames[channel::num_channels]

        # Get fft and frequencies corresponding to the fft values.
        fft_reference = numpy.fft.rfft(reference_data)
        fft_rec = numpy.fft.rfft(rec_data)
        fft_freqs_reference = numpy.fft.rfftfreq(len(reference_data),
                                                 1.0 / sample_rate)
        fft_freqs_rec = numpy.fft.rfftfreq(len(rec_data), 1.0 / sample_rate)

        # Get frequency at highest peak.
        freq_reference = fft_freqs_reference[
                numpy.argmax(numpy.abs(fft_reference))]
        abs_fft_rec = numpy.abs(fft_rec)
        freq_rec = fft_freqs_rec[numpy.argmax(abs_fft_rec)]

        # Compare the two frequencies.
        logging.info('Golden frequency of channel %i is %f', channel,
                     freq_reference)
        logging.info('Recorded frequency of channel %i is  %f', channel,
                     freq_rec)
        if _is_outside_frequency_threshold(freq_reference, freq_rec):
            raise error.TestFail('The recorded audio frequency does not match '
                                 'that of the audio played.')

        # Check for noise in the frequency domain.
        fft_rec_peak_val = numpy.max(abs_fft_rec)
        noise_detected = False
        for fft_index, fft_val in enumerate(abs_fft_rec):
            if _is_outside_frequency_threshold(freq_reference, freq_rec):
                # If the frequency exceeds _FFT_NOISE_THRESHOLD, then fail.
                if fft_val > _FFT_NOISE_THRESHOLD * fft_rec_peak_val:
                    logging.warning('Unexpected frequency peak detected at %f '
                                    'Hz.', fft_freqs_rec[fft_index])
                    noise_detected = True

        if noise_detected:
            raise error.TestFail('Signal is noiser than expected.')


def compare_file(reference_audio_filename, test_audio_filename):
    """Compares the recorded audio file to the reference audio file.

    @param reference_audio_filename : Reference audio file containing the
                                      reference signal.
    @param test_audio_filename: Audio file containing audio captured from
                                the test.
    """
    with contextlib.closing(wave.open(reference_audio_filename,
                                      'rb')) as reference_file:
        with contextlib.closing(wave.open(test_audio_filename,
                                          'rb')) as rec_file:
            # Extract data from files.
            reference_file_frames = extract_wav_frames(reference_file)
            rec_file_frames = extract_wav_frames(rec_file)

            num_channels = reference_file.getnchannels()
            _compare_frames(reference_file_frames, rec_file_frames,
                            reference_file.getnchannels(),
                            reference_file.getframerate())
