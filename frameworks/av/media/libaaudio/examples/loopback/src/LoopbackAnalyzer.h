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

/**
 * Tools for measuring latency and for detecting glitches.
 * These classes are pure math and can be used with any audio system.
 */

#ifndef AAUDIO_EXAMPLES_LOOPBACK_ANALYSER_H
#define AAUDIO_EXAMPLES_LOOPBACK_ANALYSER_H

#include <algorithm>
#include <assert.h>
#include <cctype>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <audio_utils/sndfile.h>

// Tag for machine readable results as property = value pairs
#define LOOPBACK_RESULT_TAG      "RESULT: "
#define LOOPBACK_SAMPLE_RATE     48000

#define MILLIS_PER_SECOND        1000

#define MAX_ZEROTH_PARTIAL_BINS  40
constexpr double MAX_ECHO_GAIN = 10.0; // based on experiments, otherwise autocorrelation too noisy

// A narrow impulse seems to have better immunity against over estimating the
// latency due to detecting subharmonics by the auto-correlator.
static const float s_Impulse[] = {
        0.0f, 0.0f, 0.0f, 0.0f, 0.3f, // silence on each side of the impulse
        0.99f, 0.0f, -0.99f, // bipolar with one zero crossing in middle
        -0.3f, 0.0f, 0.0f, 0.0f, 0.0f
};

constexpr int32_t kImpulseSizeInFrames = (int32_t)(sizeof(s_Impulse) / sizeof(s_Impulse[0]));

class PseudoRandom {
public:
    PseudoRandom() {}
    PseudoRandom(int64_t seed)
            :    mSeed(seed)
    {}

    /**
     * Returns the next random double from -1.0 to 1.0
     *
     * @return value from -1.0 to 1.0
     */
     double nextRandomDouble() {
        return nextRandomInteger() * (0.5 / (((int32_t)1) << 30));
    }

    /** Calculate random 32 bit number using linear-congruential method. */
    int32_t nextRandomInteger() {
        // Use values for 64-bit sequence from MMIX by Donald Knuth.
        mSeed = (mSeed * (int64_t)6364136223846793005) + (int64_t)1442695040888963407;
        return (int32_t) (mSeed >> 32); // The higher bits have a longer sequence.
    }

private:
    int64_t mSeed = 99887766;
};

static double calculateCorrelation(const float *a,
                                   const float *b,
                                   int windowSize)
{
    double correlation = 0.0;
    double sumProducts = 0.0;
    double sumSquares = 0.0;

    // Correlate a against b.
    for (int i = 0; i < windowSize; i++) {
        float s1 = a[i];
        float s2 = b[i];
        // Use a normalized cross-correlation.
        sumProducts += s1 * s2;
        sumSquares += ((s1 * s1) + (s2 * s2));
    }

    if (sumSquares >= 0.00000001) {
        correlation = (float) (2.0 * sumProducts / sumSquares);
    }
    return correlation;
}

static int calculateCorrelations(const float *haystack, int haystackSize,
                                 const float *needle, int needleSize,
                                 float *results, int resultSize)
{
    int maxCorrelations = haystackSize - needleSize;
    int numCorrelations = std::min(maxCorrelations, resultSize);

    for (int ic = 0; ic < numCorrelations; ic++) {
        double correlation = calculateCorrelation(&haystack[ic], needle, needleSize);
        results[ic] = correlation;
    }

    return numCorrelations;
}

/*==========================================================================================*/
/**
 * Scan until we get a correlation of a single scan that goes over the tolerance level,
 * peaks then drops back down.
 */
static double findFirstMatch(const float *haystack, int haystackSize,
                             const float *needle, int needleSize, double threshold  )
{
    int ic;
    // How many correlations can we calculate?
    int numCorrelations = haystackSize - needleSize;
    double maxCorrelation = 0.0;
    int peakIndex = -1;
    double location = -1.0;
    const double backThresholdScaler = 0.5;

    for (ic = 0; ic < numCorrelations; ic++) {
        double correlation = calculateCorrelation(&haystack[ic], needle, needleSize);

        if( (correlation > maxCorrelation) ) {
            maxCorrelation = correlation;
            peakIndex = ic;
        }

        //printf("PaQa_FindFirstMatch: ic = %4d, correlation = %8f, maxSum = %8f\n",
        //    ic, correlation, maxSum );
        // Are we past what we were looking for?
        if((maxCorrelation > threshold) && (correlation < backThresholdScaler * maxCorrelation)) {
            location = peakIndex;
            break;
        }
    }

    return location;
}

