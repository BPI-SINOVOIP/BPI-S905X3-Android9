# Copyright 2011 Google Inc. All Rights Reserved.
#

__author__ = 'kbaclawski@google.com (Krystian Baclawski)'

from collections import namedtuple
import glob
import gzip
import os.path
import pickle
import time
import xmlrpclib

from django import forms
from django.http import HttpResponseRedirect
from django.shortcuts import render_to_response
from django.template import Context
from django.views import static

Link = namedtuple('Link', 'href name')


def GetServerConnection():
  return xmlrpclib.Server('http://localhost:8000')


def MakeDefaultContext(*args):
  context = Context({'links': [
      Link('/job-group', 'Job Groups'), Link('/machine', 'Machines')
  ]})

  for arg in args:
    context.update(arg)

  return context


class JobInfo(object):

  def __init__(self, job_id):
    self._job = pickle.loads(GetServerConnection().GetJob(job_id))

  def GetAttributes(self):
    job = self._job

    group = [Link('/job-group/%d' % job.group.id, job.group.label)]

    predecessors = [Link('/job/%d' % pred.id, pred.label)
                    for pred in job.predecessors]

    successors = [Link('/job/%d' % succ.id, succ.label)
                  for succ in job.successors]

    machines = [Link('/machine/%s' % mach.hostname, mach.hostname)
                for mach in job.machines]

    logs = [Link('/job/%d/log' % job.id, 'Log')]

    commands = enumerate(job.PrettyFormatCommand().split('\n'), start=1)

    return {'text': [('Label', job.label), ('Directory', job.work_dir)],
            'link': [('Group', group), ('Predecessors', predecessors),
                     ('Successors', successors), ('Machines', machines),
                     ('Logs', logs)],
            'code': [('Command', commands)]}

  def GetTimeline(self):
    return [{'started': evlog.GetTimeStartedFormatted(),
             'state_from': evlog.event.from_,
             'state_to': evlog.event.to_,
             'elapsed': evlog.GetTimeElapsedRounded()}
            for evlog in self._job.timeline.GetTransitionEventHistory()]

  def GetLog(self):
    log_path = os.path.join(self._job.logs_dir,
                            '%s.gz' % self._job.log_filename_prefix)

    try:
      log = gzip.open(log_path, 'r')
    except IOError:
      content = []
    else:
      # There's a good chance that file is not closed yet, so EOF handling
      # function and CRC calculation will fail, thus we need to monkey patch the
      # _read_eof method.
      log._read_eof = lambda: None

      def SplitLine(line):
        prefix, msg = line.split(': ', 1)
        datetime, stream = prefix.rsplit(' ', 1)

        return datetime, stream, msg

      content = map(SplitLine, log.readlines())
    finally:
      log.close()

    return content


class JobGroupInfo(object):

  def __init__(self, job_group_id):
    self._job_group = pickle.loads(GetServerConnection().GetJobGroup(
        job_group_id))

  def GetAttributes(self):
    group = self._job_group

    home_dir = [Link('/job-group/%d/files/' % group.id, group.home_dir)]

    return {'text': [('Label', group.label),
                     ('Time submitted', time.ctime(group.time_submitted)),
                     ('State', group.status),
                     ('Cleanup on completion', group.cleanup_on_completion),
                     ('Cleanup on failure', group.cleanup_on_failure)],
            'link': [('Directory', home_dir)]}

  def _GetJobStatus(self, job):
    status_map = {'SUCCEEDED': 'success', 'FAILED': 'failure'}
    return status_map.get(str(job.status), None)

  def GetJobList(self):
    return [{'id': job.id,
             'label': job.label,
             'state': job.status,
             'status': self._GetJobStatus(job),
             'elapsed': job.timeline.GetTotalTime()}
            for job in self._job_group.jobs]

  def GetHomeDirectory(self):
    return self._job_group.home_dir

  def GetReportList(self):
    job_dir_pattern = os.path.join(self._job_group.home_dir, 'job-*')

    filenames = []

    for job_dir in glob.glob(job_dir_pattern):
      filename = os.path.join(job_dir, 'report.html')

      if os.access(filename, os.F_OK):
        filenames.append(filename)

    reports = []

    for filename in sorted(filenames, key=lambda f: os.stat(f).st_ctime):
      try:
        with open(filename, 'r') as report:
          reports.append(report.read())
      except IOError:
        pass

    return reports


class JobGroupListInfo(object):

  def __init__(self):
    self._all_job_groups = pickle.loads(GetServerConnection().GetAllJobGroups())

  def _GetJobGroupState(self, group):
    return str(group.status)

  def _GetJobGroupStatus(self, group):
    status_map = {'SUCCEEDED': 'success', 'FAILED': 'failure'}
    return status_map.get(self._GetJobGroupState(group), None)

  def GetList(self):
    return [{'id': group.id,
             'label': group.label,
             'submitted': time.ctime(group.time_submitted),
             'state': self._GetJobGroupState(group),
             'status': self._GetJobGroupStatus(group)}
            for group in self._all_job_groups]

  def GetLabelList(self):
    return sorted(set(group.label for group in self._all_job_groups))


def JobPageHandler(request, job_id):
  job = JobInfo(int(job_id))

  ctx = MakeDefaultContext({
      'job_id': job_id,
      'attributes': job.GetAttributes(),
      'timeline': job.GetTimeline()
  })

  return render_to_response('job.html', ctx)


def LogPageHandler(request, job_id):
  job = JobInfo(int(job_id))

  ctx = MakeDefaultContext({'job_id': job_id, 'log_lines': job.GetLog()})

  return render_to_response('job_log.html', ctx)


def JobGroupPageHandler(request, job_group_id):
  group = JobGroupInfo(int(job_group_id))

  ctx = MakeDefaultContext({
      'group_id': job_group_id,
      'attributes': group.GetAttributes(),
      'job_list': group.GetJobList(),
      'reports': group.GetReportList()
  })

  return render_to_response('job_group.html', ctx)


def JobGroupFilesPageHandler(request, job_group_id, path):
  group = JobGroupInfo(int(job_group_id))

  return static.serve(request,
                      path,
                      document_root=group.GetHomeDirectory(),
                      show_indexes=True)


class FilterJobGroupsForm(forms.Form):
  label = forms.ChoiceField(label='Filter by label:', required=False)


def JobGroupListPageHandler(request):
  groups = JobGroupListInfo()
  group_list = groups.GetList()

  field = FilterJobGroupsForm.base_fields['label']
  field.choices = [('*', '--- no filtering ---')]
  field.choices.extend([(label, label) for label in groups.GetLabelList()])

  if request.method == 'POST':
    form = FilterJobGroupsForm(request.POST)

    if form.is_valid():
      label = form.cleaned_data['label']

      if label != '*':
        group_list = [group for group in group_list if group['label'] == label]
  else:
    form = FilterJobGroupsForm({'initial': '*'})

  ctx = MakeDefaultContext({'filter': form, 'groups': group_list})

  return render_to_response('job_group_list.html', ctx)


def MachineListPageHandler(request):
  machine_list = pickle.loads(GetServerConnection().GetMachineList())

  return render_to_response('machine_list.html',
                            MakeDefaultContext({'machines': machine_list}))


def DefaultPageHandler(request):
  return HttpResponseRedirect('/job-group')
