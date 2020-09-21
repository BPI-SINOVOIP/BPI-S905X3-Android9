/*
 * Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

var FFT_SIZE = 2048;

var audioContext;
var tonegen;
var recorder;
var drawContext;
var audioPlay, audioBuffer;
var audioSourceType = "sweep";
var recordSourceType = "microphone";

/**
 * Switches Play/Record tab
 * @param {string} tab name
 */
function switchTab(tabName) {
  var canvas_detail = document.getElementsByClassName('canvas_detail');
  switch (tabName) {
    case 'play_tab':
      document.getElementById('record_tab').setAttribute('class', '');
      document.getElementById('record_div').style.display = 'none';
      document.getElementById('play_div').style.display = 'block';
      for (var i = 0; i < canvas_detail.length; i++) {
        canvas_detail[i].style.display = "none";
      }
      drawContext.drawBg();
      break;
    case 'record_tab':
      document.getElementById('play_tab').setAttribute('class', '');
      document.getElementById('play_div').style.display = 'none';
      document.getElementById('record_div').style.display = 'block';
      for (var i = 0; i < canvas_detail.length; i++) {
        canvas_detail[i].style.display = "block";
      }
      drawContext.drawCanvas();
      break;
  }
  document.getElementById(tabName).setAttribute('class', 'selected');
}

function __log(e, data) {
  log.innerHTML += "\n" + e + " " + (data || '');
}

function startUserMedia(stream) {
  var input = audioContext.createMediaStreamSource(stream);
  recorder = new Recorder(input);
}

window.onload = function init() {
  setupSourceLayer(audioSourceType);
  try {
    // webkit shim
    window.AudioContext = window.AudioContext || window.webkitAudioContext;
    navigator.getUserMedia = navigator.getUserMedia ||
        navigator.webkitGetUserMedia;
    window.URL = window.URL || window.webkitURL;

    audioContext = new AudioContext;
  } catch (e) {
    alert('No web audio support in this browser!');
  }

  navigator.getUserMedia({audio: true}, startUserMedia, function(e) {
    alert('No live audio input: ' + e);
  });

  /* Initialize global objects */
  tonegen = new ToneGen();
  audioPlay = new AudioPlay();

  var canvas = document.getElementById('fr_canvas');
  drawContext = new DrawCanvas(canvas, audioContext.sampleRate / 2);
  drawContext.drawBg();
};

/* For Play tab */

/**
 * Sets audio source layer
 * @param {string} audio source type
 */
function setupSourceLayer(value) {
  var sourceTone = document.getElementById('source_tone');
  var sourceFile = document.getElementById('source_file');
  var sweepTone = document.getElementsByClassName('sweep_tone');
  audioSourceType = value;
  switch (value) {
    case 'sine':
      for (var i = 0; i < sweepTone.length; i++) {
        sweepTone[i].style.display = "none";
      }
      document.getElementById('freq_start').value = 1000;
      document.getElementById('freq_end').value = 1000;
      sourceTone.style.display = "block";
      sourceFile.style.display = "none";
      document.getElementById('play_audio').disabled = false;
      break;
    case 'sweep':
      for (var i = 0; i < sweepTone.length; i++) {
        sweepTone[i].style.display = "block";
      }
      document.getElementById('freq_start').value = 20;
      document.getElementById('freq_end').value = 12000;
      sourceTone.style.display = "block";
      sourceFile.style.display = "none";
      document.getElementById('play_audio').disabled = false;
      break;
    case 'file':
      sourceTone.style.display = "none";
      sourceFile.style.display = "block";
      document.getElementById('play_audio').disabled = true;
      break;
  }
}

/**
 * Sets left/right gain
 */
function gainChanged() {
  var leftGain = document.getElementById('left_gain').value;
  var rightGain = document.getElementById('right_gain').value;
  var gainLabel = document.getElementById('gain_label');
  gainLabel.innerHTML = 'L(' + leftGain + ') / R(' + rightGain + ')';
}

/**
 * Checks sine tone generator parameters and sets audio buffer
 */
function toneValueCheckSet() {
  var passed = true;
  var freqStart = parseInt(document.getElementById('freq_start').value);
  var freqEnd = parseInt(document.getElementById('freq_end').value);
  var duration = parseFloat(document.getElementById('tone_sec').value);
  var leftGain = parseInt(document.getElementById('left_gain').value);
  var rightGain = parseInt(document.getElementById('right_gain').value);
  var sweepLog = document.getElementById('sweep_log').checked;

  function isNumber(value, msg) {
    if (isNaN(value) || value <= 0) {
      alert(msg);
      passed = false;
    }
  }

  if (audioSourceType == 'sine') {
    freqEnd = freqStart;
  }

  isNumber(freqStart, "Start frequency should be a positive number.");
  isNumber(freqEnd, "Stop frequency should be a positive number.");
  isNumber(duration, "Duration should be a positive number.");
  if (freqEnd > audioContext.sampleRate / 2) {
    alert('Stop frequency is too large.');
    passed = false;
  }
  if (freqStart < 20) {
    alert('Start frequency is too small.');
    passed = false;
  }
  if (passed) {
    /* Passed value check and generate tone buffer */
    tonegen.setFreq(freqStart, freqEnd, sweepLog);
    tonegen.setDuration(duration);
    tonegen.setGain(leftGain / 20, rightGain / 20);
    tonegen.setSampleRate(audioContext.sampleRate);
    tonegen.genBuffer();
    var buffer = tonegen.getBuffer();
    audioPlay.setBuffer(buffer, document.getElementById('append_tone').checked);
  }
  return passed;
}

