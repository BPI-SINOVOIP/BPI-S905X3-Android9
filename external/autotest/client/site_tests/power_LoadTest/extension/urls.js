// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// List of tasks to accomplish
var URLS = new Array();

var ViewGDoc = ('https://docs.google.com/document/d/');

var BBC_AUDIO_URL = 'https://www.bbc.co.uk/radio/player/bbc_world_service';

var PLAY_MUSIC_URL = 'https://play.google.com/music/listen?u=0#/wst/st/a2be2d85-0ac9-3a7a-b038-e221bb63ef71';

function isMP3DecoderPresent() {
    return window['MediaSource'] &&
      window['MediaSource'].isTypeSupported('audio/mpeg');
}

var tasks = [
  {
    // Chrome browser window 1. This window remains open for the entire test.
    type: 'window',
    name: 'background',
    start: 0,
    duration: minutes(60),
    focus: false,
    tabs: [
     'https://www.google.com',
     'https://news.google.com',
     'https://finance.yahoo.com',
     'https://clothing.shop.ebay.com/Womens-Shoes-/63889/i.html',
     'https://www.facebook.com'
    ]
  },
  {
    // Page cycle through popular external websites for 36 minutes
    type: 'cycle',
    name: 'web',
    start: seconds(1),
    duration: minutes(36),
    delay: seconds(60), // A minute on each page
    timeout: seconds(30),
    focus: true,
    urls: URLS,
  },
  {
    // After 36 minutes, actively read e-mail for 12 minutes
    type: 'cycle',
    name: 'email',
    start: minutes(36) + seconds(1),
    duration: minutes(12) - seconds(1),
    delay: minutes(5), // 5 minutes between full gmail refresh
    timeout: seconds(30),
    focus: true,
    urls: [
       'https://gmail.com',
       'https://mail.google.com'
    ],
  },
  {
    // After 36 minutes, start streaming audio (background tab), total playtime
    // 12 minutes
    type: 'cycle',
    name: 'audio',
    start: minutes(36),
    duration: minutes(12),
    delay: minutes(12),
    timeout: seconds(30),
    focus: false,
    // Google Play Music requires MP3 decoder for playing music.
    // Fall back to BBC if the browser does not have MP3 decoder bundle.
    urls: isMP3DecoderPresent() ? [BBC_AUDIO_URL, BBC_AUDIO_URL] :
                                  [BBC_AUDIO_URL, BBC_AUDIO_URL]
  },
  {
    // After 48 minutes, play with Google Docs for 6 minutes
    type: 'cycle',
    name: 'docs',
    start: minutes(48),
    duration: minutes(6),
    delay: minutes(1), // A minute on each page
    timeout: seconds(30),
    focus: true,
    urls: [
       ViewGDoc + '1CIvneyASuIHvxxN0WV22zikb08Us1nc93mkU0c5Azr4/edit',
       ViewGDoc + '120TtfoHXCgRuaubGhra3X5tl0_pS7KX757wFigTFf0c/edit'
    ],
  },
  {
    // After 54 minutes, watch Big Buck Bunny for 6 minutes
    type: 'window',
    name: 'video',
    start: minutes(54),
    duration: minutes(6),
    focus: true,
    tabs: [
        'https://www.youtube.com/embed/YE7VzlLtp-4?start=236&vq=hd720&autoplay=1'
    ]
  },
];


// List of URLs to cycle through
var u_index = 0;
URLS[u_index++] = 'https://www.google.com';
URLS[u_index++] = 'https://www.yahoo.com';
URLS[u_index++] = 'https://www.facebook.com';
URLS[u_index++] = 'https://www.youtube.com';
URLS[u_index++] = 'https://www.wikipedia.org';
URLS[u_index++] = 'https://www.amazon.com';
URLS[u_index++] = 'https://www.msn.com';
URLS[u_index++] = 'https://www.bing.com';
URLS[u_index++] = 'https://www.blogspot.com';
URLS[u_index++] = 'https://www.microsoft.com';
URLS[u_index++] = 'https://www.myspace.com';
URLS[u_index++] = 'http://www.go.com';
URLS[u_index++] = 'https://www.walmart.com';
URLS[u_index++] = 'https://www.about.com';
URLS[u_index++] = 'https://www.target.com';
URLS[u_index++] = 'https://www.aol.com';
URLS[u_index++] = 'https://www.mapquest.com';
URLS[u_index++] = 'https://www.ask.com';
URLS[u_index++] = 'https://www.craigslist.org';
URLS[u_index++] = 'https://www.wordpress.com';
URLS[u_index++] = 'https://www.answers.com';
URLS[u_index++] = 'https://www.paypal.com';
URLS[u_index++] = 'https://www.imdb.com';
URLS[u_index++] = 'https://www.bestbuy.com';
URLS[u_index++] = 'https://www.ehow.com';
URLS[u_index++] = 'http://www.photobucket.com';
URLS[u_index++] = 'https://www.cnn.com';
URLS[u_index++] = 'https://www.chase.com';
URLS[u_index++] = 'https://www.att.com';
URLS[u_index++] = 'https://www.sears.com';
URLS[u_index++] = 'https://www.weather.com';
URLS[u_index++] = 'https://www.apple.com';
URLS[u_index++] = 'https://www.zynga.com';
URLS[u_index++] = 'https://www.adobe.com';
URLS[u_index++] = 'https://www.bankofamerica.com';
URLS[u_index++] = 'https://www.zedo.com';
URLS[u_index++] = 'https://www.flickr.com';
URLS[u_index++] = 'https://www.shoplocal.com';
URLS[u_index++] = 'https://www.twitter.com';
URLS[u_index++] = 'https://www.cnet.com';
URLS[u_index++] = 'https://www.verizonwireless.com';
URLS[u_index++] = 'https://www.kohls.com';
URLS[u_index++] = 'https://www.bizrate.com';
URLS[u_index++] = 'https://www.jcpenney.com';
URLS[u_index++] = 'https://www.netflix.com';
URLS[u_index++] = 'https://www.fastclick.net';
URLS[u_index++] = 'https://www.windows.com';
URLS[u_index++] = 'https://www.questionmarket.com';
URLS[u_index++] = 'https://www.nytimes.com';
URLS[u_index++] = 'https://www.toysrus.com';
URLS[u_index++] = 'https://www.allrecipes.com';
URLS[u_index++] = 'https://www.overstock.com';
URLS[u_index++] = 'https://www.comcast.net';
