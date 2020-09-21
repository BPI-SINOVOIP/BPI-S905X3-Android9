/*
 * Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

var Recorder = function(source){
  var bufferLen = 4096;
  var toneFreq = 1000, errorMargin = 0.05;

  var context = source.context;
  var sampleRate = context.sampleRate;
  var recBuffersL = [], recBuffersR = [], recLength = 0;
  this.node = (context.createScriptProcessor ||
              context.createJavaScriptNode).call(context, bufferLen, 2, 2);
  var detectAppend = false, autoStop = false, recordCallback;
  var recording = false;
  var freqString;

  this.node.onaudioprocess = function(e) {
    if (!recording) return;

    var length = e.inputBuffer.getChannelData(0).length;
    var tmpLeft = new Float32Array(length);
    var tmpRight = new Float32Array(length);
    tmpLeft.set(e.inputBuffer.getChannelData(0), 0);
    tmpRight.set(e.inputBuffer.getChannelData(1), 0);

    recBuffersL.push(tmpLeft);
    recBuffersR.push(tmpRight);
    recLength += length;
    var stop = false;

    if (autoStop && detectTone(getFreqList(tmpLeft)))
      stop = true;

    if (recordCallback) {
      var tmpLeft = recBuffersL[recBuffersL.length - 1].subarray(
          -FFT_SIZE-1, -1);
      var tmpRight = recBuffersR[recBuffersR.length - 1].subarray(
          -FFT_SIZE-1, -1);
      recordCallback(tmpLeft, tmpRight, sampleRate, stop);
    }
  }

  /**
   * Starts recording
   * @param {function} callback function to get current buffer
   * @param {boolean} detect append tone or not
   * @param {boolean} auto stop when detecting append tone
   */
  this.record = function(cb, detect, stop) {
    recordCallback = cb;
    detectAppend = detect;
    autoStop = stop;
    recording = true;
  }

  /**
   * Stops recording
   */
  this.stop = function() {
    recording = false;
    recBuffersL = mergeBuffers(recBuffersL, recLength);
    recBuffersR = mergeBuffers(recBuffersR, recLength);
    if (detectAppend) {
      var freqList = getFreqList(recBuffersL);
      var index = getToneIndices(freqList);
      removeAppendTone(index[0], index[1]);
      exportFreqList(freqList);
    }
  }

  /**
   * Gets frequencies list
   * @param {Float32Array} buffer
   * @return {array} frequencies list
   */
  getFreqList = function(buffer) {
    var prevPeak = 0;
    var valid = true;
    var freqList = [];
    for (i = 1; i < recLength; i++) {
      if (buffer[i] > 0.1 &&
          buffer[i] >= buffer[i - 1] && buffer[i] >= buffer[i + 1]) {
        if (valid) {
          var freq = sampleRate / (i - prevPeak);
          freqList.push([freq, prevPeak, i]);
          prevPeak = i;
          valid = false;
        }
      } else if (buffer[i] < -0.1) {
        valid = true;
      }
    }
    return freqList;
  }

  /**
   * Checks average frequency is in allowed error margin
   * @param {float} average frequency
   * @return {boolean} checked result pass or fail
   */
  checkFreq = function (average) {
    if (Math.abs(average - toneFreq) / toneFreq < errorMargin)
      return true;
    return false;
  }

  /**
   * Detects append tone while recording.
   * @param {array} frequencies list
   * @return {boolean} detected or not
   */
  detectTone = function(freqList) {
    var passCriterion = 50;
    // Initialize function static variables
    if (typeof detectTone.startDetected == 'undefined') {
      detectTone.startDetected = false;
      detectTone.canStop = false;
      detectTone.accumulateTone = 0;
    }

    var windowSize = 10, windowSum = 0, i;
    var detected = false;
    for (i = 0; i < freqList.length && i < windowSize; i++) {
      windowSum += freqList[i][0];
    }
    if (checkFreq(windowSum / Math.min(windowSize, freqList.length))) {
      detected = true;
      detectTone.accumulateTone++;
    }
    for (; i < freqList.length; i++) {
      windowSum = windowSum + freqList[i][0] - freqList[i - windowSize][0];
      if (checkFreq(windowSum / windowSize)) {
        detected = true;
        detectTone.accumulateTone++;
      }
    }
    if (detected) {
      if (detectTone.accumulateTone > passCriterion) {
        if (!detectTone.startDetected)
          detectTone.startDetected = true;
        else if (detectTone.canStop) {
          detectTone.startDetected = false;
          detectTone.canStop = false;
          detectTone.accumulateTone = 0;
          return true;
        }
      }
    } else {
      detectTone.accumulateTone = 0;
      if (detectTone.startDetected)
        detectTone.canStop = true;
    }
    return false;
  }

  /**
   * Gets start and end indices from a frquencies list except append tone
   * @param {array} frequencies list
   * @return {array} start and end indices
   */
  getToneIndices = function(freqList) {
    // find start and end indices
    var flag, j, k;
    var windowSize = 10, windowSum;
    var index = new Array(2);
    var scanRange = [[0, freqList.length, 1], [freqList.length - 1, -1, -1]];

    if (freqList.length == 0) return index;

    for (i = 0; i < 2; i++) {
      flag = false;
      windowSum = 0;
      for (j = scanRange[i][0], k = 0; k < windowSize && j != scanRange[i][1];
          j += scanRange[i][2], k++) {
        windowSum += freqList[j][0];
      }
      for (; j != scanRange[i][1]; j += scanRange[i][2]) {
        windowSum = windowSum + freqList[j][0] -
            freqList[j - scanRange[i][2] * windowSize][0];
        var avg = windowSum / windowSize;
        if (checkFreq(avg) && !flag) {
          flag = true;
        }
        if (!checkFreq(avg) && flag) {
          index[i] = freqList[j][1];
          break;
        }
      }
    }
    return index;
  }

  /**
   * Removes append tone from recorded buffer
   * @param {int} start index
   * @param {int} end index
   */
  removeAppendTone = function(start, end) {
    if (!isNaN(start) && !isNaN(end) && end > start) {
      recBuffersL = truncateBuffers(recBuffersL, recLength, start, end);
      recBuffersR = truncateBuffers(recBuffersR, recLength, start, end);
      recLength = end - start;
    }
  }

  /**
   * Exports frequency list for debugging purpose
   */
  exportFreqList = function(freqList) {
    freqString = sampleRate + '\n';
    for (var i = 0; i < freqList.length; i++) {
      freqString += freqList[i][0] + ' ' + freqList[i][1] + ' ' +
          freqList[i][2] + '\n';
    }
  }

  this.getFreq = function() {
    return freqString;
  }

  /**
   * Clears recorded buffer
   */
  this.clear = function() {
    recLength = 0;
    recBuffersL = [];
    recBuffersR = [];
  }

  /**
   * Gets recorded buffer
   */
  this.getBuffer = function() {
    var buffers = [];
    buffers.push(recBuffersL);
    buffers.push(recBuffersR);
    return buffers;
  }

  /**
   * Exports WAV format file
   * @return {blob} audio file blob
   */
  this.exportWAV = function(type) {
    type = type || 'audio/wav';
    var interleaved = interleave(recBuffersL, recBuffersR);
    var dataview = encodeWAV(interleaved);
    var audioBlob = new Blob([dataview], { type: type });
    return audioBlob;
  }

  /**
   * Truncates buffer from start index to end index
   * @param {Float32Array} audio buffer
   * @param {int} buffer length
   * @param {int} start index
   * @param {int} end index
   * @return {Float32Array} a truncated buffer
   */
  truncateBuffers = function(recBuffers, recLength, startIdx, endIdx) {
    var buffer = new Float32Array(endIdx - startIdx);
    for (var i = startIdx, j = 0; i < endIdx; i++, j++) {
      buffer[j] = recBuffers[i];
    }
    return buffer;
  }

  /**
   * Merges buffer into an array
   * @param {array} a list of Float32Array of audio buffer
   * @param {int} buffer length
   * @return {Float32Array} a merged buffer
   */
  mergeBuffers = function(recBuffers, recLength) {
    var result = new Float32Array(recLength);
    var offset = 0;
    for (var i = 0; i < recBuffers.length; i++){
      result.set(recBuffers[i], offset);
      offset += recBuffers[i].length;
    }
    return result;
  }

  /**
   * Interleaves left and right channel buffer
   * @param {Float32Array} left channel buffer
   * @param {Float32Array} right channel buffer
   * @return {Float32Array} an interleaved buffer
   */
  interleave = function(inputL, inputR) {
    var length = inputL.length + inputR.length;
    var result = new Float32Array(length);

    var index = 0,
      inputIndex = 0;

    while (index < length){
      result[index++] = inputL[inputIndex];
      result[index++] = inputR[inputIndex];
      inputIndex++;
    }
    return result;
  }

  floatTo16BitPCM = function(output, offset, input) {
    for (var i = 0; i < input.length; i++, offset+=2){
      var s = Math.max(-1, Math.min(1, input[i]));
      output.setInt16(offset, s < 0 ? s * 0x8000 : s * 0x7FFF, true);
    }
  }

  writeString = function(view, offset, string) {
    for (var i = 0; i < string.length; i++){
      view.setUint8(offset + i, string.charCodeAt(i));
    }
  }

  /**
   * Encodes audio buffer into WAV format raw data
   * @param {Float32Array} an interleaved buffer
   * @return {DataView} WAV format raw data
   */
  encodeWAV = function(samples) {
    var buffer = new ArrayBuffer(44 + samples.length * 2);
    var view = new DataView(buffer);

    /* RIFF identifier */
    writeString(view, 0, 'RIFF');
    /* file length */
    view.setUint32(4, 32 + samples.length * 2, true);
    /* RIFF type */
    writeString(view, 8, 'WAVE');
    /* format chunk identifier */
    writeString(view, 12, 'fmt ');
    /* format chunk length */
    view.setUint32(16, 16, true);
    /* sample format (raw) */
    view.setUint16(20, 1, true);
    /* channel count */
    view.setUint16(22, 2, true);
    /* sample rate */
    view.setUint32(24, sampleRate, true);
    /* byte rate (sample rate * block align) */
    view.setUint32(28, sampleRate * 4, true);
    /* block align (channel count * bytes per sample) */
    view.setUint16(32, 4, true);
    /* bits per sample */
    view.setUint16(34, 16, true);
    /* data chunk identifier */
    writeString(view, 36, 'data');
    /* data chunk length */
    view.setUint32(40, samples.length * 2, true);

    floatTo16BitPCM(view, 44, samples);

    return view;
  }

  source.connect(this.node);
  this.node.connect(context.destination);
};

window.Recorder = Recorder;
