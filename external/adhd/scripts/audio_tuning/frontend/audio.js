/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* This is a program for tuning audio using Web Audio API. The processing
 * pipeline looks like this:
 *
 *                   INPUT
 *                     |
 *               +------------+
 *               | crossover  |
 *               +------------+
 *               /     |      \
 *      (low band) (mid band) (high band)
 *             /       |        \
 *         +------+ +------+ +------+
 *         |  DRC | |  DRC | |  DRC |
 *         +------+ +------+ +------+
 *              \      |        /
 *               \     |       /
 *              +-------------+
 *              |     (+)     |
 *              +-------------+
 *                 |        |
 *              (left)   (right)
 *                 |        |
 *              +----+   +----+
 *              | EQ |   | EQ |
 *              +----+   +----+
 *                 |        |
 *              +----+   +----+
 *              | EQ |   | EQ |
 *              +----+   +----+
 *                 .        .
 *                 .        .
 *              +----+   +----+
 *              | EQ |   | EQ |
 *              +----+   +----+
 *                  \     /
 *                   \   /
 *                     |
 *                   /   \
 *                  /     \
 *             +-----+   +-----+
 *             | FFT |   | FFT | (for visualization only)
 *             +-----+   +-----+
 *                  \     /
 *                   \   /
 *                     |
 *                   OUTPUT
 *
 * The parameters of each DRC and EQ can be adjusted or disabled independently.
 *
 * If enable_swap is set to true, the order of the DRC and the EQ stages are
 * swapped (EQ is applied first, then DRC).
 */

/* The GLOBAL state has following parameters:
 * enable_drc - A switch to turn all DRC on/off.
 * enable_eq - A switch to turn all EQ on/off.
 * enable_fft - A switch to turn visualization on/off.
 * enable_swap - A switch to swap the order of EQ and DRC stages.
 */

/* The DRC has following parameters:
 * f - The lower frequency of the band, in Hz.
 * enable - 1 to enable the compressor, 0 to disable it.
 * threshold - The value above which the compression starts, in dB.
 * knee - The value above which the knee region starts, in dB.
 * ratio - The input/output dB ratio after the knee region.
 * attack - The time to reduce the gain by 10dB, in seconds.
 * release - The time to increase the gain by 10dB, in seconds.
 * boost - The static boost value in output, in dB.
 */

/* The EQ has following parameters:
 * enable - 1 to enable the eq, 0 to disable it.
 * type - The type of the eq, the available values are 'lowpass', 'highpass',
 *     'bandpass', 'lowshelf', 'highshelf', 'peaking', 'notch'.
 * freq - The frequency of the eq, in Hz.
 * q, gain - The meaning depends on the type of the filter. See Web Audio API
 *     for details.
 */

/* The initial values of parameters for GLOBAL, DRC and EQ */
var INIT_GLOBAL_ENABLE_DRC = true;
var INIT_GLOBAL_ENABLE_EQ = true;
var INIT_GLOBAL_ENABLE_FFT = true;
var INIT_GLOBAL_ENABLE_SWAP = false;
var INIT_DRC_XO_LOW = 200;
var INIT_DRC_XO_HIGH = 2000;
var INIT_DRC_ENABLE = true;
var INIT_DRC_THRESHOLD = -24;
var INIT_DRC_KNEE = 30;
var INIT_DRC_RATIO = 12;
var INIT_DRC_ATTACK = 0.003;
var INIT_DRC_RELEASE = 0.250;
var INIT_DRC_BOOST = 0;
var INIT_EQ_ENABLE = true;
var INIT_EQ_TYPE = 'peaking';
var INIT_EQ_FREQ = 350;
var INIT_EQ_Q = 1;
var INIT_EQ_GAIN = 0;

var NEQ = 8;  /* The number of EQs per channel */
var FFT_SIZE = 2048;  /* The size of FFT used for visualization */

var audioContext;  /* Web Audio context */
var nyquist;       /* Nyquist frequency, in Hz */
var sourceNode;
var audio_graph;
var audio_ui;
var analyzer_left;      /* The FFT analyzer for left channel */
var analyzer_right;     /* The FFT analyzer for right channel */
/* get_emphasis_disabled detects if pre-emphasis in drc is disabled by browser.
 * The detection result will be stored in this value. When user saves config,
 * This value is stored in drc.emphasis_disabled in the config. */
var browser_emphasis_disabled_detection_result;
/* check_biquad_filter_q detects if the browser implements the lowpass and
 * highpass biquad filters with the original formula or the new formula from
 * Audio EQ Cookbook. Chrome changed the filter implementation in R53, see:
 * https://github.com/GoogleChrome/web-audio-samples/wiki/Detection-of-lowpass-BiquadFilter-implementation
 * The detection result is saved in this value before the page is initialized.
 * make_biquad_q() uses this value to compute Q to ensure consistent behavior
 * on different browser versions.
 */
var browser_biquad_filter_uses_audio_cookbook_formula;

/* Check the lowpass implementation and return a promise. */
function check_biquad_filter_q() {
  'use strict';
  var context = new OfflineAudioContext(1, 128, 48000);
  var osc = context.createOscillator();
  var filter1 = context.createBiquadFilter();
  var filter2 = context.createBiquadFilter();
  var inverter = context.createGain();

  osc.type = 'sawtooth';
  osc.frequency.value = 8 * 440;
  inverter.gain.value = -1;
  /* each filter should get a different Q value */
  filter1.Q.value = -1;
  filter2.Q.value = -20;
  osc.connect(filter1);
  osc.connect(filter2);
  filter1.connect(context.destination);
  filter2.connect(inverter);
  inverter.connect(context.destination);
  osc.start();

  return context.startRendering().then(function (buffer) {
    return browser_biquad_filter_uses_audio_cookbook_formula =
      Math.max(...buffer.getChannelData(0)) !== 0;
  });
}

/* Return the Q value to be used with the lowpass and highpass biquad filters,
 * given Q in dB for the original filter formula. If the browser uses the new
 * formula, conversion is made to simulate the original frequency response
 * with the new formula.
 */
function make_biquad_q(q_db) {
  if (!browser_biquad_filter_uses_audio_cookbook_formula)
    return q_db;

  var q_lin = dBToLinear(q_db);
  var q_new = 1 / Math.sqrt((4 - Math.sqrt(16 - 16 / (q_lin * q_lin))) / 2);
  q_new = linearToDb(q_new);
  return q_new;
}

/* The supported audio element names are different on browsers with different
 * versions.*/
function fix_audio_elements() {
  try {
    window.AudioContext = window.AudioContext || window.webkitAudioContext;
    window.OfflineAudioContext = (window.OfflineAudioContext ||
        window.webkitOfflineAudioContext);
  }
  catch(e) {
    alert('Web Audio API is not supported in this browser');
  }
}

function init_audio() {
  audioContext = new AudioContext();
  nyquist = audioContext.sampleRate / 2;
}

function build_graph() {
  if (sourceNode) {
    audio_graph = new graph();
    sourceNode.disconnect();
    if (get_global('enable_drc') || get_global('enable_eq') ||
        get_global('enable_fft')) {
      connect_from_native(pin(sourceNode), audio_graph);
      connect_to_native(audio_graph, pin(audioContext.destination));
    } else {
      /* no processing needed, directly connect from source to destination. */
      sourceNode.connect(audioContext.destination);
    }
  }
  apply_all_configs();
}