typedef struct LatencyReport_s {
    double latencyInFrames;
    double confidence;
} LatencyReport;

// Apply a technique similar to Harmonic Product Spectrum Analysis to find echo fundamental.
// Using first echo instead of the original impulse for a better match.
static int measureLatencyFromEchos(const float *haystack, int haystackSize,
                            const float *needle, int needleSize,
                            LatencyReport *report) {
    const double threshold = 0.1;
    printf("measureLatencyFromEchos: haystackSize = %d, needleSize = %d\n",
           haystackSize, needleSize);

    // Find first peak
    int first = (int) (findFirstMatch(haystack,
                                      haystackSize,
                                      needle,
                                      needleSize,
                                      threshold) + 0.5);

    // Use first echo as the needle for the other echos because
    // it will be more similar.
    needle = &haystack[first];
    int again = (int) (findFirstMatch(haystack,
                                      haystackSize,
                                      needle,
                                      needleSize,
                                      threshold) + 0.5);

    printf("measureLatencyFromEchos: first = %d, again at %d\n", first, again);
    first = again;

    // Allocate results array
    int remaining = haystackSize - first;
    const int maxReasonableLatencyFrames = 48000 * 2; // arbitrary but generous value
    int numCorrelations = std::min(remaining, maxReasonableLatencyFrames);
    float *correlations = new float[numCorrelations];
    float *harmonicSums = new float[numCorrelations](); // set to zero

    // Generate correlation for every position.
    numCorrelations = calculateCorrelations(&haystack[first], remaining,
                                            needle, needleSize,
                                            correlations, numCorrelations);

    // Add higher harmonics mapped onto lower harmonics.
    // This reinforces the "fundamental" echo.
    const int numEchoes = 10;
    for (int partial = 1; partial < numEchoes; partial++) {
        for (int i = 0; i < numCorrelations; i++) {
            harmonicSums[i / partial] += correlations[i] / partial;
        }
    }

    // Find highest peak in correlation array.
    float maxCorrelation = 0.0;
    float sumOfPeaks = 0.0;
    int peakIndex = 0;
    const int skip = MAX_ZEROTH_PARTIAL_BINS; // skip low bins
    for (int i = skip; i < numCorrelations; i++) {
        if (harmonicSums[i] > maxCorrelation) {
            maxCorrelation = harmonicSums[i];
            sumOfPeaks += maxCorrelation;
            peakIndex = i;
            printf("maxCorrelation = %f at %d\n", maxCorrelation, peakIndex);
        }
    }

    report->latencyInFrames = peakIndex;
    if (sumOfPeaks < 0.0001) {
        report->confidence = 0.0;
    } else {
        report->confidence = maxCorrelation / sumOfPeaks;
    }

    delete[] correlations;
    delete[] harmonicSums;
    return 0;
}

class AudioRecording
{
public:
    AudioRecording() {
    }
    ~AudioRecording() {
        delete[] mData;
    }

    void allocate(int maxFrames) {
        delete[] mData;
        mData = new float[maxFrames];
        mMaxFrames = maxFrames;
    }

    // Write SHORT data from the first channel.
    int write(int16_t *inputData, int inputChannelCount, int numFrames) {
        // stop at end of buffer
        if ((mFrameCounter + numFrames) > mMaxFrames) {
            numFrames = mMaxFrames - mFrameCounter;
        }
        for (int i = 0; i < numFrames; i++) {
            mData[mFrameCounter++] = inputData[i * inputChannelCount] * (1.0f / 32768);
        }
        return numFrames;
    }

    // Write FLOAT data from the first channel.
    int write(float *inputData, int inputChannelCount, int numFrames) {
        // stop at end of buffer
        if ((mFrameCounter + numFrames) > mMaxFrames) {
            numFrames = mMaxFrames - mFrameCounter;
        }
        for (int i = 0; i < numFrames; i++) {
            mData[mFrameCounter++] = inputData[i * inputChannelCount];
        }
        return numFrames;
    }

    int size() {
        return mFrameCounter;
    }

