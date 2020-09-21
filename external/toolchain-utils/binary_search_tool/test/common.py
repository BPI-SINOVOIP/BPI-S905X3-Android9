#!/usr/bin/env python2
"""Common utility functions."""

DEFAULT_OBJECT_NUMBER = 1238
DEFAULT_BAD_OBJECT_NUMBER = 23
OBJECTS_FILE = 'objects.txt'
WORKING_SET_FILE = 'working_set.txt'


def ReadWorkingSet():
  working_set = []
  f = open(WORKING_SET_FILE, 'r')
  for l in f:
    working_set.append(int(l))
  f.close()
  return working_set


def WriteWorkingSet(working_set):
  f = open(WORKING_SET_FILE, 'w')
  for o in working_set:
    f.write('{0}\n'.format(o))
  f.close()


def ReadObjectsFile():
  objects_file = []
  f = open(OBJECTS_FILE, 'r')
  for l in f:
    objects_file.append(int(l))
  f.close()
  return objects_file


def ReadObjectIndex(filename):
  object_index = []
  f = open(filename, 'r')
  for o in f:
    object_index.append(int(o))
  f.close()
  return object_index