/* The available configuration variables are:
 *
 * global.{enable_drc, enable_eq, enable_fft, enable_swap}
 * drc.[0-2].{f, enable, threshold, knee, ratio, attack, release, boost}
 * eq.[01].[0-7].{enable, type, freq, q, gain}.
 *
 * Each configuration variable maps a name to a value. For example,
 * "drc.1.attack" is the attack time for the second drc (the "1" is the index of
 * the drc instance), and "eq.0.2.freq" is the frequency of the third eq on the
 * left channel (the "0" means left channel, and the "2" is the index of the
 * eq).
 */
var all_configs = {};  /* stores all the configuration variables */

function init_config() {
  set_config('global', 'enable_drc', INIT_GLOBAL_ENABLE_DRC);
  set_config('global', 'enable_eq', INIT_GLOBAL_ENABLE_EQ);
  set_config('global', 'enable_fft', INIT_GLOBAL_ENABLE_FFT);
  set_config('global', 'enable_swap', INIT_GLOBAL_ENABLE_SWAP);
  set_config('drc', 0, 'f', 0);
  set_config('drc', 1, 'f', INIT_DRC_XO_LOW);
  set_config('drc', 2, 'f', INIT_DRC_XO_HIGH);
  for (var i = 0; i < 3; i++) {
    set_config('drc', i, 'enable', INIT_DRC_ENABLE);
    set_config('drc', i, 'threshold', INIT_DRC_THRESHOLD);
    set_config('drc', i, 'knee', INIT_DRC_KNEE);
    set_config('drc', i, 'ratio', INIT_DRC_RATIO);
    set_config('drc', i, 'attack', INIT_DRC_ATTACK);
    set_config('drc', i, 'release', INIT_DRC_RELEASE);
    set_config('drc', i, 'boost', INIT_DRC_BOOST);
  }
  for (var i = 0; i <= 1; i++) {
    for (var j = 0; j < NEQ; j++) {
      set_config('eq', i, j, 'enable', INIT_EQ_ENABLE);
      set_config('eq', i, j, 'type', INIT_EQ_TYPE);
      set_config('eq', i, j, 'freq', INIT_EQ_FREQ);
      set_config('eq', i, j, 'q', INIT_EQ_Q);
      set_config('eq', i, j, 'gain', INIT_EQ_GAIN);
    }
  }
}

/* Returns a string from the first n elements of a, joined by '.' */
function make_name(a, n) {
  var sub = [];
  for (var i = 0; i < n; i++) {
    sub.push(a[i].toString());
  }
  return sub.join('.');
}

function get_config() {
  var name = make_name(arguments, arguments.length);
  return all_configs[name];
}

function set_config() {
  var n = arguments.length;
  var name = make_name(arguments, n - 1);
  all_configs[name] = arguments[n - 1];
}

/* Convenience function */
function get_global(name) {
  return get_config('global', name);
}

/* set_config and apply it to the audio graph and ui. */
function use_config() {
  var n = arguments.length;
  var name = make_name(arguments, n - 1);
  all_configs[name] = arguments[n - 1];
  if (audio_graph) {
    audio_graph.config(name.split('.'), all_configs[name]);
  }
  if (audio_ui) {
    audio_ui.config(name.split('.'), all_configs[name]);
  }
}

/* re-apply all the configs to audio graph and ui. */
function apply_all_configs() {
  for (var name in all_configs) {
    if (audio_graph) {
      audio_graph.config(name.split('.'), all_configs[name]);
    }
    if (audio_ui) {
      audio_ui.config(name.split('.'), all_configs[name]);
    }
  }
}

/* Returns a zero-padded two digits number, for time formatting. */
function two(n) {
  var s = '00' + n;
  return s.slice(-2);
}

/* Returns a time string, used for save file name */
function time_str() {
  var d = new Date();
  var date = two(d.getDate());
  var month = two(d.getMonth() + 1);
  var hour = two(d.getHours());
  var minutes = two(d.getMinutes());
  return month + date + '-' + hour + minutes;
}

/* Downloads the current config to a file. */
function save_config() {
  set_config('drc', 'emphasis_disabled',
             browser_emphasis_disabled_detection_result);
  var a = document.getElementById('save_config_anchor');
  var content = JSON.stringify(all_configs, undefined, 2);
  var uriContent = 'data:application/octet-stream,' +
      encodeURIComponent(content);
  a.href = uriContent;
  a.download = 'audio-' + time_str() + '.conf';
  a.click();
}

/* Loads a config file. */
function load_config() {
  document.getElementById('config_file').click();
}

function config_file_changed() {
  var input = document.getElementById('config_file');
  var file = input.files[0];
  var reader = new FileReader();
  function onloadend() {
    var configs = JSON.parse(reader.result);
    init_config();
    for (var name in configs) {
      all_configs[name] = configs[name];
    }
    build_graph();
  }
  reader.onloadend = onloadend;
  reader.readAsText(file);
  input.value = '';
}

/* ============================ Audio components ============================ */

/* We wrap Web Audio nodes into our own components. Each component has following
 * methods:
 *
 * function input(n) - Returns a list of pins which are the n-th input of the
 * component.
 *
 * function output(n) - Returns a list of pins which are the n-th output of the
 * component.
 *
 * function config(name, value) - Changes the configuration variable for the
 * component.
 *
 * Each "pin" is just one input/output of a Web Audio node.
 */

/* Returns the top-level audio component */
function graph() {
  var stages = [];
  var drcs, eqs, ffts;
  if (get_global('enable_drc')) {
    drcs = new drc_3band();
  }
  if (get_global('enable_eq')) {
    eqs = new eq_2chan();
  }
  if (get_global('enable_swap')) {
    if (eqs) stages.push(eqs);
    if (drcs) stages.push(drcs);
  } else {
    if (drcs) stages.push(drcs);
    if (eqs) stages.push(eqs);
  }
  if (get_global('enable_fft')) {
    ffts = new fft_2chan();
    stages.push(ffts);
  }

  for (var i = 1; i < stages.length; i++) {
    connect(stages[i - 1], stages[i]);
  }

  function input(n) {
    return stages[0].input(0);
  }

  function output(n) {
    return stages[stages.length - 1].output(0);
  }

  function config(name, value) {
    var p = name[0];
    var s = name.slice(1);
    if (p == 'global') {
      /* do nothing */
    } else if (p == 'drc') {
      if (drcs) {
        drcs.config(s, value);
      }
    } else if (p == 'eq') {
      if (eqs) {
        eqs.config(s, value);
      }
    } else {
      console.log('invalid parameter: name =', name, 'value =', value);
    }
  }

  this.input = input;
  this.output = output;
  this.config = config;
}

/* Returns the fft component for two channels */
function fft_2chan() {
  var splitter = audioContext.createChannelSplitter(2);
  var merger = audioContext.createChannelMerger(2);

  analyzer_left = audioContext.createAnalyser();
  analyzer_right = audioContext.createAnalyser();
  analyzer_left.fftSize = FFT_SIZE;
  analyzer_right.fftSize = FFT_SIZE;

  splitter.connect(analyzer_left, 0, 0);
  splitter.connect(analyzer_right, 1, 0);
  analyzer_left.connect(merger, 0, 0);
  analyzer_right.connect(merger, 0, 1);

  function input(n) {
    return [pin(splitter)];
  }

  function output(n) {
    return [pin(merger)];
  }

  this.input = input;
  this.output = output;
}