    float *getData() {
        return mData;
    }

    void setSampleRate(int32_t sampleRate) {
        mSampleRate = sampleRate;
    }

    int32_t getSampleRate() {
        return mSampleRate;
    }

    int save(const char *fileName, bool writeShorts = true) {
        SNDFILE *sndFile = nullptr;
        int written = 0;
        SF_INFO info = {
                .frames = mFrameCounter,
                .samplerate = mSampleRate,
                .channels = 1,
                .format = SF_FORMAT_WAV | (writeShorts ? SF_FORMAT_PCM_16 : SF_FORMAT_FLOAT)
        };

        sndFile = sf_open(fileName, SFM_WRITE, &info);
        if (sndFile == nullptr) {
            printf("AudioRecording::save(%s) failed to open file\n", fileName);
            return -errno;
        }

        written = sf_writef_float(sndFile, mData, mFrameCounter);

        sf_close(sndFile);
        return written;
    }

    int load(const char *fileName) {
        SNDFILE *sndFile = nullptr;
        SF_INFO info;

        sndFile = sf_open(fileName, SFM_READ, &info);
        if (sndFile == nullptr) {
            printf("AudioRecording::load(%s) failed to open file\n", fileName);
            return -errno;
        }

        assert(info.channels == 1);

        allocate(info.frames);
        mFrameCounter = sf_readf_float(sndFile, mData, info.frames);

        sf_close(sndFile);
        return mFrameCounter;
    }

private:
    float  *mData = nullptr;
    int32_t mFrameCounter = 0;
    int32_t mMaxFrames = 0;
    int32_t mSampleRate = 48000; // common default
};

// ====================================================================================
class LoopbackProcessor {
public:
    virtual ~LoopbackProcessor() = default;


    virtual void reset() {}

    virtual void process(float *inputData, int inputChannelCount,
                 float *outputData, int outputChannelCount,
                 int numFrames) = 0;


    virtual void report() = 0;

    virtual void printStatus() {};

    virtual int getResult() {
        return -1;
    }

    virtual bool isDone() {
        return false;
    }

    virtual int save(const char *fileName) {
        (void) fileName;
        return AAUDIO_ERROR_UNIMPLEMENTED;
    }

    virtual int load(const char *fileName) {
        (void) fileName;
        return AAUDIO_ERROR_UNIMPLEMENTED;
    }

    virtual void setSampleRate(int32_t sampleRate) {
        mSampleRate = sampleRate;
    }

    int32_t getSampleRate() {
        return mSampleRate;
    }

    // Measure peak amplitude of buffer.
    static float measurePeakAmplitude(float *inputData, int inputChannelCount, int numFrames) {
        float peak = 0.0f;
        for (int i = 0; i < numFrames; i++) {
            float pos = fabs(*inputData);
            if (pos > peak) {
                peak = pos;
            }
            inputData += inputChannelCount;
        }
        return peak;
    }


private:
    int32_t mSampleRate = LOOPBACK_SAMPLE_RATE;
};

class PeakDetector {
public:
    float process(float input) {
        float output = mPrevious * mDecay;
        if (input > output) {
            output = input;
        }
        mPrevious = output;
        return output;
    }

private:
    float  mDecay = 0.99f;
    float  mPrevious = 0.0f;
};


static void printAudioScope(float sample) {
    const int maxStars = 80; // arbitrary, fits on one line
    char c = '*';
    if (sample < -1.0) {
        sample = -1.0;
        c = '$';
    } else if (sample > 1.0) {
        sample = 1.0;
        c = '$';
    }
    int numSpaces = (int) (((sample + 1.0) * 0.5) * maxStars);
    for (int i = 0; i < numSpaces; i++) {
        putchar(' ');
    }
    printf("%c\n", c);
}

// ====================================================================================
/**
 * Measure latency given a loopback stream data.
 * Uses a state machine to cycle through various stages including:
 *
 */
class EchoAnalyzer : public LoopbackProcessor {
public:

    EchoAnalyzer() : LoopbackProcessor() {
        mAudioRecording.allocate(2 * getSampleRate());
        mAudioRecording.setSampleRate(getSampleRate());
    }

    void setSampleRate(int32_t sampleRate) override {
        LoopbackProcessor::setSampleRate(sampleRate);
        mAudioRecording.setSampleRate(sampleRate);
    }

