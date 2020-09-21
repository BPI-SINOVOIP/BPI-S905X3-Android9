/*
 * Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

var ToneGen = function() {
  /**
  * Initializes tone generator.
   */
  this.init = function() {
    this.audioContext = new AudioContext();
  }

  /**
   * Sets sample rate
   * @param {int} sample rate
   */
  this.setSampleRate = function(sampleRate) {
    this.sampleRate = sampleRate;
  }

  /**
   * Sets start/end frequencies and logarithmic sweep
   * @param {int} start frequency
   * @param {int} end frequency
   * @param {boolean} logarithmic sweep or not
   */
  this.setFreq = function(freqStart, freqEnd, sweepLog) {
    this.freqStart = freqStart;
    this.freqEnd = freqEnd;
    this.sweepLog = sweepLog;
  }

  /**
   * Sets tone duration
   * @param {float} duration in seconds
   */
  this.setDuration = function(duration) {
    this.duration = parseFloat(duration);
  }

  /**
   * Sets left and right gain value
   * @param {float} left gain between 0 and 1
   * @param {float} right gain between 0 and 1
   */
  this.setGain = function(leftGain, rightGain) {
    this.leftGain = parseFloat(leftGain);
    this.rightGain = parseFloat(rightGain);
  }

  /**
   * Generates sine tone buffer
   */
  this.genBuffer = function() {
    this.buffer = this.audioContext.createBuffer(2,
        this.sampleRate * this.duration, this.sampleRate);
    var leftChannel = this.buffer.getChannelData(0);
    var rightChannel = this.buffer.getChannelData(1);
    var phi;
    var k = this.freqEnd / this.freqStart;
    var beta = this.duration / Math.log(k);
    for (var i = 0; i < leftChannel.length; i++) {
      if (this.sweepLog) {
        phi = 2 * Math.PI * this.freqStart * beta *
            (Math.pow(k, i / leftChannel.length) - 1.0);
      } else {
        var f = this.freqStart + (this.freqEnd - this.freqStart) *
            i / leftChannel.length / 2;
        phi = f * 2 * Math.PI * i / this.sampleRate;
      }
      leftChannel[i] = this.leftGain * Math.sin(phi);
      rightChannel[i] = this.rightGain * Math.sin(phi);
    }
  }

  /**
   * Returns generated sine tone buffer
   * @return {AudioBuffer} audio buffer
   */
  this.getBuffer = function() {
    return this.buffer;
  }

  /**
   * Returns append buffer
   * @return {AudioBuffer} append audio buffer
   */
  this.getAppendTone = function(sampleRate) {
    var tone_freq = 1000, duration = 0.5;
    this.setFreq(tone_freq, tone_freq, false);
    this.setDuration(duration);
    this.setGain(1, 1);
    this.setSampleRate(sampleRate);
    this.genBuffer();
    return this.getBuffer();
  }

  this.init();
}

window.ToneGen = ToneGen;

var AudioPlay = function() {
  var playCallback = null;
  var sampleRate;
  var playing = false;

  /**
   * Initializes audio play object
   */
  this.init = function() {
    this.audioContext = new AudioContext();
    this.genChannel();
    this.buffer = null;
    sampleRate = this.audioContext.sampleRate;
  }

  /**
   * Loads audio file
   * @param {blob} audio file
   * @param {function} callback function when file loaded
   */
  this.loadFile = function(file_blob, done_cb) {
    if (file_blob) {
      var audioContext = this.audioContext;
      reader = new FileReader();
      reader.onloadend = function(e) {
        audioContext.decodeAudioData(e.target.result,
            function(buffer) {
              done_cb(file_blob.name, buffer);
            });
      };
      reader.readAsArrayBuffer(file_blob);
    }
  }

  /**
   * Sets audio path
   */
  this.genChannel = function() {
    this.node = (this.audioContext.createScriptProcessor ||
        this.audioContext.createJavaScriptNode).call(
        this.audioContext, 4096, 2, 2);
    this.splitter = this.audioContext.createChannelSplitter(2);
    this.merger = this.audioContext.createChannelMerger(2);
    this.node.connect(this.splitter);
    this.splitter.connect(this.merger, 0, 0);
    this.splitter.connect(this.merger, 1, 1);
    this.merger.connect(this.audioContext.destination);

    this.node.onaudioprocess = function(e) {
      for (var i = 0; i < e.inputBuffer.numberOfChannels; i++) {
        e.outputBuffer.getChannelData(i).set(
            e.inputBuffer.getChannelData(i), 0);
      }
      if (!playing) return;
      if (playCallback) {
        var tmpLeft = e.inputBuffer.getChannelData(0).subarray(
            -FFT_SIZE-1, -1);
        var tmpRight = e.inputBuffer.getChannelData(1).subarray(
            -FFT_SIZE-1, -1);
        playCallback(tmpLeft, tmpRight, sampleRate);
      }
    }
  }

  /**
   * Plays audio
   * @param {function} callback function when audio end
   * @param {function} callback function to get current buffer
   */
  this.play = function(done_cb, play_cb) {
    playCallback = play_cb;
    this.source = this.audioContext.createBufferSource();
    this.source.buffer = this.buffer;
    this.source.onended = function(e) {
      playing = false;
      this.disconnect();
      if (done_cb) {
        done_cb();
      }
    }
    this.source.connect(this.node);
    this.source.start(0);
    playing = true;
  }

  /**
   * Stops audio
   */
  this.stop = function() {
    playing = false;
    this.source.stop();
    this.source.disconnect();
  }

  /**
   * Sets audio buffer
   * @param {AudioBuffer} audio buffer
   * @param {boolean} append tone or not
   */
  this.setBuffer = function(buffer, append) {
    if (append) {
      function copyBuffer(src, dest, offset) {
        for (var i = 0; i < dest.numberOfChannels; i++) {
          dest.getChannelData(i).set(src.getChannelData(i), offset);
        }
      }
      var appendTone = tonegen.getAppendTone(buffer.sampleRate);
      var bufferLength = appendTone.length * 2 + buffer.length;
      var newBuffer = this.audioContext.createBuffer(buffer.numberOfChannels,
          bufferLength, buffer.sampleRate);
      copyBuffer(appendTone, newBuffer, 0);
      copyBuffer(buffer, newBuffer, appendTone.length);
      copyBuffer(appendTone, newBuffer, appendTone.length + buffer.length);
      this.buffer = newBuffer;
    } else {
      this.buffer = buffer;
    }
  }

  this.init();
}

window.AudioPlay = AudioPlay;
