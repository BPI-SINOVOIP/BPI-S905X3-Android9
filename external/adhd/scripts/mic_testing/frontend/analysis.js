/*
 * Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**
 * Gets a random color
 */
function getRandomColor() {
  var letters = '0123456789ABCDEF'.split('');
  var color = '#';
  for (var i = 0; i < 6; i++) {
    color += letters[Math.floor(Math.random() * 16)];
  }
  return color;
}

/**
 * Audio channel class
 */
var AudioChannel = function(buffer) {
  this.init = function(buffer) {
    this.buffer = buffer;
    this.fftBuffer = this.toFFT(this.buffer);
    this.curveColor = getRandomColor();
    this.visible = true;
  }

  this.toFFT = function(buffer) {
    var k = Math.ceil(Math.log(buffer.length) / Math.LN2);
    var length = Math.pow(2, k);
    var tmpBuffer = new Float32Array(length);

    for (var i = 0; i < buffer.length; i++) {
      tmpBuffer[i] = buffer[i];
    }
    for (var i = buffer.length; i < length; i++) {
      tmpBuffer[i] = 0;
    }
    var fft = new FFT(length);
    fft.forward(tmpBuffer);
    return fft.spectrum;
  }

  this.init(buffer);
}

window.AudioChannel = AudioChannel;

var numberOfCurve = 0;

/**
 * Audio curve class
 */
var AudioCurve = function(buffers, filename, sampleRate) {
  this.init = function(buffers, filename) {
    this.filename = filename;
    this.id = numberOfCurve++;
    this.sampleRate = sampleRate;
    this.channel = [];
    for (var i = 0; i < buffers.length; i++) {
      this.channel.push(new AudioChannel(buffers[i]));
    }
  }
  this.init(buffers, filename);
}

window.AudioCurve = AudioCurve;

/**
 * Draw frequency response of curves on the canvas
 * @param {canvas} HTML canvas element to draw frequency response
 * @param {int} Nyquist frequency, in Hz
 */