    void reset() override {
        mDownCounter = 200;
        mLoopCounter = 0;
        mMeasuredLoopGain = 0.0f;
        mEchoGain = 1.0f;
        mState = STATE_INITIAL_SILENCE;
    }

    virtual int getResult() {
        return mState == STATE_DONE ? 0 : -1;
    }

    virtual bool isDone() {
        return mState == STATE_DONE || mState == STATE_FAILED;
    }

    void setGain(float gain) {
        mEchoGain = gain;
    }

    float getGain() {
        return mEchoGain;
    }

    void report() override {

        printf("EchoAnalyzer ---------------\n");
        printf(LOOPBACK_RESULT_TAG "measured.gain          = %f\n", mMeasuredLoopGain);
        printf(LOOPBACK_RESULT_TAG "echo.gain              = %f\n", mEchoGain);
        printf(LOOPBACK_RESULT_TAG "test.state             = %d\n", mState);
        if (mMeasuredLoopGain >= 0.9999) {
            printf("   ERROR - clipping, turn down volume slightly\n");
        } else {
            const float *needle = s_Impulse;
            int needleSize = (int) (sizeof(s_Impulse) / sizeof(float));
            float *haystack = mAudioRecording.getData();
            int haystackSize = mAudioRecording.size();
            measureLatencyFromEchos(haystack, haystackSize, needle, needleSize, &mLatencyReport);
            if (mLatencyReport.confidence < 0.01) {
                printf("   ERROR - confidence too low = %f\n", mLatencyReport.confidence);
            } else {
                double latencyMillis = 1000.0 * mLatencyReport.latencyInFrames / getSampleRate();
                printf(LOOPBACK_RESULT_TAG "latency.frames        = %8.2f\n", mLatencyReport.latencyInFrames);
                printf(LOOPBACK_RESULT_TAG "latency.msec          = %8.2f\n", latencyMillis);
                printf(LOOPBACK_RESULT_TAG "latency.confidence    = %8.6f\n", mLatencyReport.confidence);
            }
        }
    }

    void printStatus() override {
        printf("st = %d, echo gain = %f ", mState, mEchoGain);
    }

    void sendImpulses(float *outputData, int outputChannelCount, int numFrames) {
        while (numFrames-- > 0) {
            float sample = s_Impulse[mSampleIndex++];
            if (mSampleIndex >= kImpulseSizeInFrames) {
                mSampleIndex = 0;
            }

            *outputData = sample;
            outputData += outputChannelCount;
        }
    }

    void sendOneImpulse(float *outputData, int outputChannelCount) {
        mSampleIndex = 0;
        sendImpulses(outputData, outputChannelCount, kImpulseSizeInFrames);
    }