/* Returns eq for two channels */
function eq_2chan() {
  var eqcs = [new eq_channel(0), new eq_channel(1)];
  var splitter = audioContext.createChannelSplitter(2);
  var merger = audioContext.createChannelMerger(2);

  connect_from_native(pin(splitter, 0), eqcs[0]);
  connect_from_native(pin(splitter, 1), eqcs[1]);
  connect_to_native(eqcs[0], pin(merger, 0));
  connect_to_native(eqcs[1], pin(merger, 1));

  function input(n) {
    return [pin(splitter)];
  }

  function output(n) {
    return [pin(merger)];
  }

  function config(name, value) {
    var p = parseInt(name[0]);
    var s = name.slice(1);
    eqcs[p].config(s, value);
  }

  this.input = input;
  this.output = output;
  this.config = config;
}

/* Returns eq for one channel (left or right). It contains a series of eq
 * filters.  */
function eq_channel(channel) {
  var eqs = [];
  var first = new delay(0);
  var last = first;
  for (var i = 0; i < NEQ; i++) {
    eqs.push(new eq());
    if (get_config('eq', channel, i, 'enable')) {
      connect(last, eqs[i]);
      last = eqs[i];
    }
  }

  function input(n) {
    return first.input(0);
  }

  function output(n) {
    return last.output(0);
  }

  function config(name, value) {
    var p = parseInt(name[0]);
    var s = name.slice(1);
    eqs[p].config(s, value);
  }

  this.input = input;
  this.output = output;
  this.config = config;
}

/* Returns a delay component (output = input with n seconds delay) */
function delay(n) {
  var delay = audioContext.createDelay();
  delay.delayTime.value = n;

  function input(n) {
    return [pin(delay)];
  }

  function output(n) {
    return [pin(delay)];
  }

  function config(name, value) {
    console.log('invalid parameter: name =', name, 'value =', value);
  }

  this.input = input;
  this.output = output;
  this.config = config;
}

/* Returns an eq filter */
function eq() {
  var filter = audioContext.createBiquadFilter();
  filter.type = INIT_EQ_TYPE;
  filter.frequency.value = INIT_EQ_FREQ;
  filter.Q.value = INIT_EQ_Q;
  filter.gain.value = INIT_EQ_GAIN;

  function input(n) {
    return [pin(filter)];
  }

  function output(n) {
    return [pin(filter)];
  }

  function config(name, value) {
    switch (name[0]) {
    case 'type':
      filter.type = value;
      break;
    case 'freq':
      filter.frequency.value = parseFloat(value);
      break;
    case 'q':
      value = parseFloat(value);
      if (filter.type == 'lowpass' || filter.type == 'highpass')
        value = make_biquad_q(value);
      filter.Q.value = value;
      break;
    case 'gain':
      filter.gain.value = parseFloat(value);
      break;
    case 'enable':
      break;
    default:
      console.log('invalid parameter: name =', name, 'value =', value);
    }
  }

  this.input = input;
  this.output = output;
  this.config = config;
}

/* Returns DRC for 3 bands */
function drc_3band() {
  var xo = new xo3();
  var drcs = [new drc(), new drc(), new drc()];

  var out = [];
  for (var i = 0; i < 3; i++) {
    if (get_config('drc', i, 'enable')) {
      connect(xo, drcs[i], i);
      out = out.concat(drcs[i].output());
    } else {
      /* The DynamicsCompressorNode in Chrome has 6ms pre-delay buffer. So for
       * other bands we need to delay for the same amount of time.
       */
      var d = new delay(0.006);
      connect(xo, d, i);
      out = out.concat(d.output());
    }
  }

  function input(n) {
    return xo.input(0);
  }

  function output(n) {
    return out;
  }

  function config(name, value) {
    if (name[1] == 'f') {
      xo.config(name, value);
    } else if (name[0] != 'emphasis_disabled') {
      var n = parseInt(name[0]);
      drcs[n].config(name.slice(1), value);
    }
  }

  this.input = input;
  this.output = output;
  this.config = config;
}


/* This snippet came from LayoutTests/webaudio/dynamicscompressor-simple.html in
 * https://codereview.chromium.org/152333003/. It can determine if
 * emphasis/deemphasis is disabled in the browser. Then it sets the value to
 * drc.emphasis_disabled in the config.*/
function get_emphasis_disabled() {
  var context;
  var sampleRate = 44100;
  var lengthInSeconds = 1;
  var renderedData;
  // This threshold is experimentally determined. It depends on the the gain
  // value of the gain node below and the dynamics compressor.  When the
  // DynamicsCompressor had the pre-emphasis filters, the peak value is about
  // 0.21.  Without it, the peak is 0.85.
  var peakThreshold = 0.85;

  function checkResult(event) {
    var renderedBuffer = event.renderedBuffer;
    renderedData = renderedBuffer.getChannelData(0);
    // Search for a peak in the last part of the data.
    var startSample = sampleRate * (lengthInSeconds - .1);
    var endSample = renderedData.length;
    var k;
    var peak = -1;
    var emphasis_disabled = 0;

    for (k = startSample; k < endSample; ++k) {
      var sample = Math.abs(renderedData[k]);
      if (peak < sample)
         peak = sample;
    }

    if (peak >= peakThreshold) {
      console.log("Pre-emphasis effect not applied as expected..");
      emphasis_disabled = 1;
    } else {
      console.log("Pre-emphasis caused output to be decreased to " + peak
                 + " (expected >= " + peakThreshold + ")");
      emphasis_disabled = 0;
    }
    browser_emphasis_disabled_detection_result = emphasis_disabled;
    /* save_config button will be disabled until we can decide
       emphasis_disabled in chrome. */
    document.getElementById('save_config').disabled = false;
  }

  function runTest() {
    context = new OfflineAudioContext(1, sampleRate * lengthInSeconds,
                                      sampleRate);
    // Connect an oscillator to a gain node to the compressor.  The
    // oscillator frequency is set to a high value for the (original)
    // emphasis to kick in. The gain is a little extra boost to get the
    // compressor enabled.
    //
    var osc = context.createOscillator();
    osc.frequency.value = 15000;
    var gain = context.createGain();
    gain.gain.value = 1.5;
    var compressor = context.createDynamicsCompressor();
    osc.connect(gain);
    gain.connect(compressor);
    compressor.connect(context.destination);
    osc.start();
    context.oncomplete = checkResult;
    context.startRendering();
  }

  runTest();

}