var DrawCanvas = function(canvas, nyquist) {
  var HTML_TABLE_ROW_OFFSET = 2;
  var topMargin = 30;
  var leftMargin = 40;
  var downMargin = 10;
  var rightMargin = 30;
  var width = canvas.width - leftMargin - rightMargin;
  var height = canvas.height - topMargin - downMargin;
  var canvasContext = canvas.getContext('2d');
  var pixelsPerDb = height / 96.0;
  var noctaves = 10;
  var curveBuffer = [];

  findId = function(id) {
    for (var i = 0; i < curveBuffer.length; i++)
      if (curveBuffer[i].id == id)
        return i;
    return -1;
  }

  /**
   * Adds curve on the canvas
   * @param {AudioCurve} audio curve object
   */
  this.add = function(audioCurve) {
    curveBuffer.push(audioCurve);
    addTableList();
    this.drawCanvas();
  }

  /**
   * Removes curve from the canvas
   * @param {int} curve index
   */
  this.remove = function(id) {
    var index = findId(id);
    if (index != -1) {
      curveBuffer.splice(index, 1);
      removeTableList(index);
      this.drawCanvas();
    }
  }

  removeTableList = function(index) {
    var table = document.getElementById('curve_table');
    table.deleteRow(index + HTML_TABLE_ROW_OFFSET);
  }

  addTableList = function() {
    var table = document.getElementById('curve_table');
    var index = table.rows.length - HTML_TABLE_ROW_OFFSET;
    var curve_id = curveBuffer[index].id;
    var tr = table.insertRow(table.rows.length);
    var tdCheckbox = tr.insertCell(0);
    var tdFile = tr.insertCell(1);
    var tdLeft = tr.insertCell(2);
    var tdRight = tr.insertCell(3);
    var tdRemove = tr.insertCell(4);

    var checkbox = document.createElement('input');
    checkbox.setAttribute('type', 'checkbox');
    checkbox.checked = true;
    checkbox.onclick = function() {
      setCurveVisible(checkbox, curve_id, 'all');
    }
    tdCheckbox.appendChild(checkbox);
    tdFile.innerHTML = curveBuffer[index].filename;

    var checkLeft = document.createElement('input');
    checkLeft.setAttribute('type', 'checkbox');
    checkLeft.checked = true;
    checkLeft.onclick = function() {
      setCurveVisible(checkLeft, curve_id, 0);
    }
    tdLeft.bgColor = curveBuffer[index].channel[0].curveColor;
    tdLeft.appendChild(checkLeft);

    if (curveBuffer[index].channel.length > 1) {
      var checkRight = document.createElement('input');
      checkRight.setAttribute('type', 'checkbox');
      checkRight.checked = true;
      checkRight.onclick = function() {
        setCurveVisible(checkRight, curve_id, 1);
      }
      tdRight.bgColor = curveBuffer[index].channel[1].curveColor;
      tdRight.appendChild(checkRight);
    }

    var btnRemove = document.createElement('input');
    btnRemove.setAttribute('type', 'button');
    btnRemove.value = 'Remove';
    btnRemove.onclick = function() { removeCurve(curve_id); }
    tdRemove.appendChild(btnRemove);
  }

  /**
   * Sets visibility of curves
   * @param {boolean} visible or not
   * @param {int} curve index
   * @param {int,string} channel index.
   */
  this.setVisible = function(checkbox, id, channel) {
    var index = findId(id);
    if (channel == 'all') {
      for (var i = 0; i < curveBuffer[index].channel.length; i++) {
        curveBuffer[index].channel[i].visible = checkbox.checked;
      }
    } else if (channel == 0 || channel == 1) {
      curveBuffer[index].channel[channel].visible = checkbox.checked;
    }
    this.drawCanvas();
  }

  /**
   * Draws canvas background
   */
  this.drawBg = function() {
    var gridColor = 'rgb(200,200,200)';
    var textColor = 'rgb(238,221,130)';

    /* Draw the background */
    canvasContext.fillStyle = 'rgb(0, 0, 0)';
    canvasContext.fillRect(0, 0, canvas.width, canvas.height);

    /* Draw frequency scale. */
    canvasContext.beginPath();
    canvasContext.lineWidth = 1;
    canvasContext.strokeStyle = gridColor;

    for (var octave = 0; octave <= noctaves; octave++) {
      var x = octave * width / noctaves + leftMargin;

      canvasContext.moveTo(x, topMargin);
      canvasContext.lineTo(x, topMargin + height);
      canvasContext.stroke();

      var f = nyquist * Math.pow(2.0, octave - noctaves);
      canvasContext.textAlign = 'center';
      canvasContext.strokeText(f.toFixed(0) + 'Hz', x, 20);
    }

    /* Draw 0dB line. */
    canvasContext.beginPath();
    canvasContext.moveTo(leftMargin, topMargin + 0.5 * height);
    canvasContext.lineTo(leftMargin + width, topMargin + 0.5 * height);
    canvasContext.stroke();

    /* Draw decibel scale. */
    for (var db = -96.0; db <= 0; db += 12) {
      var y = topMargin + height - (db + 96) * pixelsPerDb;
      canvasContext.beginPath();
      canvasContext.setLineDash([1, 4]);
      canvasContext.moveTo(leftMargin, y);
      canvasContext.lineTo(leftMargin + width, y);
      canvasContext.stroke();
      canvasContext.setLineDash([]);
      canvasContext.strokeStyle = textColor;
      canvasContext.strokeText(db.toFixed(0) + 'dB', 20, y);
      canvasContext.strokeStyle = gridColor;
    }
  }

  /**
   * Draws a channel of a curve
   * @param {Float32Array} fft buffer of a channel
   * @param {string} curve color
   * @param {int} sample rate
   */
  this.drawCurve = function(buffer, curveColor, sampleRate) {
    canvasContext.beginPath();
    canvasContext.lineWidth = 1;
    canvasContext.strokeStyle = curveColor;
    canvasContext.moveTo(leftMargin, topMargin + height);

    for (var i = 0; i < buffer.length; ++i) {
      var f = i * sampleRate / 2 / nyquist / buffer.length;

      /* Convert to log frequency scale (octaves). */
      f = 1 + Math.log(f) / (noctaves * Math.LN2);
      if (f < 0) { continue; }
      /* Draw the magnitude */
      var x = f * width + leftMargin;
      var value = Math.max(20 * Math.log(buffer[i]) / Math.LN10, -96);
      var y = topMargin + height - ((value + 96) * pixelsPerDb);

      canvasContext.lineTo(x, y);
    }
    canvasContext.stroke();
  }

  /**
   * Draws all curves
   */
  this.drawCanvas = function() {
    this.drawBg();
    for (var i = 0; i < curveBuffer.length; i++) {
      for (var j = 0; j < curveBuffer[i].channel.length; j++) {
        if (curveBuffer[i].channel[j].visible) {
          this.drawCurve(curveBuffer[i].channel[j].fftBuffer,
                         curveBuffer[i].channel[j].curveColor,
                         curveBuffer[i].sampleRate);
        }
      }
    }
  }

  /**
   * Draws current buffer
   * @param {Float32Array} left channel buffer
   * @param {Float32Array} right channel buffer
   * @param {int} sample rate
   */
  this.drawInstantCurve = function(leftData, rightData, sampleRate) {
    this.drawBg();
    var fftLeft = new FFT(leftData.length);
    fftLeft.forward(leftData);
    var fftRight = new FFT(rightData.length);
    fftRight.forward(rightData);
    this.drawCurve(fftLeft.spectrum, "#FF0000", sampleRate);
    this.drawCurve(fftRight.spectrum, "#00FF00", sampleRate);
  }

  exportCurveByFreq = function(freqList) {
    function calcIndex(freq, length, sampleRate) {
      var idx = parseInt(freq * length * 2 / sampleRate);
      return Math.min(idx, length - 1);
    }
    /* header */
    channelName = ['L', 'R'];
    cvsString = 'freq';
    for (var i = 0; i < curveBuffer.length; i++) {
      for (var j = 0; j < curveBuffer[i].channel.length; j++) {
        cvsString += ',' + curveBuffer[i].filename + '_' + channelName[j];
      }
    }
    for (var i = 0; i < freqList.length; i++) {
      cvsString += '\n' + freqList[i];
      for (var j = 0; j < curveBuffer.length; j++) {
        var curve = curveBuffer[j];
        for (var k = 0; k < curve.channel.length; k++) {
          var fftBuffer = curve.channel[k].fftBuffer;
          var prevIdx = (i - 1 < 0) ? 0 :
              calcIndex(freqList[i - 1], fftBuffer.length, curve.sampleRate);
          var currIdx = calcIndex(
              freqList[i], fftBuffer.length, curve.sampleRate);

          var sum = 0;
          for (var l = prevIdx; l <= currIdx; l++) { // Get average
            var value = 20 * Math.log(fftBuffer[l]) / Math.LN10;
            sum += value;
          }
          cvsString += ',' + sum / (currIdx - prevIdx + 1);
        }
      }
    }
    return cvsString;
  }

  /**
   * Exports frequency response of curves into CSV format
   * @param {int} point number in octaves
   * @return {string} a string with CSV format
   */
  this.exportCurve = function(nInOctaves) {
    var freqList= [];
    for (var i = 0; i < noctaves; i++) {
      var fStart = nyquist * Math.pow(2.0, i - noctaves);
      var fEnd = nyquist * Math.pow(2.0, i + 1 - noctaves);
      var powerStart = Math.log(fStart) / Math.LN2;
      var powerEnd = Math.log(fEnd) / Math.LN2;
      for (var j = 0; j < nInOctaves; j++) {
        f = Math.pow(2,
            powerStart + j * (powerEnd - powerStart) / nInOctaves);
        freqList.push(f);
      }
    }
    freqList.push(nyquist);
    return exportCurveByFreq(freqList);
  }
}

