#!/usr/bin/python
#
# Copyright 2010 Google Inc. All Rights Reserved.

import logging
import optparse
import pickle
import signal
from SimpleXMLRPCServer import SimpleXMLRPCServer
import sys

from automation.common import logger
from automation.common.command_executer import CommandExecuter
from automation.server import machine_manager
from automation.server.job_group_manager import JobGroupManager
from automation.server.job_manager import JobManager


class Server(object):
  """Plays a role of external interface accessible over XMLRPC."""

  def __init__(self, machines_file=None, dry_run=False):
    """Default constructor.

    Args:
      machines_file: Path to file storing a list of machines.
      dry_run: If True, the server only simulates command execution.
    """
    CommandExecuter.Configure(dry_run)

    self.job_manager = JobManager(
        machine_manager.MachineManager.FromMachineListFile(
            machines_file or machine_manager.DEFAULT_MACHINES_FILE))

    self.job_group_manager = JobGroupManager(self.job_manager)

    self._logger = logging.getLogger(self.__class__.__name__)

  def ExecuteJobGroup(self, job_group, dry_run=False):
    job_group = pickle.loads(job_group)
    self._logger.info('Received ExecuteJobGroup(%r, dry_run=%s) request.',
                      job_group, dry_run)

    for job in job_group.jobs:
      job.dry_run = dry_run
    return self.job_group_manager.AddJobGroup(job_group)

  def GetAllJobGroups(self):
    self._logger.info('Received GetAllJobGroups() request.')
    return pickle.dumps(self.job_group_manager.GetAllJobGroups())

  def KillJobGroup(self, job_group_id):
    self._logger.info('Received KillJobGroup(%d) request.', job_group_id)
    self.job_group_manager.KillJobGroup(pickle.loads(job_group_id))

  def GetJobGroup(self, job_group_id):
    self._logger.info('Received GetJobGroup(%d) request.', job_group_id)

    return pickle.dumps(self.job_group_manager.GetJobGroup(job_group_id))

  def GetJob(self, job_id):
    self._logger.info('Received GetJob(%d) request.', job_id)

    return pickle.dumps(self.job_manager.GetJob(job_id))

  def GetMachineList(self):
    self._logger.info('Received GetMachineList() request.')

    return pickle.dumps(self.job_manager.machine_manager.GetMachineList())

  def StartServer(self):
    self.job_manager.StartJobManager()

  def StopServer(self):
    self.job_manager.StopJobManager()
    self.job_manager.join()


def GetServerOptions():
  """Get server's settings from command line options."""
  parser = optparse.OptionParser()
  parser.add_option('-m',
                    '--machines-file',
                    dest='machines_file',
                    help='The location of the file '
                    'containing the machines database',
                    default=machine_manager.DEFAULT_MACHINES_FILE)
  parser.add_option('-n',
                    '--dry-run',
                    dest='dry_run',
                    help='Start the server in dry-run mode, where jobs will '
                    'not actually be executed.',
                    action='store_true',
                    default=False)
  return parser.parse_args()[0]


def Main():
  logger.SetUpRootLogger(filename='server.log', level=logging.DEBUG)

  options = GetServerOptions()
  server = Server(options.machines_file, options.dry_run)
  server.StartServer()

  def _HandleKeyboardInterrupt(*_):
    server.StopServer()
    sys.exit(1)

  signal.signal(signal.SIGINT, _HandleKeyboardInterrupt)

  try:
    xmlserver = SimpleXMLRPCServer(
        ('localhost', 8000),
        allow_none=True,
        logRequests=False)
    xmlserver.register_instance(server)
    xmlserver.serve_forever()
  except Exception as ex:
    logging.error(ex)
    server.StopServer()
    sys.exit(1)


if __name__ == '__main__':
  Main()
