/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

var NN = 100;    // Total number of points
var FIXES = 25;  // Number of fixed points, evenly spaced in the range [0, NN]
var minmax_boxes = []; // The text input boxes for min/max/step
var fix_boxes = [];  // The text input boxes for fixed points

window.onload = function() {
  init_minmax();
  init_fixes();
  init_canvas();
};

// Create min/max/step boxes
function init_minmax() {
  var table = document.getElementById('minmax');
  var names = ['Min:' , 'Max:', 'Step:'];
  for (var i = 0; i < names.length; i++) {
    var row = table.insertRow(-1);
    var col_name = row.insertCell(-1);
    var col_box = row.insertCell(-1);
    var col_db = row.insertCell(-1);
    var box = document.createElement('input');
    box.size = 5;
    box.className = 'box';
    col_name.appendChild(document.createTextNode(names[i]));
    col_name.align = 'right';
    col_box.appendChild(box);
    col_db.appendChild(document.createTextNode('dB'));
    minmax_boxes.push(box);
    box.oninput = redraw;
  }
}

// Create fixed point boxes
function init_fixes() {
  var table = document.getElementById('fixes');
  for (var i = 0; i <= FIXES; i++) {
    var row = table.insertRow(-1);
    var col_name = row.insertCell(-1);
    var col_box = row.insertCell(-1);
    var col_db = row.insertCell(-1);
    var box = document.createElement('input');
    box.size = 5;
    box.className = 'box';
    // round fix_pos (the dB value for this fixed point) to one place
    // after decimal point.
    var fix_pos = Math.round(i * NN * 10 / FIXES) / 10;
    col_name.appendChild(document.createTextNode(fix_pos + ':'));
    col_name.align = 'right';
    col_box.appendChild(box);
    col_db.appendChild(document.createTextNode('dB'));
    fix_boxes.push(box);
    box.oninput = redraw;
  }
}

function init_canvas() {
  redraw();
}

// Redraw everything on the canvas. This is run every time any input is changed.
function redraw() {
  var backgroundColor = 'black';
  var gridColor = 'rgb(200,200,200)';
  var dotColor = 'rgb(245,245,0)';
  var marginLeft = 60;
  var marginBottom = 30;
  var marginTop = 20;
  var marginRight = 30;
  var canvas = document.getElementById('curve');
  var ctx = canvas.getContext('2d');
  var w = 800;
  var h = 400;
  canvas.width = w + marginLeft + marginRight;
  canvas.height = h + marginBottom + marginTop;
  ctx.fillStyle = backgroundColor;
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  ctx.lineWidth = 1;
  ctx.font = '16px sans-serif';
  ctx.textAlign = 'center';

  // Set up coordinate system
  ctx.translate(marginLeft, h + marginTop);
  ctx.scale(1, -1);

  // Draw two lines at x = 0 and y = 0 which are solid lines
  ctx.strokeStyle = gridColor;
  ctx.beginPath();
  ctx.moveTo(0, h + marginTop / 2);
  ctx.lineTo(0, 0);
  ctx.lineTo(w + marginRight / 2, 0);
  ctx.stroke();

  // Draw vertical lines and labels on x axis
  ctx.strokeStyle = gridColor;
  ctx.fillStyle = gridColor;
  ctx.beginPath();
  ctx.setLineDash([1, 4]);
  for (var i = 0; i <= FIXES; i++) {
    var x = i * w / FIXES;
    if (i > 0) {
      ctx.moveTo(x, 0);
      ctx.lineTo(x, h + marginTop / 2);
    }
    drawText(ctx, Math.round(i * NN * 10 / FIXES) / 10, x, -20, 'center');
  }
  ctx.stroke();
  ctx.setLineDash([]);

  // Draw horizontal lines and labels on y axis
  var min = parseFloat(minmax_boxes[0].value);
  var max = parseFloat(minmax_boxes[1].value);
  var step = parseFloat(minmax_boxes[2].value);

  // Sanity checks
  if (isNaN(min) || isNaN(max) || isNaN(step)) return;
  if (min >= max || step <= 0 || (max - min) / step > 10000) return;

  // Let s = minimal multiple of step such that
  // vdivs = Math.round((max - min) / s) <= 20
  var vdivs;
  var s = Math.max(1, Math.floor((max - min) / 20 / step)) * step;
  while (true) {
    var vdivs = Math.round((max - min) / s);
    if (vdivs <= 20) break;
    s += step;
  }

  // Scale from v to y is
  // y = (v - min) / s * h / vdivs
  ctx.strokeStyle = gridColor;
  ctx.fillStyle = gridColor;
  ctx.beginPath();
  ctx.setLineDash([1, 4]);
  for (var i = 0;; i++) {
    var v = min + s * i;
    var y;
    if (v <= max) {
      y = i * h / vdivs;
    } else {
      v = max;
      y = (max - min) / s * h / vdivs;
    }
    drawText(ctx, v.toFixed(2), -5 , y - 4, 'right');
    if (i > 0) {
      ctx.moveTo(0, y);
      ctx.lineTo(w + marginRight / 2, y);
    }
    if (v >= max) break;
  }
  ctx.stroke();
  ctx.setLineDash([]);

  // Draw fixed points
  ctx.strokeStyle = dotColor;
  ctx.fillStyle = dotColor;
  for (var i = 0; i <= FIXES; i++) {
    var v = getFix(i);
    if (isNaN(v)) continue;
    var x = i * w / FIXES;
    var y = (v - min) / s * h / vdivs;
    ctx.beginPath();
    ctx.arc(x, y, 4, 0, 2 * Math.PI);
    ctx.stroke();
  }

  // Draw interpolated points
  var points = generatePoints();
  for (var i = 0; i <= NN; i++) {
    var v = points[i];
    if (isNaN(v)) continue;
    var x = i * w / NN;
    var y = (v - min) / s * h / vdivs;
    ctx.beginPath();
    ctx.arc(x, y, 2, 0, 2 * Math.PI);
    ctx.stroke();
    ctx.fill();
  }
}