    void process(float *inputData, int inputChannelCount,
                 float *outputData, int outputChannelCount,
                 int numFrames) override {
        int channelsValid = std::min(inputChannelCount, outputChannelCount);
        float peak = 0.0f;
        int numWritten;
        int numSamples;

        echo_state_t nextState = mState;

        switch (mState) {
            case STATE_INITIAL_SILENCE:
                // Output silence at the beginning.
                numSamples = numFrames * outputChannelCount;
                for (int i = 0; i < numSamples; i++) {
                    outputData[i] = 0;
                }
                if (mDownCounter-- <= 0) {
                    nextState = STATE_MEASURING_GAIN;
                    //printf("%5d: switch to STATE_MEASURING_GAIN\n", mLoopCounter);
                    mDownCounter = 8;
                }
                break;

            case STATE_MEASURING_GAIN:
                sendImpulses(outputData, outputChannelCount, numFrames);
                peak = measurePeakAmplitude(inputData, inputChannelCount, numFrames);
                // If we get several in a row then go to next state.
                if (peak > mPulseThreshold) {
                    if (mDownCounter-- <= 0) {
                        //printf("%5d: switch to STATE_WAITING_FOR_SILENCE, measured peak = %f\n",
                        //       mLoopCounter, peak);
                        mDownCounter = 8;
                        mMeasuredLoopGain = peak;  // assumes original pulse amplitude is one
                        // Calculate gain that will give us a nice decaying echo.
                        mEchoGain = mDesiredEchoGain / mMeasuredLoopGain;
                        if (mEchoGain > MAX_ECHO_GAIN) {
                            printf("ERROR - loop gain too low. Increase the volume.\n");
                            nextState = STATE_FAILED;
                        } else {
                            nextState = STATE_WAITING_FOR_SILENCE;
                        }
                    }
                } else if (numFrames > kImpulseSizeInFrames){ // ignore short callbacks
                    mDownCounter = 8;
                }
                break;

            case STATE_WAITING_FOR_SILENCE:
                // Output silence and wait for the echos to die down.
                numSamples = numFrames * outputChannelCount;
                for (int i = 0; i < numSamples; i++) {
                    outputData[i] = 0;
                }
                peak = measurePeakAmplitude(inputData, inputChannelCount, numFrames);
                // If we get several in a row then go to next state.
                if (peak < mSilenceThreshold) {
                    if (mDownCounter-- <= 0) {
                        nextState = STATE_SENDING_PULSE;
                        //printf("%5d: switch to STATE_SENDING_PULSE\n", mLoopCounter);
                        mDownCounter = 8;
                    }
                } else {
                    mDownCounter = 8;
                }
                break;

            case STATE_SENDING_PULSE:
                mAudioRecording.write(inputData, inputChannelCount, numFrames);
                sendOneImpulse(outputData, outputChannelCount);
                nextState = STATE_GATHERING_ECHOS;
                //printf("%5d: switch to STATE_GATHERING_ECHOS\n", mLoopCounter);
                break;

            case STATE_GATHERING_ECHOS:
                numWritten = mAudioRecording.write(inputData, inputChannelCount, numFrames);
                peak = measurePeakAmplitude(inputData, inputChannelCount, numFrames);
                if (peak > mMeasuredLoopGain) {
                    mMeasuredLoopGain = peak;  // AGC might be raising gain so adjust it on the fly.
                    // Recalculate gain that will give us a nice decaying echo.
                    mEchoGain = mDesiredEchoGain / mMeasuredLoopGain;
                }
                // Echo input to output.
                for (int i = 0; i < numFrames; i++) {
                    int ic;
                    for (ic = 0; ic < channelsValid; ic++) {
                        outputData[ic] = inputData[ic] * mEchoGain;
                    }
                    for (; ic < outputChannelCount; ic++) {
                        outputData[ic] = 0;
                    }
                    inputData += inputChannelCount;
                    outputData += outputChannelCount;
                }
                if (numWritten  < numFrames) {
                    nextState = STATE_DONE;
                    //printf("%5d: switch to STATE_DONE\n", mLoopCounter);
                }
                break;

            case STATE_DONE:
            default:
                break;
        }

        mState = nextState;
        mLoopCounter++;
    }

    int save(const char *fileName) override {
        return mAudioRecording.save(fileName);
    }

    int load(const char *fileName) override {
        return mAudioRecording.load(fileName);
    }

private:

    enum echo_state_t {
        STATE_INITIAL_SILENCE,
        STATE_MEASURING_GAIN,
        STATE_WAITING_FOR_SILENCE,
        STATE_SENDING_PULSE,
        STATE_GATHERING_ECHOS,
        STATE_DONE,
        STATE_FAILED
    };

    int32_t         mDownCounter = 500;
    int32_t         mLoopCounter = 0;
    int32_t         mSampleIndex = 0;
    float           mPulseThreshold = 0.02f;
    float           mSilenceThreshold = 0.002f;
    float           mMeasuredLoopGain = 0.0f;
    float           mDesiredEchoGain = 0.95f;
    float           mEchoGain = 1.0f;
    echo_state_t    mState = STATE_INITIAL_SILENCE;

    AudioRecording  mAudioRecording; // contains only the input after the gain detection burst
    LatencyReport   mLatencyReport;
    // PeakDetector    mPeakDetector;
};


// ====================================================================================
/**
 * Output a steady sinewave and analyze the return signal.
 *
 * Use a cosine transform to measure the predicted magnitude and relative phase of the
 * looped back sine wave. Then generate a predicted signal and compare with the actual signal.
 */
class SineAnalyzer : public LoopbackProcessor {
public:

    virtual int getResult() {
        return mState == STATE_LOCKED ? 0 : -1;
    }

