//This content script injected into the page is to suppress
// popup that prevents closing the tab.

var PLAY_MUSIC_HOSTNAME = 'play.google.com';

if (document.location.hostname === PLAY_MUSIC_HOSTNAME) {
  var script = document.createElement('script');
  script.src = chrome.extension.getURL("custom_event_listener.js");
  document.documentElement.insertBefore(script, document.documentElement.firstChild);
}