function loadAudioFile() {
  document.getElementById('audio_file').click();
}

/**
 * Loads audio file from local drive
 */
function changeAudioFile() {
  function loadAudioDone(filename, buffer) {
    audioBuffer = buffer;
    document.getElementById('play_audio').disabled = false;
  }
  var input = document.getElementById('audio_file');
  document.getElementById('play_audio').disabled = true;
  audioPlay.loadFile(input.files[0], loadAudioDone);
  input.value = '';
}

/**
 * Play audio according source type
 */
function playAudioFile() {
  /**
   * Callback function to draw frequency response of current buffer
   */
  function getInstantBuffer(leftData, rightData, sampleRate) {
    drawContext.drawInstantCurve(leftData, rightData, sampleRate);
  }

  var btn = document.getElementById('play_audio');
  var append = document.getElementById('append_tone').checked;
  if (btn.className == 'btn-off') {
    switch (audioSourceType) {
      case 'sine':
      case 'sweep':
        if (toneValueCheckSet()) {
          audioPlay.play(playAudioFile, getInstantBuffer);
          btn.className = 'btn-on';
        }
        break;
      case 'file':
        audioPlay.setBuffer(audioBuffer, append);
        audioPlay.play(playAudioFile, getInstantBuffer);
        btn.className = 'btn-on';
        break;
    }
  } else {
    audioPlay.stop();
    btn.className = 'btn-off';
    drawContext.drawBg();
  }
}

/* For Record tab */

/**
 * Sets record source type
 * @param {string} record source type
 */
function setupRecordSource(value) {
  recordSourceType = value;
  var autoStop = document.getElementById('auto_stop');
  if (value == 'audio') {
    autoStop.disabled = true;
    autoStop.checked = false;
  } else {
    autoStop.disabled = false;
    autoStop.checked = true;
  }
}

function loadButtonClicked() {
  document.getElementById('sample_file').click();
}

/**
 * Loads sample file to draw frequency response curve into canvas
 */
function loadSampleFile() {
  /**
   * Callback function when file loaded
   * @param {string} file name
   * @param {AudioBuffer} file buffer
   */
  function addFileToCanvas(filename, buffer) {
    var newBuffer = [];
    for (var i = 0; i < buffer.numberOfChannels; i++) {
      newBuffer.push(buffer.getChannelData(i));
    }
    drawContext.add(new AudioCurve(newBuffer, filename, buffer.sampleRate));
  }
  var input = document.getElementById('sample_file');
  audioPlay.loadFile(input.files[0], addFileToCanvas);
  input.value = '';
}

/**
 * Starts/Stops record function
 */
function recordButtonClicked() {
  /**
   * Callback function to draw frequency response of current recorded buffer
   */
  function getInstantBuffer(leftData, rightData, sampleRate, stop) {
    drawContext.drawInstantCurve(leftData, rightData, sampleRate);
    if (stop)
      recordButtonClicked();
  }

  var btn = document.getElementById('record_btn');
  if (btn.className == 'btn-off') {
    var detect = document.getElementById('detect_tone').checked;
    var autoStop = document.getElementById('auto_stop').checked;
    var append = document.getElementById('append_tone').checked;
    if (recordSourceType == 'audio') {
      switch(audioSourceType) {
        case 'sine':
        case 'sweep':
          if (toneValueCheckSet()) {
            audioPlay.play(recordButtonClicked);
            btn.className = 'btn-on';
          }
          break;
        case 'file':
          audioPlay.setBuffer(audioBuffer, append);
          audioPlay.play(recordButtonClicked);
          btn.className = 'btn-on';
          break;
      }
    } else {
      btn.className = 'btn-on';
    }
    recorder.record(getInstantBuffer, detect, autoStop);
  } else {
    recorder.stop();
    if (recordSourceType == 'audio') {
      audioPlay.stop();
    }
    // create WAV download link using audio data blob
    var filename = new Date().toISOString() + '.wav';
    buffer = recorder.getBuffer();
    drawContext.add(new AudioCurve(buffer, filename, audioContext.sampleRate));
    createDownloadLink(filename);
    recorder.clear();
    btn.className = 'btn-off';
  }
}

/**
 * Creates download link of recorded file
 * @param {string} file name
 */
function createDownloadLink(filename) {
  var blob = recorder.exportWAV();
  var url = URL.createObjectURL(blob);
  var table = document.getElementById('record_list');
  var au = document.createElement('audio');
  au.controls = true;
  au.src = url;

  var hf = document.createElement('a');
  hf.href = url;
  hf.download = filename;
  hf.innerHTML = hf.download;

  var tr = table.insertRow(table.rows.length);
  var td_au = tr.insertCell(0);
  var td_hf = tr.insertCell(1);
  td_hf.style = "white-space: nowrap";
  td_au.appendChild(au);
  td_hf.appendChild(hf);
}

/**
 * Exports frequency response CVS file of curves on the canvas
 */
function exportCSV() {
  var hf = document.getElementById('export_csv');
  var noctaves = document.getElementById('noctaves').value;
  content = drawContext.exportCurve(noctaves);
  var blob = new Blob([content], {type: 'application/octet-stream'});
  var url = URL.createObjectURL(blob);
  hf.href = url;
  hf.download = 'audio.csv';
}
