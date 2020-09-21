// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

request = {action: 'should_scroll'}

var PLAY_MUSIC_HOSTNAME = 'play.google.com';

//Sends message to the test.js(background script). test.js on
//receiving a message from content script assumes the page has
//loaded successfully. It further responds with instructions on
//whether/how to scroll.
function sendSuccessToBGScript() {
  chrome.runtime.sendMessage(request, function(response) {
    if (response && response.should_scroll) {
      window.focus();
      lastOffset = window.pageYOffset;
      var start_interval = Math.max(10000, response.scroll_interval);
      function smoothScrollDown() {
        window.scrollBy(0, response.scroll_by);
        if (window.pageYOffset != lastOffset) {
          lastOffset = window.pageYOffset;
          setTimeout(smoothScrollDown, response.scroll_interval);
        } else if (response.should_scroll_up) {
          setTimeout(smoothScrollUp, start_interval);
        }
      }
      function smoothScrollUp() {
        window.scrollBy(0, -1 * response.scroll_by);
        if (window.pageYOffset != lastOffset) {
          lastOffset = window.pageYOffset;
          setTimeout(smoothScrollUp, response.scroll_interval);
        } else if (response.scroll_loop) {
          setTimeout(smoothScrollDown, start_interval);
        }
      }
      setTimeout(smoothScrollDown, start_interval);
    }
  });
}

function afterLoad() {
  if (document.location.hostname !== PLAY_MUSIC_HOSTNAME) {
    sendSuccessToBGScript();
    return;
  }

  var playButton = document.querySelector('[data-id="play"]');

  //If play music website, if we do not see a play button
  //that effectively means the music is not loaded. So do not
  //report success load to test.js.
  if (playButton) {
    sendSuccessToBGScript();
    playButton.click();
  }
}

window.addEventListener('load', afterLoad);
