# Copyright 2011 Google Inc. All Rights Reserved.
#
# Django settings for monitor project.
#
# For explanation look here: http://docs.djangoproject.com/en/dev/ref/settings
#

__author__ = 'kbaclawski@google.com (Krystian Baclawski)'

import os.path
import sys

# Path to the root of application. It's a custom setting, not related to Django.
ROOT_PATH = os.path.dirname(os.path.realpath(sys.argv[0]))

# Print useful information during runtime if possible.
DEBUG = True
TEMPLATE_DEBUG = DEBUG

# Sqlite3 database configuration, though we don't use it right now.
DATABASE_ENGINE = 'sqlite3'
DATABASE_NAME = os.path.join(ROOT_PATH, 'monitor.db')

# Local time zone for this installation.
TIME_ZONE = 'America/Los_Angeles'

# Language code for this installation.
LANGUAGE_CODE = 'en-us'

# If you set this to False, Django will make some optimizations so as not
# to load the internationalization machinery.
USE_I18N = True

# Absolute path to the directory that holds media.
MEDIA_ROOT = os.path.join(ROOT_PATH, 'static') + '/'

# URL that handles the media served from MEDIA_ROOT. Make sure to use a
# trailing slash if there is a path component (optional in other cases).
MEDIA_URL = '/static/'

# Used to provide a seed in secret-key hashing algorithms.  Make this unique,
# and don't share it with anybody.
SECRET_KEY = '13p5p_4q91*8@yo+tvvt#2k&6#d_&e_zvxdpdil53k419i5sop'

# A string representing the full Python import path to your root URLconf.
ROOT_URLCONF = 'monitor.urls'

# List of locations of the template source files, in search order.
TEMPLATE_DIRS = (os.path.join(ROOT_PATH, 'templates'),)
