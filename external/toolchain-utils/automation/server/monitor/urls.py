# Copyright 2011 Google Inc. All Rights Reserved.
#

__author__ = 'kbaclawski@google.com (Krystian Baclawski)'

from django.conf import settings
from django.conf.urls.defaults import patterns

urlpatterns = patterns(
    'dashboard', (r'^job-group$', 'JobGroupListPageHandler'),
    (r'^machine$', 'MachineListPageHandler'),
    (r'^job/(?P<job_id>\d+)/log$', 'LogPageHandler'),
    (r'^job/(?P<job_id>\d+)$', 'JobPageHandler'), (
        r'^job-group/(?P<job_group_id>\d+)/files/(?P<path>.*)$',
        'JobGroupFilesPageHandler'),
    (r'^job-group/(?P<job_group_id>\d+)$', 'JobGroupPageHandler'),
    (r'^$', 'DefaultPageHandler'))

urlpatterns += patterns('',
                        (r'^static/(?P<path>.*)$', 'django.views.static.serve',
                         {'document_root': settings.MEDIA_ROOT}))