/* Returns one DRC filter */
function drc() {
  var comp = audioContext.createDynamicsCompressor();

  /* The supported method names are different on browsers with different
   * versions.*/
  audioContext.createGainNode = (audioContext.createGainNode ||
                                 audioContext.createGain);
  var boost = audioContext.createGainNode();
  comp.threshold.value = INIT_DRC_THRESHOLD;
  comp.knee.value = INIT_DRC_KNEE;
  comp.ratio.value = INIT_DRC_RATIO;
  comp.attack.value = INIT_DRC_ATTACK;
  comp.release.value = INIT_DRC_RELEASE;
  boost.gain.value = dBToLinear(INIT_DRC_BOOST);

  comp.connect(boost);

  function input(n) {
    return [pin(comp)];
  }

  function output(n) {
    return [pin(boost)];
  }

  function config(name, value) {
    var p = name[0];
    switch (p) {
    case 'threshold':
    case 'knee':
    case 'ratio':
    case 'attack':
    case 'release':
      comp[p].value = parseFloat(value);
      break;
    case 'boost':
      boost.gain.value = dBToLinear(parseFloat(value));
      break;
    case 'enable':
      break;
    default:
      console.log('invalid parameter: name =', name, 'value =', value);
    }
  }

  this.input = input;
  this.output = output;
  this.config = config;
}

/* Crossover filter
 *
 * INPUT --+-- lp1 --+-- lp2a --+-- LOW (0)
 *         |         |          |
 *         |         \-- hp2a --/
 *         |
 *         \-- hp1 --+-- lp2 ------ MID (1)
 *                   |
 *                   \-- hp2 ------ HIGH (2)
 *
 *            [f1]       [f2]
 */

/* Returns a crossover component which splits input into 3 bands */
function xo3() {
  var f1 = INIT_DRC_XO_LOW;
  var f2 = INIT_DRC_XO_HIGH;

  var lp1 = lr4_lowpass(f1);
  var hp1 = lr4_highpass(f1);
  var lp2 = lr4_lowpass(f2);
  var hp2 = lr4_highpass(f2);
  var lp2a = lr4_lowpass(f2);
  var hp2a = lr4_highpass(f2);

  connect(lp1, lp2a);
  connect(lp1, hp2a);
  connect(hp1, lp2);
  connect(hp1, hp2);

  function input(n) {
    return lp1.input().concat(hp1.input());
  }

  function output(n) {
    switch (n) {
    case 0:
      return lp2a.output().concat(hp2a.output());
    case 1:
      return lp2.output();
    case 2:
      return hp2.output();
    default:
      console.log('invalid index ' + n);
      return [];
    }
  }

  function config(name, value) {
    var p = name[0];
    var s = name.slice(1);
    if (p == '0') {
      /* Ignore. The lower frequency of the low band is always 0. */
    } else if (p == '1') {
      lp1.config(s, value);
      hp1.config(s, value);
    } else if (p == '2') {
      lp2.config(s, value);
      hp2.config(s, value);
      lp2a.config(s, value);
      hp2a.config(s, value);
    } else {
      console.log('invalid parameter: name =', name, 'value =', value);
    }
  }

  this.output = output;
  this.input = input;
  this.config = config;
}

/* Connects two components: the n-th output of c1 and the m-th input of c2. */
function connect(c1, c2, n, m) {
  n = n || 0; /* default is the first output */
  m = m || 0; /* default is the first input */
  outs = c1.output(n);
  ins = c2.input(m);

  for (var i = 0; i < outs.length; i++) {
    for (var j = 0; j < ins.length; j++) {
      var from = outs[i];
      var to = ins[j];
      from.node.connect(to.node, from.index, to.index);
    }
  }
}

/* Connects from pin "from" to the n-th input of component c2 */
function connect_from_native(from, c2, n) {
  n = n || 0;  /* default is the first input */
  ins = c2.input(n);
  for (var i = 0; i < ins.length; i++) {
    var to = ins[i];
    from.node.connect(to.node, from.index, to.index);
  }
}

/* Connects from m-th output of component c1 to pin "to" */
function connect_to_native(c1, to, m) {
  m = m || 0;  /* default is the first output */
  outs = c1.output(m);
  for (var i = 0; i < outs.length; i++) {
    var from = outs[i];
    from.node.connect(to.node, from.index, to.index);
  }
}

/* Returns a LR4 lowpass component */
function lr4_lowpass(freq) {
  return new double(freq, create_lowpass);
}

/* Returns a LR4 highpass component */
function lr4_highpass(freq) {
  return new double(freq, create_highpass);
}

/* Returns a component by apply the same filter twice. */
function double(freq, creator) {
  var f1 = creator(freq);
  var f2 = creator(freq);
  f1.connect(f2);

  function input(n) {
    return [pin(f1)];
  }

  function output(n) {
    return [pin(f2)];
  }

  function config(name, value) {
    if (name[0] == 'f') {
      f1.frequency.value = parseFloat(value);
      f2.frequency.value = parseFloat(value);
    } else {
      console.log('invalid parameter: name =', name, 'value =', value);
    }
  }

  this.input = input;
  this.output = output;
  this.config = config;
}

/* Returns a lowpass filter */
function create_lowpass(freq) {
  var lp = audioContext.createBiquadFilter();
  lp.type = 'lowpass';
  lp.frequency.value = freq;
  lp.Q.value = make_biquad_q(0);
  return lp;
}

/* Returns a highpass filter */
function create_highpass(freq) {
  var hp = audioContext.createBiquadFilter();
  hp.type = 'highpass';
  hp.frequency.value = freq;
  hp.Q.value = make_biquad_q(0);
  return hp;
}

/* A pin specifies one of the input/output of a Web Audio node */
function pin(node, index) {
  var p = new Pin();
  p.node = node;
  p.index = index || 0;
  return p;
}

function Pin(node, index) {
}

/* ============================ Event Handlers ============================ */

function audio_source_select(select) {
  var index = select.selectedIndex;
  var url = document.getElementById('audio_source_url');
  url.value = select.options[index].value;
  url.blur();
  audio_source_set(url.value);
}

/* Loads a local audio file. */
function load_audio() {
  document.getElementById('audio_file').click();
}

function audio_file_changed() {
  var input = document.getElementById('audio_file');
  var file = input.files[0];
  var file_url = window.webkitURL.createObjectURL(file);
  input.value = '';

  var url = document.getElementById('audio_source_url');
  url.value = file.name;

  audio_source_set(file_url);
}

function audio_source_set(url) {
  var player = document.getElementById('audio_player');
  var container = document.getElementById('audio_player_container');
  var loading = document.getElementById('audio_loading');
  loading.style.visibility = 'visible';

  /* Re-create an audio element when the audio source URL is changed. */
  player.pause();
  container.removeChild(player);
  player = document.createElement('audio');
  player.crossOrigin = 'anonymous';
  player.id = 'audio_player';
  player.loop = true;
  player.controls = true;
  player.addEventListener('canplay', audio_source_canplay);
  container.appendChild(player);
  update_source_node(player);

  player.src = url;
  player.load();
}

function audio_source_canplay() {
  var player = document.getElementById('audio_player');
  var loading = document.getElementById('audio_loading');
  loading.style.visibility = 'hidden';
  player.play();
}

function update_source_node(mediaElement) {
  sourceNode = audioContext.createMediaElementSource(mediaElement);
  build_graph();
}

function toggle_global_checkbox(name, enable) {
  use_config('global', name, enable);
  build_graph();
}

function toggle_one_drc(index, enable) {
  use_config('drc', index, 'enable', enable);
  build_graph();
}

function toggle_one_eq(channel, index, enable) {
  use_config('eq', channel, index, 'enable', enable);
  build_graph();
}

/* ============================== UI widgets ============================== */

