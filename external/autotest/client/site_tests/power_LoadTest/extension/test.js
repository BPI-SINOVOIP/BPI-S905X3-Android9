// Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var cycle_tabs = {};
var cycles = {};
var time_ratio = 3600 * 1000 / test_time_ms; // default test time is 1 hour
var preexisting_windows = [];
var log_lines = [];
var error_codes = {}; //for each active tabId
var page_load_times = [];
var page_load_time_counter = {};
var unique_url_salt = 1;

function setupTest() {
  //adding these listeners to track request failure codes
  chrome.webRequest.onCompleted.addListener(capture_completed_status, {urls: ["<all_urls>"]});
  chrome.windows.getAll(null, function(windows) {
    preexisting_windows = windows;
    for (var i = 0; i < tasks.length; i++) {
      setTimeout(launch_task, tasks[i].start / time_ratio, tasks[i]);
    }
    var end = 3600 * 1000 / time_ratio;
    log_lines = [];
    page_load_times = [];
    page_load_time_counter = {};
    record_log_entry(dateToString(new Date()) + " Loop started");
    setTimeout(send_summary, end);
  });
}

function close_preexisting_windows() {
  for (var i = 0; i < preexisting_windows.length; i++) {
    chrome.windows.remove(preexisting_windows[i].id);
  }
  preexisting_windows.length = 0;
}

function get_active_url(cycle) {
  active_idx = cycle.idx == 0 ? cycle.urls.length - 1 : cycle.idx - 1;
  return cycle.urls[active_idx];
}

function testListener(request, sender, sendResponse) {
  end = Date.now()
  if (sender.tab.id in cycle_tabs) {
    cycle = cycle_tabs[sender.tab.id];
    cycle.successful_loads++;
    url = get_active_url(cycle);
   var page_load_time = end - page_load_time_counter[cycle.id]
   page_load_times.push({'url': (unique_url_salt++) + url, 'time': page_load_time});
   console.log(JSON.stringify(page_load_times));
    record_log_entry(dateToString(new Date()) + " [load success] " + url);
    if (request.action == "should_scroll" && cycle.focus) {
      sendResponse({"should_scroll": should_scroll,
                    "should_scroll_up": should_scroll_up,
                    "scroll_loop": scroll_loop,
                    "scroll_interval": scroll_interval_ms,
                    "scroll_by": scroll_by_pixels});
    }
    delete cycle_tabs[sender.tab.id];
  }
}

function capture_completed_status(details) {
  var tabId = details.tabId;
  if (!(details.tabId in error_codes)) {
    error_codes[tabId] = [];
  }
  var report = {
    'url':details.url,
    'code': details.statusCode,
    'status': details.statusLine,
    'time': new Date(details.timeStamp)
  }
  error_codes[tabId].push(report);
}


function cycle_navigate(cycle) {
  cycle_tabs[cycle.id] = cycle;
  var url = cycle.urls[cycle.idx];
  // Resetting the error codes.
  // TODO(coconutruben) Verify if reseeting here might give us
  // garbage data since some requests/responses might still come
  // in before we update the tab, but we'll register them as
  // information about the subsequent url
  error_codes[cycle.id] = [];
  record_log_entry(dateToString(new Date()) + " [load start] " + url)
  var start = Date.now();
  page_load_time_counter[cycle.id] = start;
  chrome.tabs.update(cycle.id, {'url': url, 'selected': true});
  cycle.idx = (cycle.idx + 1) % cycle.urls.length;
  if (cycle.timeout < cycle.delay / time_ratio && cycle.timeout > 0) {
    cycle.timer = setTimeout(cycle_check_timeout, cycle.timeout, cycle);
  } else {
    cycle.timer = setTimeout(cycle_navigate, cycle.delay / time_ratio, cycle);
  }
}

function record_error_codes(cycle) {
  var error_report = dateToString(new Date()) + " [load failure details] " + get_active_url(cycle) + "\n";
  var reports = error_codes[cycle.id];
  for (var i = 0; i < reports.length; i++) {
    report = reports[i];
    error_report = error_report + "\t\t" +
    dateToString(report.time) + " | " +
    "[response code] " + report.code + " | " +
    "[url] " + report.url + " | " +
    "[status line] " + report.status + "\n";
  }
  log_lines.push(error_report);
  console.log(error_report);
}

