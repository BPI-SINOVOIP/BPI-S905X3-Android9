# Copyright 2011 Google Inc. All Rights Reserved.
"""A global variable for testing."""

is_test = [False]


def SetTestMode(flag):
  is_test[0] = flag


def GetTestMode():
  return is_test[0]