/* Adds a row to the table. The row contains an input box and a slider. */
function slider_input(table, name, initial_value, min_value, max_value, step,
                      suffix, handler) {
  function id(x) {
    return x;
  }

  return new slider_input_common(table, name, initial_value, min_value,
                                 max_value, step, suffix, handler, id, id);
}

/* This is similar to slider_input, but uses log scale for the slider. */
function slider_input_log(table, name, initial_value, min_value, max_value,
                          suffix, precision, handler, mapping,
                          inverse_mapping) {
  function mapping(x) {
    return Math.log(x + 1);
  }

  function inv_mapping(x) {
    return (Math.exp(x) - 1).toFixed(precision);
  }

  return new slider_input_common(table, name, initial_value, min_value,
                                 max_value, 1e-6, suffix, handler, mapping,
                                 inv_mapping);
}

/* The common implementation of linear and log-scale sliders. Each slider has
 * the following methods:
 *
 * function update(v) - update the slider (and the text box) to the value v.
 *
 * function hide(h) - hide/unhide the slider.
 */
function slider_input_common(table, name, initial_value, min_value, max_value,
                             step, suffix, handler, mapping, inv_mapping) {
  var row = table.insertRow(-1);
  var col_name = row.insertCell(-1);
  var col_box = row.insertCell(-1);
  var col_slider = row.insertCell(-1);

  var name_span = document.createElement('span');
  name_span.appendChild(document.createTextNode(name));
  col_name.appendChild(name_span);

  var box = document.createElement('input');
  box.defaultValue = initial_value;
  box.type = 'text';
  box.size = 5;
  box.className = 'nbox';
  col_box.appendChild(box);
  var suffix_span = document.createElement('span');
  suffix_span.appendChild(document.createTextNode(suffix));
  col_box.appendChild(suffix_span);

  var slider = document.createElement('input');
  slider.defaultValue = Math.log(initial_value);
  slider.type = 'range';
  slider.className = 'nslider';
  slider.min = mapping(min_value);
  slider.max = mapping(max_value);
  slider.step = step;
  col_slider.appendChild(slider);

  box.onchange = function() {
    slider.value = mapping(box.value);
    handler(parseFloat(box.value));
  };

  slider.onchange = function() {
    box.value = inv_mapping(slider.value);
    handler(parseFloat(box.value));
  };

  function update(v) {
    box.value = v;
    slider.value = mapping(v);
  }

  function hide(h) {
    var v = h ? 'hidden' : 'visible';
    name_span.style.visibility = v;
    box.style.visibility = v;
    suffix_span.style.visibility = v;
    slider.style.visibility = v;
  }

  this.update = update;
  this.hide = hide;
}

/* Adds a enable/disable checkbox to a div. The method "update" can change the
 * checkbox state. */
function check_button(div, handler) {
  var check = document.createElement('input');
  check.className = 'enable_check';
  check.type = 'checkbox';
  check.checked = true;
  check.onchange = function() {
    handler(check.checked);
  };
  div.appendChild(check);

  function update(v) {
    check.checked = v;
  }

  this.update = update;
}

function dummy() {
}

/* Changes the opacity of a div. */
function toggle_card(div, enable) {
  div.style.opacity = enable ? 1 : 0.3;
}

/* Appends a card of DRC controls and graphs to the specified parent.
 * Args:
 *     parent - The parent element
 *     index - The index of this DRC component (0-2)
 *     lower_freq - The lower frequency of this DRC component
 *     freq_label - The label for the lower frequency input text box
 */
function drc_card(parent, index, lower_freq, freq_label) {
  var top = document.createElement('div');
  top.className = 'drc_data';
  parent.appendChild(top);
  function toggle_drc_card(enable) {
    toggle_card(div, enable);
    toggle_one_drc(index, enable);
  }
  var enable_button = new check_button(top, toggle_drc_card);

  var div = document.createElement('div');
  top.appendChild(div);

  /* Canvas */
  var p = document.createElement('p');
  div.appendChild(p);

  var canvas = document.createElement('canvas');
  canvas.className = 'drc_curve';
  p.appendChild(canvas);

  canvas.width = 240;
  canvas.height = 180;
  var dd = new DrcDrawer(canvas);
  dd.init();

  /* Parameters */
  var table = document.createElement('table');
  div.appendChild(table);

  function change_lower_freq(v) {
    use_config('drc', index, 'f', v);
  }

  function change_threshold(v) {
    dd.update_threshold(v);
    use_config('drc', index, 'threshold', v);
  }

  function change_knee(v) {
    dd.update_knee(v);
    use_config('drc', index, 'knee', v);
  }

  function change_ratio(v) {
    dd.update_ratio(v);
    use_config('drc', index, 'ratio', v);
  }

  function change_boost(v) {
    dd.update_boost(v);
    use_config('drc', index, 'boost', v);
  }

  function change_attack(v) {
    use_config('drc', index, 'attack', v);
  }

  function change_release(v) {
    use_config('drc', index, 'release', v);
  }

  var f_slider;
  if (lower_freq == 0) {  /* Special case for the lowest band */
    f_slider = new slider_input_log(table, freq_label, lower_freq, 0, 1,
                                    'Hz', 0, dummy);
    f_slider.hide(true);
  } else {
    f_slider = new slider_input_log(table, freq_label, lower_freq, 1,
                                    nyquist, 'Hz', 0, change_lower_freq);
  }

  var sliders = {
    'f': f_slider,
    'threshold': new slider_input(table, 'Threshold', INIT_DRC_THRESHOLD,
                                  -100, 0, 1, 'dB', change_threshold),
    'knee': new slider_input(table, 'Knee', INIT_DRC_KNEE, 0, 40, 1, 'dB',
                             change_knee),
    'ratio': new slider_input(table, 'Ratio', INIT_DRC_RATIO, 1, 20, 0.001,
                              '', change_ratio),
    'boost': new slider_input(table, 'Boost', 0, -40, 40, 1, 'dB',
                              change_boost),
    'attack': new slider_input(table, 'Attack', INIT_DRC_ATTACK, 0.001,
                               1, 0.001, 's', change_attack),
    'release': new slider_input(table, 'Release', INIT_DRC_RELEASE,
                                0.001, 1, 0.001, 's', change_release)
  };

  function config(name, value) {
    var p = name[0];
    var fv = parseFloat(value);
    switch (p) {
    case 'f':
    case 'threshold':
    case 'knee':
    case 'ratio':
    case 'boost':
    case 'attack':
    case 'release':
      sliders[p].update(fv);
      break;
    case 'enable':
      toggle_card(div, value);
      enable_button.update(value);
      break;
    default:
      console.log('invalid parameter: name =', name, 'value =', value);
    }

    switch (p) {
    case 'threshold':
      dd.update_threshold(fv);
      break;
    case 'knee':
      dd.update_knee(fv);
      break;
    case 'ratio':
      dd.update_ratio(fv);
      break;
    case 'boost':
      dd.update_boost(fv);
      break;
    }
  }

  this.config = config;
}