    void report() override {
        printf("SineAnalyzer ------------------\n");
        printf(LOOPBACK_RESULT_TAG "peak.amplitude     = %7.5f\n", mPeakAmplitude);
        printf(LOOPBACK_RESULT_TAG "sine.magnitude     = %7.5f\n", mMagnitude);
        printf(LOOPBACK_RESULT_TAG "phase.offset       = %7.5f\n", mPhaseOffset);
        printf(LOOPBACK_RESULT_TAG "ref.phase          = %7.5f\n", mPhase);
        printf(LOOPBACK_RESULT_TAG "frames.accumulated = %6d\n", mFramesAccumulated);
        printf(LOOPBACK_RESULT_TAG "sine.period        = %6d\n", mSinePeriod);
        printf(LOOPBACK_RESULT_TAG "test.state         = %6d\n", mState);
        printf(LOOPBACK_RESULT_TAG "frame.count        = %6d\n", mFrameCounter);
        // Did we ever get a lock?
        bool gotLock = (mState == STATE_LOCKED) || (mGlitchCount > 0);
        if (!gotLock) {
            printf("ERROR - failed to lock on reference sine tone\n");
        } else {
            // Only print if meaningful.
            printf(LOOPBACK_RESULT_TAG "glitch.count       = %6d\n", mGlitchCount);
        }
    }

    void printStatus() override {
        printf("st = %d, #gl = %3d,", mState, mGlitchCount);
    }

    double calculateMagnitude(double *phasePtr = NULL) {
        if (mFramesAccumulated == 0) {
            return 0.0;
        }
        double sinMean = mSinAccumulator / mFramesAccumulated;
        double cosMean = mCosAccumulator / mFramesAccumulated;
        double magnitude = 2.0 * sqrt( (sinMean * sinMean) + (cosMean * cosMean ));
        if( phasePtr != NULL )
        {
            double phase = M_PI_2 - atan2( sinMean, cosMean );
            *phasePtr = phase;
        }
        return magnitude;
    }

    /**
     * @param inputData contains microphone data with sine signal feedback
     * @param outputData contains the reference sine wave
     */
    void process(float *inputData, int inputChannelCount,
                 float *outputData, int outputChannelCount,
                 int numFrames) override {
        mProcessCount++;

        float peak = measurePeakAmplitude(inputData, inputChannelCount, numFrames);
        if (peak > mPeakAmplitude) {
            mPeakAmplitude = peak;
        }

        for (int i = 0; i < numFrames; i++) {
            float sample = inputData[i * inputChannelCount];

            float sinOut = sinf(mPhase);

            switch (mState) {
                case STATE_IDLE:
                case STATE_IMMUNE:
                case STATE_WAITING_FOR_SIGNAL:
                    break;
                case STATE_WAITING_FOR_LOCK:
                    mSinAccumulator += sample * sinOut;
                    mCosAccumulator += sample * cosf(mPhase);
                    mFramesAccumulated++;
                    // Must be a multiple of the period or the calculation will not be accurate.
                    if (mFramesAccumulated == mSinePeriod * PERIODS_NEEDED_FOR_LOCK) {
                        mPhaseOffset = 0.0;
                        mMagnitude = calculateMagnitude(&mPhaseOffset);
                        if (mMagnitude > mThreshold) {
                            if (fabs(mPreviousPhaseOffset - mPhaseOffset) < 0.001) {
                                mState = STATE_LOCKED;
                                //printf("%5d: switch to STATE_LOCKED\n", mFrameCounter);
                            }
                            mPreviousPhaseOffset = mPhaseOffset;
                        }
                        resetAccumulator();
                    }
                    break;

                case STATE_LOCKED: {
                    // Predict next sine value
                    float predicted = sinf(mPhase + mPhaseOffset) * mMagnitude;
                    // printf("    predicted = %f, actual = %f\n", predicted, sample);

                    float diff = predicted - sample;
                    if (fabs(diff) > mTolerance) {
                        mGlitchCount++;
                        //printf("%5d: Got a glitch # %d, predicted = %f, actual = %f\n",
                        //       mFrameCounter, mGlitchCount, predicted, sample);
                        mState = STATE_IMMUNE;
                        //printf("%5d: switch to STATE_IMMUNE\n", mFrameCounter);
                        mDownCounter = mSinePeriod;  // Set duration of IMMUNE state.
                    }

                    // Track incoming signal and slowly adjust magnitude to account
                    // for drift in the DRC or AGC.
                    mSinAccumulator += sample * sinOut;
                    mCosAccumulator += sample * cosf(mPhase);
                    mFramesAccumulated++;
                    // Must be a multiple of the period or the calculation will not be accurate.
                    if (mFramesAccumulated == mSinePeriod) {
                        const double coefficient = 0.1;
                        double phaseOffset = 0.0;
                        double magnitude = calculateMagnitude(&phaseOffset);
                        // One pole averaging filter.
                        mMagnitude = (mMagnitude * (1.0 - coefficient)) + (magnitude * coefficient);
                        resetAccumulator();
                    }
                } break;
            }

            // Output sine wave so we can measure it.
            outputData[i * outputChannelCount] = (sinOut * mOutputAmplitude)
                    + (mWhiteNoise.nextRandomDouble() * mNoiseAmplitude);
            // printf("%5d: sin(%f) = %f, %f\n", i, mPhase, sinOut,  mPhaseIncrement);

            // advance and wrap phase
            mPhase += mPhaseIncrement;
            if (mPhase > M_PI) {
                mPhase -= (2.0 * M_PI);
            }

            mFrameCounter++;
        }

        // Do these once per buffer.
        switch (mState) {
            case STATE_IDLE:
                mState = STATE_IMMUNE; // so we can tell when
                break;
            case STATE_IMMUNE:
                mDownCounter -= numFrames;
                if (mDownCounter <= 0) {
                    mState = STATE_WAITING_FOR_SIGNAL;
                    //printf("%5d: switch to STATE_WAITING_FOR_SIGNAL\n", mFrameCounter);
                }
                break;
            case STATE_WAITING_FOR_SIGNAL:
                if (peak > mThreshold) {
                    mState = STATE_WAITING_FOR_LOCK;
                    //printf("%5d: switch to STATE_WAITING_FOR_LOCK\n", mFrameCounter);
                    resetAccumulator();
                }
                break;
            case STATE_WAITING_FOR_LOCK:
            case STATE_LOCKED:
                break;
        }

    }

