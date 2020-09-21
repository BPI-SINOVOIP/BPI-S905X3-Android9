/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Client id obtained from API console credential page should be set before
 * running this script. */
if (!CLIENT_ID) {
  console.error.log('CLIENT_ID is not set');
}

/* Initialize a Google Drive picker after Google client API is loaded. */
function onGoogleClientApiLoad() {
  var picker = new Picker({
      clientId: CLIENT_ID,
      button: document.getElementById('google_drive_pick_file')
  });
}

/* Initializes a Google Drive picker and loads drive API and picker API.*/
var Picker = function(options) {
  this.clientId = options.clientId;
  this.button = options.button;
  this.button.addEventListener('click', this.open.bind(this));

  gapi.client.load('drive', 'v2');
  gapi.load('picker', {callback: this.onPickerApiLoaded.bind(this) });
}

/* Enable the button after picker API is loaded. */
Picker.prototype.onPickerApiLoaded = function() {
  this.button.disabled = false;
};

/* Let user authenticate and show file picker. */
Picker.prototype.open = function(){
  if (gapi.auth.getToken()) {
    this.showFilePicker();
  } else {
    this.authenticate(this.showFilePicker.bind(this));
  }
};

/* Run authenticate using auth API. */
Picker.prototype.authenticate = function(callback) {
  gapi.auth.authorize(
      {
          client_id: this.clientId + '.apps.googleusercontent.com',
          scope: 'https://www.googleapis.com/auth/drive.readonly',
          immediate: false
      },
      callback);
};

/* Create a picker using picker API. */
Picker.prototype.showFilePicker = function() {
  var accessToken = gapi.auth.getToken().access_token;
  this.picker = new google.picker.PickerBuilder().
                addView(google.picker.ViewId.DOCS).
                setAppId(this.clientId).
                setOAuthToken(accessToken).
                setCallback(this.onFilePicked.bind(this)).
                build().
                setVisible(true);
};

/* Request the file using drive API once a file is picked. */
Picker.prototype.onFilePicked = function(data) {
  if (data[google.picker.Response.ACTION] == google.picker.Action.PICKED) {
    var file = data[google.picker.Response.DOCUMENTS][0];
    var id = file[google.picker.Document.ID];
    var request = gapi.client.drive.files.get({fileId: id});
    request.execute(this.onFileGet.bind(this));
  }
};

/* Retrieve the file URL and access token and set it as audio source. */
Picker.prototype.onFileGet = function(file) {
  var accessToken = gapi.auth.getToken().access_token;
  var authedUrl = file.downloadUrl + "&access_token="
      + encodeURIComponent(accessToken);
  audio_source_set(authedUrl);
};
