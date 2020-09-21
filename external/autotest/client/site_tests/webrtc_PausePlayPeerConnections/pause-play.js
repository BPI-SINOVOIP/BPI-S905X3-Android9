/*
 * Copyright 2017 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/*jshint esversion: 6 */

'use strict';

const $ = document.getElementById.bind(document);

class TestRunner {
  constructor(runtimeSeconds, pausePlayIterationDelayMillis) {
    this.runtimeSeconds = runtimeSeconds;
    this.pausePlayIterationDelayMillis = pausePlayIterationDelayMillis;
    this.elements = [];
    this.peerConnections = [];
    this.iteration = 0;
    this.startTime;
  }

  addPeerConnection(elementType) {
    const element = document.createElement(elementType);
    element.autoplay = false;
    $('body').appendChild(element);
    let resolution;
    if (elementType === 'video') {
      resolution = {w: 300, h: 225};
    } else if (elementType === 'audio') {
      resolution = {w: -1, h: -1};  // -1 is interpreted as disabled
    } else {
      throw new Error('elementType must be one of "audio" or "video"');
    }
    this.elements.push(element);
    this.peerConnections.push(
        new PeerConnection(element, [resolution], cpuOveruseDetection));
  }

  runTest() {
    let promises = this.peerConnections.map((conn) => conn.start());
    Promise.all(promises)
        .then(() => {
          this.startTime = Date.now();
          this.pauseAndPlayLoop();
        })
        .catch((e) => {throw e});
  }

  pauseAndPlayLoop() {
    this.iteration++;
    this.elements.forEach((feed) => {
      if (Math.random() >= 0.5) {
        feed.play();
      } else {
        feed.pause();
      }
    });
    const status = this.getStatus();
    $('status').textContent = status
    if (status != 'ok-done') {
      setTimeout(
          () => {this.pauseAndPlayLoop()}, this.pausePlayIterationDelayMillis);
    } else {  // We're done. Pause all feeds.
      this.elements.forEach((feed) => {
        feed.pause();
      });
    }
  }

  getStatus() {
    if (this.iteration == 0) {
      return 'not-started';
    }
    try {
      this.peerConnections.forEach((conn) => conn.verifyState());
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

// Declare testRunner so that the Python code can access it to query status.
// Also allows us to access it easily in dev tools for debugging.
let testRunner;
// Set from the Python test runner
let cpuOveruseDetection = null;
let elementType;

function startTest(
    runtimeSeconds, numPeerConnections, pausePlayIterationDelayMillis) {
  testRunner = new TestRunner(
      runtimeSeconds, pausePlayIterationDelayMillis);
  for (let i = 0; i < numPeerConnections; i++) {
    testRunner.addPeerConnection(elementType);
  }
  testRunner.runTest();
}

function getStatus() {
  return testRunner ? testRunner.getStatus() : 'not-initialized';
}