// Returns the value of the fixed point with index i
function getFix(i) {
  var v = parseFloat(fix_boxes[i].value);
  var min = parseFloat(minmax_boxes[0].value);
  var max = parseFloat(minmax_boxes[1].value);

  if (isNaN(v)) return v;
  if (v > max) v = max;
  if (v < min) v = min;
  return v;
}

// Returns a value quantized to the given min/max/step
function quantize(v) {
  var min = parseFloat(minmax_boxes[0].value);
  var max = parseFloat(minmax_boxes[1].value);
  var step = parseFloat(minmax_boxes[2].value);

  v = min + Math.round((v - min) / step) * step;
  if (isNaN(v)) return v;
  if (v > max) v = max;
  if (v < min) v = min;
  return v;
}

// Generate points indexed by 0 to NN, using interpolation and quantization
function generatePoints() {
  // Go through all points, for each point:
  // (1) Find the left fix: the max defined fixed point <= current point
  // (2) Find the right fix: the min defined fixed point >= current point
  // (3) If both exist, interpolate value for current point
  // (4) Otherwise skip current point

  // Returns left fix index for current point, or NaN if it does not exist
  var find_left = function(current) {
    for (i = FIXES; i >= 0; i--) {
      var x = NN * i / FIXES;
      if (x <= current && !isNaN(getFix(i))) {
        return i;
      }
    }
    return NaN;
  };

  // Returns right fix index for current point, or NaN if it does not exist
  var find_right = function(current) {
    for (i = 0; i <= FIXES; i++) {
      var x = NN * i / FIXES;
      if (x >= current && !isNaN(getFix(i))) {
        return i;
      }
    }
    return NaN;
  };

  // Interpolate value for point x
  var interpolate = function(x) {
    var left = find_left(x);
    if (isNaN(left)) return NaN;

    var right = find_right(x);
    if (isNaN(right)) return NaN;

    var xl = NN * left / FIXES;
    var xr = NN * right / FIXES;
    var yl = getFix(left);
    var yr = getFix(right);

    if (xl == xr) return yl;

    return yl + (yr - yl) * (x - xl) / (xr - xl);
  };

  var result = [];
  for (var x = 0; x <= NN; x++) {
    result.push(quantize(interpolate(x)));
  }
  return result;
}

function drawText(ctx, s, x, y, align) {
  ctx.save();
  ctx.translate(x, y);
  ctx.scale(1, -1);
  ctx.textAlign = align;
  ctx.fillText(s, 0, 0);
  ctx.restore();
}

// The output config file looks like:
//
// [Speaker]
//   volume_curve = explicit
//   db_at_100 = 0
//   db_at_99 = -75
//   db_at_98 = -75
//   ...
//   db_at_1 = -4500
//   db_at_0 = -4800
// [Headphone]
//   volume_curve = simple_step
//   volume_step = 70
//   max_volume = 0
//
function download_config() {
  var content = '';
  content += '[Speaker]\n';
  content += '  volume_curve = explicit\n';
  var points = generatePoints();
  var last = 0;
  for (var i = NN; i >= 0; i--) {
    var v = points[i];
    if (isNaN(points[i])) v = last;
    content += '  db_at_' + i + ' = ' + Math.round(v * 100) + '\n';
  }

  content += '[Headphone]\n';
  content += '  volume_curve = simple_step\n';
  content += '  volume_step = 70\n';
  content += '  max_volume = 0\n';
  save_config(content);
}

function save_config(content) {
  var a = document.getElementById('save_config_anchor');
  var uriContent = 'data:application/octet-stream,' +
      encodeURIComponent(content);
  a.href = uriContent;
  a.download = 'HDA Intel PCH';
  a.click();
}