    void resetAccumulator() {
        mFramesAccumulated = 0;
        mSinAccumulator = 0.0;
        mCosAccumulator = 0.0;
    }

    void reset() override {
        mGlitchCount = 0;
        mState = STATE_IMMUNE;
        mDownCounter = IMMUNE_FRAME_COUNT;
        mPhaseIncrement = 2.0 * M_PI / mSinePeriod;
        printf("phaseInc = %f for period %d\n", mPhaseIncrement, mSinePeriod);
        resetAccumulator();
        mProcessCount = 0;
    }

private:

    enum sine_state_t {
        STATE_IDLE,
        STATE_IMMUNE,
        STATE_WAITING_FOR_SIGNAL,
        STATE_WAITING_FOR_LOCK,
        STATE_LOCKED
    };

    enum constants {
        IMMUNE_FRAME_COUNT = 48 * 500,
        PERIODS_NEEDED_FOR_LOCK = 8
    };

    int     mSinePeriod = 79;
    double  mPhaseIncrement = 0.0;
    double  mPhase = 0.0;
    double  mPhaseOffset = 0.0;
    double  mPreviousPhaseOffset = 0.0;
    double  mMagnitude = 0.0;
    double  mThreshold = 0.005;
    double  mTolerance = 0.01;
    int32_t mFramesAccumulated = 0;
    int32_t mProcessCount = 0;
    double  mSinAccumulator = 0.0;
    double  mCosAccumulator = 0.0;
    int32_t mGlitchCount = 0;
    double  mPeakAmplitude = 0.0;
    int     mDownCounter = IMMUNE_FRAME_COUNT;
    int32_t mFrameCounter = 0;
    float   mOutputAmplitude = 0.75;

    PseudoRandom  mWhiteNoise;
    float   mNoiseAmplitude = 0.00; // Used to experiment with warbling caused by DRC.

    sine_state_t  mState = STATE_IDLE;
};


#undef LOOPBACK_SAMPLE_RATE
#undef LOOPBACK_RESULT_TAG

#endif /* AAUDIO_EXAMPLES_LOOPBACK_ANALYSER_H */