function cycle_check_timeout(cycle) {
  if (cycle.id in cycle_tabs) {
    cycle.failed_loads++;
    record_error_codes(cycle);
    record_log_entry(dateToString(new Date()) + " [load failure] " + get_active_url(cycle));
    cycle_navigate(cycle);
  } else {
    cycle.timer = setTimeout(cycle_navigate,
                             cycle.delay / time_ratio - cycle.timeout,
                             cycle);
  }
}

function launch_task(task) {
  if (task.type == 'window' && task.tabs) {
    chrome.windows.create({'url': '/focus.html'}, function (win) {
      close_preexisting_windows();
      chrome.tabs.getSelected(win.id, function(tab) {
        chrome.tabs.update(tab.id, {'url': task.tabs[0], 'selected': true});
        for (var i = 1; i < task.tabs.length; i++) {
          chrome.tabs.create({'windowId':win.id, url: task.tabs[i]});
        }
        setTimeout(chrome.windows.remove, task.duration / time_ratio, win.id);
      });
    });
  } else if (task.type == 'cycle' && task.urls) {
    chrome.windows.create({'url': '/focus.html'}, function (win) {
      close_preexisting_windows();
      chrome.tabs.getSelected(win.id, function(tab) {
        var cycle = {
           'timeout': task.timeout,
           'name': task.name,
           'delay': task.delay,
           'urls': task.urls,
           'id': tab.id,
           'idx': 0,
           'timer': null,
           'focus': !!task.focus,
           'successful_loads': 0,
           'failed_loads': 0
        };
        cycles[task.name] = cycle;
        cycle_navigate(cycle);
        setTimeout(function(cycle, win_id) {
          clearTimeout(cycle.timer);
          chrome.windows.remove(win_id);
        }, task.duration / time_ratio, cycle, win.id);
      });
    });
  }
}

function record_log_entry(entry) {
  log_lines.push(entry);
}

function send_log_entries() {
  var post = [];
  log_lines.forEach(function (item, index, array) {
    var entry = encodeURIComponent(item);
    post.push('url'+ index + '=' + entry);
  });

  var log_url = 'http://localhost:8001/log';
  //  TODO(coconutruben): code-snippet below is shared
  //  across record_log_entry and send_keyvals. Consider
  //  pull into helper if we use more urls.
  var req = new XMLHttpRequest();
  req.open('POST', log_url, true);
  req.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
  req.send(post.join("&"));
  console.log(post.join("&"));
}

function send_keyvals() {
  var post = ["status=good"];

  for (var name in cycles) {
    var cycle = cycles[name];
    post.push(name + "_successful_loads=" + cycle.successful_loads);
    post.push(name + "_failed_loads=" + cycle.failed_loads);
  }

  chrome.runtime.onMessage.removeListener(testListener);

  var status_url = 'http://localhost:8001/status';
  var req = new XMLHttpRequest();
  req.open('POST', status_url, true);
  req.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
  req.send(post.join("&"));
  console.log(post.join("&"));
}

function send_raw_page_load_time_info() {
  var post = [];
  page_load_times.forEach(function (item, index, array) {
    var key = encodeURIComponent(item.url);
    post.push(key + "=" + item.time);
  })

  var pagelt_info_url = 'http://localhost:8001/pagelt'
  var req = new XMLHttpRequest();
  req.open('POST', pagelt_info_url, true);
  req.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
  req.send(post.join("&"));
  console.log(post.join("&"));
}

function send_summary() {
  send_log_entries();
  send_raw_page_load_time_info();
  send_keyvals();
}

function startTest() {
  time_ratio = 3600 * 1000 / test_time_ms; // default test time is 1 hour
  chrome.runtime.onMessage.addListener(testListener);
  setTimeout(setupTest, 1000);
}

function initialize() {
  // Called when the user clicks on the browser action.
  chrome.browserAction.onClicked.addListener(function(tab) {
    // Start the test with default settings.
    chrome.runtime.onMessage.addListener(testListener);
    setTimeout(setupTest, 1000);
  });
}

window.addEventListener("load", initialize);
