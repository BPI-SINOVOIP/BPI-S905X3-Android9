/*
 * Copyright 2017 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/*jshint esversion: 6 */

'use strict';

const $ = document.getElementById.bind(document);

const MAIN_FEED_RESOLUTION = {w:1280, h:720};

// This resolution is what we typically get on Hangouts Meet.
const SMALL_FEED_RESOLUTION = {w:182, h:136};

// This test frequently reports weird resolutions although the visuals look OK.
// Hence, require lots of consecutive bad resolutions before failure.
// TODO(kerl): Effectively disabled now, investigate why we get so many bad
// resolution reports.
const NUM_BAD_RESOLUTIONS_FOR_FAILURE = Number.MAX_SAFE_INTEGER;

class TestRunner {
  constructor(numConnections, runtimeSeconds, iterationDelayMillis) {
    this.runtimeSeconds = runtimeSeconds;
    this.iterationDelayMillis = iterationDelayMillis;
    this.videoElements = [];
    this.mainFeed = null;
    this.peerConnections = [];
    this.numConnections = numConnections;
    this.iteration = 0;
    this.startTime = 0;  // initialized to dummy value
    this.status = this.getStatusInternal_();
  }

  runTest() {
    for (let i = 0; i < this.numConnections; i++) {
      const videoElement = document.createElement('video');
      videoElement.autoplay = true;
      $('body').appendChild(videoElement);
      if (!this.mainFeed) {
        // The first created is the main feed.
        setSize(videoElement, MAIN_FEED_RESOLUTION);
        this.mainFeed = videoElement;
      } else {
        setSize(videoElement, SMALL_FEED_RESOLUTION);
        this.videoElements.push(videoElement);
      }
      this.peerConnections.push(new PeerConnection(
          videoElement, [MAIN_FEED_RESOLUTION], cpuOveruseDetection));
    }
    const promises = this.peerConnections.map((conn) => conn.start());
    Promise.all(promises)
        .then(() => {
          this.startTime = Date.now();
          this.switchFeedLoop();
        })
        .catch((e) => {throw e});
  }

  switchFeedLoop() {
    this.iteration++;
    this.status = this.getStatusInternal_();
    $('status').textContent = this.status;
    if (this.status != 'ok-done') {
      const switchWith = Math.floor(Math.random() * this.videoElements.length);
      const newMainSrc = this.videoElements[switchWith].srcObject;
      this.videoElements[switchWith].srcObject = this.mainFeed.srcObject;
      this.mainFeed.srcObject = newMainSrc;
      setTimeout(
          () => this.switchFeedLoop(), this.iterationDelayMillis);
    }
  }

  getStatus() {
    return this.status;
  }

  getStatusInternal_() {
    if (this.iteration == 0) {
      return 'not-started';
    }
    try {
      this.peerConnections.forEach(
          (conn) => conn.verifyState(NUM_BAD_RESOLUTIONS_FOR_FAILURE));
    } catch (e) {
      return `failure: ${e.message}`;
    }
    const timeSpent = Date.now() - this.startTime;
    if (timeSpent >= this.runtimeSeconds * 1000) {
      return 'ok-done';
    } else {
      return `running, iteration: ${this.iteration}`;
    }
  }
}

function setSize(element, size) {
  element.setAttribute('style', `width:${size.w}px;height:${size.h}px`);
}

// Declare testRunner so that the Python code can access it to query status.
// Also allows us to access it easily in dev tools for debugging.
let testRunner;
// Set from the Python test runner
let cpuOveruseDetection = null;

function startTest(
    runtimeSeconds, numPeerConnections, iterationDelayMillis) {
  testRunner = new TestRunner(
      numPeerConnections, runtimeSeconds, iterationDelayMillis);
  testRunner.runTest();
}

function getStatus() {
  return testRunner ? testRunner.getStatus() : 'not-initialized';
}