window.DrawCanvas = DrawCanvas;

/**
 * FFT is a class for calculating the Discrete Fourier Transform of a signal
 * with the Fast Fourier Transform algorithm.
 *
 * @param {Number} bufferSize The size of the sample buffer to be computed.
 * Must be power of 2
 * @constructor
 */
function FFT(bufferSize) {
  this.bufferSize = bufferSize;
  this.spectrum   = new Float32Array(bufferSize/2);
  this.real       = new Float32Array(bufferSize);
  this.imag       = new Float32Array(bufferSize);

  this.reverseTable = new Uint32Array(bufferSize);
  this.sinTable = new Float32Array(bufferSize);
  this.cosTable = new Float32Array(bufferSize);

  var limit = 1;
  var bit = bufferSize >> 1;
  var i;

  while (limit < bufferSize) {
    for (i = 0; i < limit; i++) {
      this.reverseTable[i + limit] = this.reverseTable[i] + bit;
    }

    limit = limit << 1;
    bit = bit >> 1;
  }

  for (i = 0; i < bufferSize; i++) {
    this.sinTable[i] = Math.sin(-Math.PI/i);
    this.cosTable[i] = Math.cos(-Math.PI/i);
  }
}

/**
 * Performs a forward transform on the sample buffer.
 * Converts a time domain signal to frequency domain spectra.
 *
 * @param {Array} buffer The sample buffer. Buffer Length must be power of 2
 * @returns The frequency spectrum array
 */