/* Appends a menu of biquad types to the specified table. */
function biquad_type_select(table, handler) {
  var row = table.insertRow(-1);
  var col_name = row.insertCell(-1);
  var col_menu = row.insertCell(-1);

  col_name.appendChild(document.createTextNode('Type'));

  var select = document.createElement('select');
  select.className = 'biquad_type_select';
  var options = [
    'lowpass',
    'highpass',
    'bandpass',
    'lowshelf',
    'highshelf',
    'peaking',
    'notch'
    /* no need: 'allpass' */
  ];

  for (var i = 0; i < options.length; i++) {
    var o = document.createElement('option');
    o.appendChild(document.createTextNode(options[i]));
    select.appendChild(o);
  }

  select.value = INIT_EQ_TYPE;
  col_menu.appendChild(select);

  function onchange() {
    handler(select.value);
  }
  select.onchange = onchange;

  function update(v) {
    select.value = v;
  }

  this.update = update;
}

/* Appends a card of EQ controls to the specified parent.
 * Args:
 *     parent - The parent element
 *     channel - The index of the channel this EQ component is on (0-1)
 *     index - The index of this EQ on this channel (0-7)
 *     ed - The EQ curve drawer. We will notify the drawer to redraw if the
 *         parameters for this EQ changes.
 */
function eq_card(parent, channel, index, ed) {
  var top = document.createElement('div');
  top.className = 'eq_data';
  parent.appendChild(top);
  function toggle_eq_card(enable) {
    toggle_card(table, enable);
    toggle_one_eq(channel, index, enable);
    ed.update_enable(index, enable);
  }
  var enable_button = new check_button(top, toggle_eq_card);

  var table = document.createElement('table');
  table.className = 'eq_table';
  top.appendChild(table);

  function change_type(v) {
    ed.update_type(index, v);
    hide_unused_slider(v);
    use_config('eq', channel, index, 'type', v);
    /* Special case: automatically set Q to 0 for lowpass/highpass filters. */
    if (v == 'lowpass' || v == 'highpass') {
      use_config('eq', channel, index, 'q', 0);
    }
  }

  function change_freq(v)
  {
    ed.update_freq(index, v);
    use_config('eq', channel, index, 'freq', v);
  }

  function change_q(v)
  {
    ed.update_q(index, v);
    use_config('eq', channel, index, 'q', v);
  }

  function change_gain(v)
  {
    ed.update_gain(index, v);
    use_config('eq', channel, index, 'gain', v);
  }

  var type_select = new biquad_type_select(table, change_type);

  var sliders = {
    'freq': new slider_input_log(table, 'Frequency', INIT_EQ_FREQ, 1,
                                 nyquist, 'Hz', 0, change_freq),
    'q': new slider_input_log(table, 'Q', INIT_EQ_Q, 0, 1000, '', 4,
                              change_q),
    'gain': new slider_input(table, 'Gain', INIT_EQ_GAIN, -40, 40, 0.1,
                             'dB', change_gain)
  };

  var unused = {
    'lowpass': [0, 0, 1],
    'highpass': [0, 0, 1],
    'bandpass': [0, 0, 1],
    'lowshelf': [0, 1, 0],
    'highshelf': [0, 1, 0],
    'peaking': [0, 0, 0],
    'notch': [0, 0, 1],
    'allpass': [0, 0, 1]
  };
  function hide_unused_slider(type) {
    var u = unused[type];
    sliders['freq'].hide(u[0]);
    sliders['q'].hide(u[1]);
    sliders['gain'].hide(u[2]);
  }

  function config(name, value) {
    var p = name[0];
    var fv = parseFloat(value);
    switch (p) {
    case 'type':
      type_select.update(value);
      break;
    case 'freq':
    case 'q':
    case 'gain':
      sliders[p].update(fv);
      break;
    case 'enable':
      toggle_card(table, value);
      enable_button.update(value);
      break;
    default:
      console.log('invalid parameter: name =', name, 'value =', value);
    }

    switch (p) {
    case 'type':
      ed.update_type(index, value);
      hide_unused_slider(value);
      break;
    case 'freq':
      ed.update_freq(index, fv);
      break;
    case 'q':
      ed.update_q(index, fv);
      break;
    case 'gain':
      ed.update_gain(index, fv);
      break;
    }
  }

  this.config = config;
}

/* Appends the EQ UI for one channel to the specified parent */
function eq_section(parent, channel) {
  /* Two canvas, one for eq curve, another for fft. */
  var p = document.createElement('p');
  p.className = 'eq_curve_parent';

  var canvas_eq = document.createElement('canvas');
  canvas_eq.className = 'eq_curve';
  canvas_eq.width = 960;
  canvas_eq.height = 270;

  p.appendChild(canvas_eq);
  var ed = new EqDrawer(canvas_eq, channel);
  ed.init();

  var canvas_fft = document.createElement('canvas');
  canvas_fft.className = 'eq_curve';
  canvas_fft.width = 960;
  canvas_fft.height = 270;

  p.appendChild(canvas_fft);
  var fd = new FFTDrawer(canvas_fft, channel);
  fd.init();

  parent.appendChild(p);

  /* Eq cards */
  var eq = {};
  for (var i = 0; i < NEQ; i++) {
    eq[i] = new eq_card(parent, channel, i, ed);
  }

  function config(name, value) {
    var p = parseInt(name[0]);
    var s = name.slice(1);
    eq[p].config(s, value);
  }

  this.config = config;
}

function global_section(parent) {
  var checkbox_data = [
    /* config name, text label, checkbox object */
    ['enable_drc', 'Enable DRC', null],
    ['enable_eq', 'Enable EQ', null],
    ['enable_fft', 'Show FFT', null],
    ['enable_swap', 'Swap DRC/EQ', null]
  ];

  for (var i = 0; i < checkbox_data.length; i++) {
    config_name = checkbox_data[i][0];
    text_label = checkbox_data[i][1];

    var cb = document.createElement('input');
    cb.type = 'checkbox';
    cb.checked = get_global(config_name);
    cb.onchange = function(name) {
      return function() { toggle_global_checkbox(name, this.checked); }
    }(config_name);
    checkbox_data[i][2] = cb;
    parent.appendChild(cb);
    parent.appendChild(document.createTextNode(text_label));
  }

  function config(name, value) {
    var i;
    for (i = 0; i < checkbox_data.length; i++) {
      if (checkbox_data[i][0] == name[0]) {
        break;
      }
    }
    if (i < checkbox_data.length) {
      checkbox_data[i][2].checked = value;
    } else {
      console.log('invalid parameter: name =', name, 'value =', value);
    }
  }

  this.config = config;
}

window.onload = function() {
  fix_audio_elements();
  check_biquad_filter_q().then(function (flag) {
    console.log('Browser biquad filter uses Audio Cookbook formula:', flag);
    /* Detects if emphasis is disabled and sets
     * browser_emphasis_disabled_detection_result. */
    get_emphasis_disabled();
    init_config();
    init_audio();
    init_ui();
  }).catch(function (reason) {
    alert('Cannot detect browser biquad filter implementation:', reason);
  });
};

function init_ui() {
  audio_ui = new ui();
}

