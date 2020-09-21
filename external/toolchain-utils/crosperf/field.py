# Copyright 2011 Google Inc. All Rights Reserved.
"""Module to represent a Field in an experiment file."""


class Field(object):
  """Class representing a Field in an experiment file."""

  def __init__(self, name, required, default, inheritable, description):
    self.name = name
    self.required = required
    self.assigned = False
    self.default = default
    self._value = default
    self.inheritable = inheritable
    self.description = description

  def Set(self, value, parse=True):
    if parse:
      self._value = self._Parse(value)
    else:
      self._value = value
    self.assigned = True

  def Append(self, value):
    self._value += self._Parse(value)
    self.assigned = True

  def _Parse(self, value):
    return value

  def Get(self):
    return self._value

  def GetString(self):
    return str(self._value)


class TextField(Field):
  """Class of text field."""

  def __init__(self,
               name,
               required=False,
               default='',
               inheritable=False,
               description=''):
    super(TextField, self).__init__(name, required, default, inheritable,
                                    description)

  def _Parse(self, value):
    return str(value)


class BooleanField(Field):
  """Class of boolean field."""

  def __init__(self,
               name,
               required=False,
               default=False,
               inheritable=False,
               description=''):
    super(BooleanField, self).__init__(name, required, default, inheritable,
                                       description)

  def _Parse(self, value):
    if value.lower() == 'true':
      return True
    elif value.lower() == 'false':
      return False
    raise TypeError(
        "Invalid value for '%s'. Must be true or false." % self.name)


class IntegerField(Field):
  """Class of integer field."""

  def __init__(self,
               name,
               required=False,
               default=0,
               inheritable=False,
               description=''):
    super(IntegerField, self).__init__(name, required, default, inheritable,
                                       description)

  def _Parse(self, value):
    return int(value)


class FloatField(Field):
  """Class of float field."""

  def __init__(self,
               name,
               required=False,
               default=0,
               inheritable=False,
               description=''):
    super(FloatField, self).__init__(name, required, default, inheritable,
                                     description)

  def _Parse(self, value):
    return float(value)


class ListField(Field):
  """Class of list field."""

  def __init__(self,
               name,
               required=False,
               default=None,
               inheritable=False,
               description=''):
    super(ListField, self).__init__(name, required, default, inheritable,
                                    description)

  def _Parse(self, value):
    return value.split()

  def GetString(self):
    return ' '.join(self._value)

  def Append(self, value):
    v = self._Parse(value)
    if not self._value:
      self._value = v
    else:
      self._value += v
    self.assigned = True


class EnumField(Field):
  """Class of enum field."""

  def __init__(self,
               name,
               options,
               required=False,
               default='',
               inheritable=False,
               description=''):
    super(EnumField, self).__init__(name, required, default, inheritable,
                                    description)
    self.options = options

  def _Parse(self, value):
    if value not in self.options:
      raise TypeError("Invalid enum value for field '%s'. Must be one of (%s)" %
                      (self.name, ', '.join(self.options)))
    return str(value)