FFT.prototype.forward = function(buffer) {
  var bufferSize      = this.bufferSize,
      cosTable        = this.cosTable,
      sinTable        = this.sinTable,
      reverseTable    = this.reverseTable,
      real            = this.real,
      imag            = this.imag,
      spectrum        = this.spectrum;

  var k = Math.floor(Math.log(bufferSize) / Math.LN2);

  if (Math.pow(2, k) !== bufferSize) {
    throw "Invalid buffer size, must be a power of 2.";
  }
  if (bufferSize !== buffer.length) {
    throw "Supplied buffer is not the same size as defined FFT. FFT Size: "
        + bufferSize + " Buffer Size: " + buffer.length;
  }

  var halfSize = 1,
      phaseShiftStepReal,
      phaseShiftStepImag,
      currentPhaseShiftReal,
      currentPhaseShiftImag,
      off,
      tr,
      ti,
      tmpReal,
      i;

  for (i = 0; i < bufferSize; i++) {
    real[i] = buffer[reverseTable[i]];
    imag[i] = 0;
  }

  while (halfSize < bufferSize) {
    phaseShiftStepReal = cosTable[halfSize];
    phaseShiftStepImag = sinTable[halfSize];

    currentPhaseShiftReal = 1.0;
    currentPhaseShiftImag = 0.0;

    for (var fftStep = 0; fftStep < halfSize; fftStep++) {
      i = fftStep;

      while (i < bufferSize) {
        off = i + halfSize;
        tr = (currentPhaseShiftReal * real[off]) -
            (currentPhaseShiftImag * imag[off]);
        ti = (currentPhaseShiftReal * imag[off]) +
            (currentPhaseShiftImag * real[off]);
        real[off] = real[i] - tr;
        imag[off] = imag[i] - ti;
        real[i] += tr;
        imag[i] += ti;

        i += halfSize << 1;
      }

      tmpReal = currentPhaseShiftReal;
      currentPhaseShiftReal = (tmpReal * phaseShiftStepReal) -
          (currentPhaseShiftImag * phaseShiftStepImag);
      currentPhaseShiftImag = (tmpReal * phaseShiftStepImag) +
          (currentPhaseShiftImag * phaseShiftStepReal);
    }

    halfSize = halfSize << 1;
  }

  i = bufferSize / 2;
  while(i--) {
    spectrum[i] = 2 * Math.sqrt(real[i] * real[i] + imag[i] * imag[i]) /
        bufferSize;
  }
};

function setCurveVisible(checkbox, id, channel) {
  drawContext.setVisible(checkbox, id, channel);
}

function removeCurve(id) {
  drawContext.remove(id);
}
