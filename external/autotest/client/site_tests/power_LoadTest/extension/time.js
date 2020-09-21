// Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Convert seconds to milliseconds
function seconds(s) {
    return s * 1000;
}

// Convert minutes to milliseconds
function minutes(m) {
    return seconds(m * 60);
}

// returns a formatted string of current time
// for debugging
function dateToString(date) {
	var formatting = {
		hour: 'numeric',
		minute: 'numeric',
		second: 'numeric',
		hour12: false
	}
	var timeString = date.toLocaleTimeString([], formatting);
	timeString = timeString + ":" + date.getMilliseconds();
	return timeString
}