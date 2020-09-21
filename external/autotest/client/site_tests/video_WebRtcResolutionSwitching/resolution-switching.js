/*
 * Copyright 2017 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/*jshint esversion: 6 */

'use strict';

const $ = document.getElementById.bind(document);

function logError(err) {
  console.error(err);
}

// Available resolutions to switch between. These are 4:3 resolutions chosen
// since they have significant distance between them and are quite common. E.g.
// they can be selected for youtube videos. We also avoid higher resolutions
// since they consume a lot of resources.
const RESOLUTIONS = [
  {w:320, h:240},
  {w:480, h:360},
  {w:640, h:480},
  {w:1280, h:720},
];

class TestRunner {
  constructor(runtimeSeconds, switchResolutionDelayMillis) {
    this.runtimeSeconds = runtimeSeconds;
    this.switchResolutionDelayMillis = switchResolutionDelayMillis;
    this.videoElements = [];
    this.peerConnections = [];
    this.numConnections = 0;
    this.iteration = 0;
    this.startTime = 0;  // initialized to dummy value
    this.status = this.getStatusInternal_();
  }

  addPeerConnection() {
    const videoElement = document.createElement('video');
    videoElement.autoplay = true;
    $('body').appendChild(videoElement);
    this.videoElements.push(videoElement);
    this.peerConnections.push(
        new PeerConnection(videoElement, RESOLUTIONS, cpuOveruseDetection));
  }

  runTest() {
    const promises = this.peerConnections.map((conn) => conn.start());
    Promise.all(promises)
        .then(() => {
          this.startTime = Date.now();
          this.switchResolutionLoop();
        })
        .catch((e) => {throw e});
  }

  switchResolutionLoop() {
    this.iteration++;
    this.status = this.getStatusInternal_();
    $('status').textContent = this.status;
    if (this.status != 'ok-done') {
      Promise.all(this.peerConnections.map((pc) => pc.switchToRandomStream()))
          .then(
              () => setTimeout(
                  () => this.switchResolutionLoop(),
                  this.switchResolutionDelayMillis));
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
      this.peerConnections.forEach((conn) => conn.verifyState());
    } catch (e) {
      return `failure: ${e.message}`;
    }
    const timeSpent = Date.now() - this.startTime;
    if (timeSpent >= this.runtimeSeconds * 1000) {
      return 'ok-done';
    }
    return `running, iteration: ${this.iteration}`;
  }
}

// Declare testRunner so that the Python code can access it to query status.
// Also allows us to access it easily in dev tools for debugging.
let testRunner;
// Set from the Python test runner
let cpuOveruseDetection = null;

function startTest(
    runtimeSeconds, numPeerConnections, switchResolutionDelayMillis) {
  testRunner = new TestRunner(runtimeSeconds, switchResolutionDelayMillis);
  for (let i = 0; i < numPeerConnections; i++) {
    testRunner.addPeerConnection();
  }
  testRunner.runTest();
}

function getStatus() {
  return testRunner ? testRunner.getStatus() : 'not-initialized';
}

