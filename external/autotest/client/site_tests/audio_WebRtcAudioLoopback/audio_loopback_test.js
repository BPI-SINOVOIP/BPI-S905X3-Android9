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


class FeedTable {
  constructor() {
    this.numCols = 5;
    this.col = 0;
    this.testTable = document.getElementById('test-table');
    this.row = this.testTable.insertRow(-1);
  }

  addNewAudioCell() {
    if (this.col == this.numCols) {
      this.row = this.testTable.insertRow(-1);
      this.col = 0;
    }
    var newCell = this.row.insertCell(-1);
    var audio = document.createElement('audio');
    audio.autoplay = false;
    newCell.appendChild(audio);
    this.col++;
    return audio;
  }
}


class PeerConnection {
  constructor(audioElement) {
    this.localConnection = null;
    this.remoteConnection = null;
    this.remoteAudio = audioElement;
  }

  start() {
    const onGetUserMediaSuccess = this.onGetUserMediaSuccess.bind(this);
    return navigator.mediaDevices
        .getUserMedia({audio: true, video: true})
        .then(onGetUserMediaSuccess);
  }

  onGetUserMediaSuccess(stream) {
    this.localConnection = new RTCPeerConnection(null);
    this.localConnection.onicecandidate = (event) => {
      this.onIceCandidate(this.remoteConnection, event);
    };
    this.localConnection.addStream(stream);

    this.remoteConnection = new RTCPeerConnection(null);
    this.remoteConnection.onicecandidate = (event) => {
      this.onIceCandidate(this.localConnection, event);
    };
    this.remoteConnection.onaddstream = (e) => {
      this.remoteAudio.srcObject = e.stream;
    };

    var onCreateOfferSuccess = this.onCreateOfferSuccess.bind(this);
    this.localConnection
        .createOffer({offerToReceiveAudio: 1, offerToReceiveVideo: 1})
        .then(onCreateOfferSuccess, logError);
  }

  onCreateOfferSuccess(desc) {
    this.localConnection.setLocalDescription(desc);
    this.remoteConnection.setRemoteDescription(desc);

    var onCreateAnswerSuccess = this.onCreateAnswerSuccess.bind(this);
    this.remoteConnection.createAnswer().then(onCreateAnswerSuccess, logError);
  }

  onCreateAnswerSuccess(desc) {
    this.remoteConnection.setLocalDescription(desc);
    this.localConnection.setRemoteDescription(desc);
  }

  onIceCandidate(connection, event) {
    if (event.candidate) {
      connection.addIceCandidate(new RTCIceCandidate(event.candidate));
    }
  }
}


class TestRunner {
  constructor(runtimeSeconds) {
    this.runtimeSeconds = runtimeSeconds;
    this.audioElements = [];
    this.peerConnections = [];
    this.feedTable = new FeedTable();
    this.iteration = 0;
    this.startTime;
    this.lastIterationTime;
  }

  addPeerConnection() {
    const audioElement = this.feedTable.addNewAudioCell();
    this.audioElements.push(audioElement);
    this.peerConnections.push(new PeerConnection(audioElement));
  }

  startTest() {
    this.startTime = Date.now();
    let promises = testRunner.peerConnections.map((conn) => conn.start());
    Promise.all(promises)
        .then(() => {
          this.startTime = Date.now();
          this.audioElements.forEach((feed) => feed.play());
          this.pauseAndPlayLoop();
        })
        .catch((e) => {throw e});
  }

  pauseAndPlayLoop() {
    this.iteration++;
    const status = this.getStatus();
    this.lastIterationTime = Date.now();
    $('status').textContent = status
    if (status != 'ok-done') {
      setTimeout(() => this.pauseAndPlayLoop());
    } else {
      // Finished, pause the audio.
      this.audioElements.forEach((feed) => feed.pause());
    }
  }

  getStatus() {
    if (this.iteration == 0) {
      return 'not-started';
    }
    const timeSpent = Date.now() - this.startTime;
    if (timeSpent >= this.runtimeSeconds * 1000) {
      return 'ok-done';
    } else {
      return `running, iteration: ${this.iteration}`;
    }
  }

  getResults() {
    const runTimeMillis = this.lastIterationTime - this.startTime;
    return {'runTimeSeconds': runTimeMillis / 1000};
  }
}


let testRunner;

function run(runtimeSeconds, numPeerConnections) {
  testRunner = new TestRunner(runtimeSeconds);
  for (let i = 0; i < numPeerConnections; i++) {
    testRunner.addPeerConnection();
  }
  testRunner.startTest();
}