/* Top-level UI */
function ui() {
  var global = new global_section(document.getElementById('global_section'));
  var drc_div = document.getElementById('drc_section');
  var drc_cards = [
    new drc_card(drc_div, 0, 0, ''),
    new drc_card(drc_div, 1, INIT_DRC_XO_LOW, 'Start From'),
    new drc_card(drc_div, 2, INIT_DRC_XO_HIGH, 'Start From')
  ];

  var left_div = document.getElementById('eq_left_section');
  var right_div = document.getElementById('eq_right_section');
  var eq_sections = [
    new eq_section(left_div, 0),
    new eq_section(right_div, 1)
  ];

  function config(name, value) {
    var p = name[0];
    var i = parseInt(name[1]);
    var s = name.slice(2);
    if (p == 'global') {
      global.config(name.slice(1), value);
    } else if (p == 'drc') {
      if (name[1] == 'emphasis_disabled') {
        return;
      }
      drc_cards[i].config(s, value);
    } else if (p == 'eq') {
      eq_sections[i].config(s, value);
    } else {
      console.log('invalid parameter: name =', name, 'value =', value);
    }
  }

  this.config = config;
}

/* Draws the DRC curve on a canvas. The update*() methods should be called when
 * the parameters change, so the curve can be redrawn. */
function DrcDrawer(canvas) {
  var canvasContext = canvas.getContext('2d');

  var backgroundColor = 'black';
  var curveColor = 'rgb(192,192,192)';
  var gridColor = 'rgb(200,200,200)';
  var textColor = 'rgb(238,221,130)';
  var thresholdColor = 'rgb(255,160,122)';

  var dbThreshold = INIT_DRC_THRESHOLD;
  var dbKnee = INIT_DRC_KNEE;
  var ratio = INIT_DRC_RATIO;
  var boost = INIT_DRC_BOOST;

  var curve_slope;
  var curve_k;
  var linearThreshold;
  var kneeThresholdDb;
  var kneeThreshold;
  var ykneeThresholdDb;
  var masterLinearGain;

  var maxOutputDb = 6;
  var minOutputDb = -36;

  function xpixelToDb(x) {
    /* This is right even though it looks like we should scale by width. We
     * want the same pixel/dB scale for both. */
    var k = x / canvas.height;
    var db = minOutputDb + k * (maxOutputDb - minOutputDb);
    return db;
  }

  function dBToXPixel(db) {
    var k = (db - minOutputDb) / (maxOutputDb - minOutputDb);
    var x = k * canvas.height;
    return x;
  }

  function ypixelToDb(y) {
    var k = y / canvas.height;
    var db = maxOutputDb - k * (maxOutputDb - minOutputDb);
    return db;
  }

  function dBToYPixel(db) {
    var k = (maxOutputDb - db) / (maxOutputDb - minOutputDb);
    var y = k * canvas.height;
    return y;
  }

  function kneeCurve(x, k) {
    if (x < linearThreshold)
      return x;

    return linearThreshold +
        (1 - Math.exp(-k * (x - linearThreshold))) / k;
  }

  function saturate(x, k) {
    var y;
    if (x < kneeThreshold) {
      y = kneeCurve(x, k);
    } else {
      var xDb = linearToDb(x);
      var yDb = ykneeThresholdDb + curve_slope * (xDb - kneeThresholdDb);
      y = dBToLinear(yDb);
    }
    return y;
  }

  function slopeAt(x, k) {
    if (x < linearThreshold)
      return 1;
    var x2 = x * 1.001;
    var xDb = linearToDb(x);
    var x2Db = linearToDb(x2);
    var yDb = linearToDb(kneeCurve(x, k));
    var y2Db = linearToDb(kneeCurve(x2, k));
    var m = (y2Db - yDb) / (x2Db - xDb);
    return m;
  }

  function kAtSlope(desiredSlope) {
    var xDb = dbThreshold + dbKnee;
    var x = dBToLinear(xDb);

    var minK = 0.1;
    var maxK = 10000;
    var k = 5;

    for (var i = 0; i < 15; i++) {
      var slope = slopeAt(x, k);
      if (slope < desiredSlope) {
        maxK = k;
      } else {
        minK = k;
      }
      k = Math.sqrt(minK * maxK);
    }
    return k;
  }

  function drawCurve() {
    /* Update curve parameters */
    linearThreshold = dBToLinear(dbThreshold);
    curve_slope = 1 / ratio;
    curve_k = kAtSlope(1 / ratio);
    kneeThresholdDb = dbThreshold + dbKnee;
    kneeThreshold = dBToLinear(kneeThresholdDb);
    ykneeThresholdDb = linearToDb(kneeCurve(kneeThreshold, curve_k));

    /* Calculate masterLinearGain */
    var fullRangeGain = saturate(1, curve_k);
    var fullRangeMakeupGain = Math.pow(1 / fullRangeGain, 0.6);
    masterLinearGain = dBToLinear(boost) * fullRangeMakeupGain;

    /* Clear canvas */
    var width = canvas.width;
    var height = canvas.height;
    canvasContext.fillStyle = backgroundColor;
    canvasContext.fillRect(0, 0, width, height);

    /* Draw linear response for reference. */
    canvasContext.strokeStyle = gridColor;
    canvasContext.lineWidth = 1;
    canvasContext.beginPath();
    canvasContext.moveTo(dBToXPixel(minOutputDb), dBToYPixel(minOutputDb));
    canvasContext.lineTo(dBToXPixel(maxOutputDb), dBToYPixel(maxOutputDb));
    canvasContext.stroke();

    /* Draw 0dBFS output levels from 0dBFS down to -36dBFS */
    for (var dbFS = 0; dbFS >= -36; dbFS -= 6) {
      canvasContext.beginPath();

      var y = dBToYPixel(dbFS);
      canvasContext.setLineDash([1, 4]);
      canvasContext.moveTo(0, y);
      canvasContext.lineTo(width, y);
      canvasContext.stroke();
      canvasContext.setLineDash([]);

      canvasContext.textAlign = 'center';
      canvasContext.strokeStyle = textColor;
      canvasContext.strokeText(dbFS.toFixed(0) + ' dB', 15, y - 2);
      canvasContext.strokeStyle = gridColor;
    }

    /* Draw 0dBFS input line */
    canvasContext.beginPath();
    canvasContext.moveTo(dBToXPixel(0), 0);
    canvasContext.lineTo(dBToXPixel(0), height);
    canvasContext.stroke();
    canvasContext.strokeText('0dB', dBToXPixel(0), height);

    /* Draw threshold input line */
    canvasContext.beginPath();
    canvasContext.moveTo(dBToXPixel(dbThreshold), 0);
    canvasContext.lineTo(dBToXPixel(dbThreshold), height);
    canvasContext.moveTo(dBToXPixel(kneeThresholdDb), 0);
    canvasContext.lineTo(dBToXPixel(kneeThresholdDb), height);
    canvasContext.strokeStyle = thresholdColor;
    canvasContext.stroke();

    /* Draw the compressor curve */
    canvasContext.strokeStyle = curveColor;
    canvasContext.lineWidth = 3;

    canvasContext.beginPath();
    var pixelsPerDb = (0.5 * height) / 40.0;

    for (var x = 0; x < width; ++x) {
      var inputDb = xpixelToDb(x);
      var inputLinear = dBToLinear(inputDb);
      var outputLinear = saturate(inputLinear, curve_k);
      outputLinear *= masterLinearGain;
      var outputDb = linearToDb(outputLinear);
      var y = dBToYPixel(outputDb);

      canvasContext.lineTo(x, y);
    }
    canvasContext.stroke();

  }

  function init() {
    drawCurve();
  }

  function update_threshold(v)
  {
    dbThreshold = v;
    drawCurve();
  }

  function update_knee(v)
  {
    dbKnee = v;
    drawCurve();
  }

  function update_ratio(v)
  {
    ratio = v;
    drawCurve();
  }

  function update_boost(v)
  {
    boost = v;
    drawCurve();
  }

  this.init = init;
  this.update_threshold = update_threshold;
  this.update_knee = update_knee;
  this.update_ratio = update_ratio;
  this.update_boost = update_boost;
}

