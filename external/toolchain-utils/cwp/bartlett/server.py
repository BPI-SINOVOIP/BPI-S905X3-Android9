#!/usr/bin/python
# Copyright 2012 Google Inc. All Rights Reserved.
# Author: mrdmnd@ (Matt Redmond)
# Based off of code in //depot/google3/experimental/mobile_gwp
"""Code to transport profile data between a user's machine and the CWP servers.
    Pages:
    "/": the main page for the app, left blank so that users cannot access
         the file upload but left in the code for debugging purposes
    "/upload": Updates the datastore with a new file. the upload depends on
               the format which is templated on the main page ("/")
               input includes:
                    profile_data: the zipped file containing profile data
                    board:  the architecture we ran on
                    chromeos_version: the chromeos_version
    "/serve": Lists all of the files in the datastore. Each line is a new entry
              in the datastore. The format is key~date, where key is the entry's
              key in the datastore and date is the file upload time and date.
              (Authentication Required)
    "/serve/([^/]+)?": For downloading a file of profile data, ([^/]+)? means
                       any character sequence so to download the file go to
                       '/serve/$key' where $key is the datastore key of the file
                       you want to download.
                       (Authentication Required)
    "/del/([^/]+)?": For deleting an entry in the datastore. To use go to
                     '/del/$key' where $key is the datastore key of the entry
                     you want to be deleted form the datastore.
                     (Authentication Required)
    TODO: Add more extensive logging"""

import cgi
import logging
import md5
import urllib

from google.appengine.api import users
from google.appengine.ext import db
from google.appengine.ext import webapp
from google.appengine.ext.webapp.util import run_wsgi_app

logging.getLogger().setLevel(logging.DEBUG)


class FileEntry(db.Model):
  profile_data = db.BlobProperty()  # The profile data
  date = db.DateTimeProperty(auto_now_add=True)  # Date it was uploaded
  data_md5 = db.ByteStringProperty()  # md5 of the profile data
  board = db.StringProperty()  # board arch
  chromeos_version = db.StringProperty()  # ChromeOS version


class MainPage(webapp.RequestHandler):
  """Main page only used as the form template, not actually displayed."""

  def get(self, response=''):  # pylint: disable-msg=C6409
    if response:
      self.response.out.write('<html><body>')
      self.response.out.write("""<br>
        <form action="/upload" enctype="multipart/form-data" method="post">
          <div><label>Profile Data:</label></div>
          <div><input type="file" name="profile_data"/></div>
          <div><label>Board</label></div>
          <div><input type="text" name="board"/></div>
          <div><label>ChromeOS Version</label></div>
          <div><input type="text" name="chromeos_version"></div>
          <div><input type="submit" value="send" name="submit"></div>
        </form>
      </body>
      </html>""")


class Upload(webapp.RequestHandler):
  """Handler for uploading data to the datastore, accessible by anyone."""

  def post(self):  # pylint: disable-msg=C6409
    """Takes input based on the main page's form."""
    getfile = FileEntry()
    f1 = self.request.get('profile_data')
    getfile.profile_data = db.Blob(f1)
    getfile.data_md5 = md5.new(f1).hexdigest()
    getfile.board = self.request.get('board')
    getfile.chromeos_version = self.request.get('chromeos_version')
    getfile.put()
    self.response.out.write(getfile.key())
    #self.redirect('/')


class ServeHandler(webapp.RequestHandler):
  """Given the entry's key in the database, output the profile data file. Only
      accessible from @google.com accounts."""

  def get(self, resource):  # pylint: disable-msg=C6409
    if Authenticate(self):
      file_key = str(urllib.unquote(resource))
      request = db.get(file_key)
      self.response.out.write(request.profile_data)


class ListAll(webapp.RequestHandler):
  """Displays all files uploaded. Only accessible by @google.com accounts."""

  def get(self):  # pylint: disable-msg=C6409
    """Displays all information in FileEntry, ~ delimited."""
    if Authenticate(self):
      query_str = 'SELECT * FROM FileEntry ORDER BY date ASC'
      query = db.GqlQuery(query_str)
      delimiter = '~'

      for item in query:
        display_list = [item.key(), item.date, item.data_md5, item.board,
                        item.chromeos_version]
        str_list = [cgi.escape(str(i)) for i in display_list]
        self.response.out.write(delimiter.join(str_list) + '</br>')


class DelEntries(webapp.RequestHandler):
  """Deletes entries. Only accessible from @google.com accounts."""

  def get(self, resource):  # pylint: disable-msg=C6409
    """A specific entry is deleted, when the key is given."""
    if Authenticate(self):
      fkey = str(urllib.unquote(resource))
      request = db.get(fkey)
      if request:
        db.delete(fkey)


def Authenticate(webpage):
  """Some urls are only accessible if logged in with a @google.com account."""
  user = users.get_current_user()
  if user is None:
    webpage.redirect(users.create_login_url(webpage.request.uri))
  elif user.email().endswith('@google.com'):
    return True
  else:
    webpage.response.out.write('Not Authenticated')
    return False


def main():
  application = webapp.WSGIApplication(
      [
          ('/', MainPage),
          ('/upload', Upload),
          ('/serve/([^/]+)?', ServeHandler),
          ('/serve', ListAll),
          ('/del/([^/]+)?', DelEntries),
      ],
      debug=False)
  run_wsgi_app(application)


if __name__ == '__main__':
  main()
