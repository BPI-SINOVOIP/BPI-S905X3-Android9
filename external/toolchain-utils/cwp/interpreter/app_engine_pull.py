# Copyright 2012 Google Inc. All Rights Reserved.
# Author: mrdmnd@ (Matt Redmond)
"""A client to pull data from Bartlett.

Inspired by //depot/google3/experimental/mobile_gwp/database/app_engine_pull.py

The server houses perf.data.gz, board, chrome version for each upload.
This script first authenticates with a proper @google.com account, then
downloads a sample (if it's not already cached) and unzips perf.data

  Authenticate(): Gets login info and returns an auth token
  DownloadSamples(): Download and unzip samples.
  _GetServePage(): Pulls /serve page from the app engine server
  _DownloadSampleFromServer(): Downloads a local compressed copy of a sample
  _UncompressSample(): Decompresses a sample, deleting the compressed version.
"""
import cookielib
import getpass
import gzip
import optparse
import os
import urllib
import urllib2

SERVER_NAME = 'http://chromeoswideprofiling.appspot.com'
APP_NAME = 'chromeoswideprofiling'
DELIMITER = '~'


def Authenticate(server_name):
  """Gets credentials from user and attempts to retrieve auth token.
     TODO: Accept OAuth2 instead of password.
  Args:
    server_name: (string) URL that the app engine code is living on.
  Returns:
    authtoken: (string) The authorization token that can be used
                        to grab other pages.
  """

  if server_name.endswith('/'):
    server_name = server_name.rstrip('/')
  # Grab username and password from user through stdin.
  username = raw_input('Email (must be @google.com account): ')
  password = getpass.getpass('Password: ')
  # Use a cookie to authenticate with GAE.
  cookiejar = cookielib.LWPCookieJar()
  opener = urllib2.build_opener(urllib2.HTTPCookieProcessor(cookiejar))
  urllib2.install_opener(opener)
  # Get an AuthToken from Google accounts service.
  auth_uri = 'https://www.google.com/accounts/ClientLogin'
  authreq_data = urllib.urlencode({'Email': username,
                                   'Passwd': password,
                                   'service': 'ah',
                                   'source': APP_NAME,
                                   'accountType': 'HOSTED_OR_GOOGLE'})
  auth_req = urllib2.Request(auth_uri, data=authreq_data)
  try:
    auth_resp = urllib2.urlopen(auth_req)
  except urllib2.URLError:
    print 'Error logging in to Google accounts service.'
    return None
  body = auth_resp.read()
  # Auth response contains several fields.
  # We care about the part after Auth=
  auth_resp_dict = dict(x.split('=') for x in body.split('\n') if x)
  authtoken = auth_resp_dict['Auth']
  return authtoken


def DownloadSamples(server_name, authtoken, output_dir, start, stop):
  """Download every sample and write unzipped version
     to output directory.
  Args:
    server_name: (string) URL that the app engine code is living on.
    authtoken:   (string) Authorization token.
    output_dir   (string) Filepath to write output to.
    start:       (int)    Index to start downloading from, starting at top.
    stop:        (int)    Index to stop downloading, non-inclusive. -1 for end.
  Returns:
    None
  """

  if server_name.endswith('/'):
    server_name = server_name.rstrip('/')

  serve_page_string = _GetServePage(server_name, authtoken)
  if serve_page_string is None:
    print 'Error getting /serve page.'
    return

  sample_list = serve_page_string.split('</br>')
  print 'Will download:'
  sample_list_subset = sample_list[start:stop]
  for sample in sample_list_subset:
    print sample
  for sample in sample_list_subset:
    assert sample, 'Sample should be valid.'
    sample_info = [s.strip() for s in sample.split(DELIMITER)]
    key = sample_info[0]
    time = sample_info[1]
    time = time.replace(' ', '_')  # No space between date and time.
    # sample_md5 = sample_info[2]
    board = sample_info[3]
    version = sample_info[4]

    # Put a compressed copy of the samples in output directory.
    _DownloadSampleFromServer(server_name, authtoken, key, time, board, version,
                              output_dir)
    _UncompressSample(key, time, board, version, output_dir)


def _BuildFilenameFromParams(key, time, board, version):
  """Return the filename for our sample.
  Args:
    key:  (string) Key indexing our sample in the datastore.
    time: (string) Date that the sample was uploaded.
    board: (string) Board that the sample was taken on.
    version: (string) Version string from /etc/lsb-release
  Returns:
    filename (string)
  """
  filename = DELIMITER.join([key, time, board, version])
  return filename