/* Draws the EQ curve on a canvas. The update*() methods should be called when
 * the parameters change, so the curve can be redrawn. */
function EqDrawer(canvas, channel) {
  var canvasContext = canvas.getContext('2d');
  var curveColor = 'rgb(192,192,192)';
  var gridColor = 'rgb(200,200,200)';
  var textColor = 'rgb(238,221,130)';
  var centerFreq = {};
  var q = {};
  var gain = {};

  for (var i = 0; i < NEQ; i++) {
    centerFreq[i] = INIT_EQ_FREQ;
    q[i] = INIT_EQ_Q;
    gain[i] = INIT_EQ_GAIN;
  }

  function drawCurve() {
    /* Create a biquad node to calculate frequency response. */
    var filter = audioContext.createBiquadFilter();
    var width = canvas.width;
    var height = canvas.height;
    var pixelsPerDb = height / 48.0;
    var noctaves = 10;

    /* Prepare the frequency array */
    var frequencyHz = new Float32Array(width);
    for (var i = 0; i < width; ++i) {
      var f = i / width;

      /* Convert to log frequency scale (octaves). */
      f = Math.pow(2.0, noctaves * (f - 1.0));
      frequencyHz[i] = f * nyquist;
    }

    /* Get the response */
    var magResponse = new Float32Array(width);
    var phaseResponse = new Float32Array(width);
    var totalMagResponse = new Float32Array(width);

    for (var i = 0; i < width; i++) {
      totalMagResponse[i] = 1;
    }

    for (var i = 0; i < NEQ; i++) {
      if (!get_config('eq', channel, i, 'enable')) {
        continue;
      }
      filter.type = get_config('eq', channel, i, 'type');
      filter.frequency.value = centerFreq[i];
      if (filter.type == 'lowpass' || filter.type == 'highpass')
        filter.Q.value = make_biquad_q(q[i]);
      else
        filter.Q.value = q[i];
      filter.gain.value = gain[i];
      filter.getFrequencyResponse(frequencyHz, magResponse,
                                  phaseResponse);
      for (var j = 0; j < width; j++) {
        totalMagResponse[j] *= magResponse[j];
      }
    }

    /* Draw the response */
    canvasContext.fillStyle = 'rgb(0, 0, 0)';
    canvasContext.fillRect(0, 0, width, height);
    canvasContext.strokeStyle = curveColor;
    canvasContext.lineWidth = 3;
    canvasContext.beginPath();

    for (var i = 0; i < width; ++i) {
      var response = totalMagResponse[i];
      var dbResponse = linearToDb(response);

      var x = i;
      var y = height - (dbResponse + 24) * pixelsPerDb;

      canvasContext.lineTo(x, y);
    }
    canvasContext.stroke();

    /* Draw frequency scale. */
    canvasContext.beginPath();
    canvasContext.lineWidth = 1;
    canvasContext.strokeStyle = gridColor;

    for (var octave = 0; octave <= noctaves; octave++) {
      var x = octave * width / noctaves;

      canvasContext.moveTo(x, 30);
      canvasContext.lineTo(x, height);
      canvasContext.stroke();

      var f = nyquist * Math.pow(2.0, octave - noctaves);
      canvasContext.textAlign = 'center';
      canvasContext.strokeText(f.toFixed(0) + 'Hz', x, 20);
    }

    /* Draw 0dB line. */
    canvasContext.beginPath();
    canvasContext.moveTo(0, 0.5 * height);
    canvasContext.lineTo(width, 0.5 * height);
    canvasContext.stroke();

    /* Draw decibel scale. */
    for (var db = -24.0; db < 24.0; db += 6) {
      var y = height - (db + 24) * pixelsPerDb;
      canvasContext.beginPath();
      canvasContext.setLineDash([1, 4]);
      canvasContext.moveTo(0, y);
      canvasContext.lineTo(width, y);
      canvasContext.stroke();
      canvasContext.setLineDash([]);
      canvasContext.strokeStyle = textColor;
      canvasContext.strokeText(db.toFixed(0) + 'dB', width - 20, y);
      canvasContext.strokeStyle = gridColor;
    }
  }

  function update_freq(index, v) {
    centerFreq[index] = v;
    drawCurve();
  }

  function update_q(index, v) {
    q[index] = v;
    drawCurve();
  }

  function update_gain(index, v) {
    gain[index] = v;
    drawCurve();
  }

  function update_enable(index, v) {
    drawCurve();
  }

  function update_type(index, v) {
    drawCurve();
  }

  function init() {
    drawCurve();
  }

  this.init = init;
  this.update_freq = update_freq;
  this.update_q = update_q;
  this.update_gain = update_gain;
  this.update_enable = update_enable;
  this.update_type = update_type;
}

/* Draws the FFT curve on a canvas. This will update continuously when the audio
 * is playing. */
function FFTDrawer(canvas, channel) {
  var canvasContext = canvas.getContext('2d');
  var curveColor = 'rgb(255,160,122)';
  var binCount = FFT_SIZE / 2;
  var data = new Float32Array(binCount);

  function drawCurve() {
    var width = canvas.width;
    var height = canvas.height;
    var pixelsPerDb = height / 96.0;

    canvasContext.clearRect(0, 0, width, height);

    /* Get the proper analyzer from the audio graph */
    var analyzer = (channel == 0) ? analyzer_left : analyzer_right;
    if (!analyzer || !get_global('enable_fft')) {
      requestAnimationFrame(drawCurve);
      return;
    }

    /* Draw decibel scale. */
    for (var db = -96.0; db <= 0; db += 12) {
      var y = height - (db + 96) * pixelsPerDb;
      canvasContext.strokeStyle = curveColor;
      canvasContext.strokeText(db.toFixed(0) + 'dB', 10, y);
    }

    /* Draw FFT */
    analyzer.getFloatFrequencyData(data);
    canvasContext.beginPath();
    canvasContext.lineWidth = 1;
    canvasContext.strokeStyle = curveColor;
    canvasContext.moveTo(0, height);

    var frequencyHz = new Float32Array(width);
    for (var i = 0; i < binCount; ++i) {
      var f = i / binCount;

      /* Convert to log frequency scale (octaves). */
      var noctaves = 10;
      f = 1 + Math.log(f) / (noctaves * Math.LN2);

      /* Draw the magnitude */
      var x = f * width;
      var y = height - (data[i] + 96) * pixelsPerDb;

      canvasContext.lineTo(x, y);
    }

    canvasContext.stroke();
    requestAnimationFrame(drawCurve);
  }

  function init() {
    requestAnimationFrame(drawCurve);
  }

  this.init = init;
}

function dBToLinear(db) {
  return Math.pow(10.0, 0.05 * db);
}

function linearToDb(x) {
  return 20.0 * Math.log(x) / Math.LN10;
}