def _DownloadSampleFromServer(server_name, authtoken, key, time, board, version,
                              output_dir):
  """Downloads sample_$(samplekey).gz to current dir.
  Args:
    server_name: (string) URL that the app engine code is living on.
    authtoken:   (string) Authorization token.
    key:  (string) Key indexing our sample in the datastore
    time: (string) Date that the sample was uploaded.
    board: (string) Board that the sample was taken on.
    version: (string) Version string from /etc/lsb-release
    output_dir:  (string) Filepath to write to output to.
  Returns:
    None
  """
  filename = _BuildFilenameFromParams(key, time, board, version)
  compressed_filename = filename + '.gz'

  if os.path.exists(os.path.join(output_dir, filename)):
    print 'Already downloaded %s, skipping.' % filename
    return

  serv_uri = server_name + '/serve/' + key
  serv_args = {'continue': serv_uri, 'auth': authtoken}
  full_serv_uri = server_name + '/_ah/login?%s' % urllib.urlencode(serv_args)
  serv_req = urllib2.Request(full_serv_uri)
  serv_resp = urllib2.urlopen(serv_req)
  f = open(os.path.join(output_dir, compressed_filename), 'w+')
  f.write(serv_resp.read())
  f.close()


def _UncompressSample(key, time, board, version, output_dir):
  """Uncompresses a given sample.gz file and deletes the compressed version.
  Args:
    key: (string) Sample key to uncompress.
    time: (string) Date that the sample was uploaded.
    board: (string) Board that the sample was taken on.
    version: (string) Version string from /etc/lsb-release
    output_dir: (string) Filepath to find sample key in.
  Returns:
    None
  """
  filename = _BuildFilenameFromParams(key, time, board, version)
  compressed_filename = filename + '.gz'

  if os.path.exists(os.path.join(output_dir, filename)):
    print 'Already decompressed %s, skipping.' % filename
    return

  out_file = open(os.path.join(output_dir, filename), 'wb')
  in_file = gzip.open(os.path.join(output_dir, compressed_filename), 'rb')
  out_file.write(in_file.read())
  in_file.close()
  out_file.close()
  os.remove(os.path.join(output_dir, compressed_filename))


def _DeleteSampleFromServer(server_name, authtoken, key):
  """Opens the /delete page with the specified key
     to delete the sample off the datastore.
    Args:
      server_name: (string) URL that the app engine code is living on.
      authtoken:   (string) Authorization token.
      key:  (string) Key to delete.
    Returns:
      None
  """

  serv_uri = server_name + '/del/' + key
  serv_args = {'continue': serv_uri, 'auth': authtoken}
  full_serv_uri = server_name + '/_ah/login?%s' % urllib.urlencode(serv_args)
  serv_req = urllib2.Request(full_serv_uri)
  urllib2.urlopen(serv_req)


def _GetServePage(server_name, authtoken):
  """Opens the /serve page and lists all keys.
  Args:
    server_name: (string) URL the app engine code is living on.
    authtoken:   (string) Authorization token.
  Returns:
    The text of the /serve page (including HTML tags)
  """

  serv_uri = server_name + '/serve'
  serv_args = {'continue': serv_uri, 'auth': authtoken}
  full_serv_uri = server_name + '/_ah/login?%s' % urllib.urlencode(serv_args)
  serv_req = urllib2.Request(full_serv_uri)
  serv_resp = urllib2.urlopen(serv_req)
  return serv_resp.read()


def main():
  parser = optparse.OptionParser()
  parser.add_option('--output_dir',
                    dest='output_dir',
                    action='store',
                    help='Path to output perf data files.')
  parser.add_option('--start',
                    dest='start_ind',
                    action='store',
                    default=0,
                    help='Start index.')
  parser.add_option('--stop',
                    dest='stop_ind',
                    action='store',
                    default=-1,
                    help='Stop index.')
  options = parser.parse_args()[0]
  if not options.output_dir:
    print 'Must specify --output_dir.'
    return 1
  if not os.path.exists(options.output_dir):
    print 'Specified output_dir does not exist.'
    return 1

  authtoken = Authenticate(SERVER_NAME)
  if not authtoken:
    print 'Could not obtain authtoken, exiting.'
    return 1
  DownloadSamples(SERVER_NAME, authtoken, options.output_dir, options.start_ind,
                  options.stop_ind)
  print 'Downloaded samples.'
  return 0


if __name__ == '__main__':
  exit(main())
