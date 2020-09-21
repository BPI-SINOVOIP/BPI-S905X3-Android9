# Copyright 2010 Google Inc. All Rights Reserved.

from itertools import chain
import gzip
import logging
import logging.handlers
import time
import traceback


def SetUpRootLogger(filename=None, level=None, display_flags={}):
  console_handler = logging.StreamHandler()
  console_handler.setFormatter(CustomFormatter(AnsiColorCoder(), display_flags))
  logging.root.addHandler(console_handler)

  if filename:
    file_handler = logging.handlers.RotatingFileHandler(
        filename,
        maxBytes=10 * 1024 * 1024,
        backupCount=9,
        delay=True)
    file_handler.setFormatter(CustomFormatter(NullColorCoder(), display_flags))
    logging.root.addHandler(file_handler)

  if level:
    logging.root.setLevel(level)


class NullColorCoder(object):

  def __call__(self, *args):
    return ''


class AnsiColorCoder(object):
  CODES = {'reset': (0,),
           'bold': (1, 22),
           'italics': (3, 23),
           'underline': (4, 24),
           'inverse': (7, 27),
           'strikethrough': (9, 29),
           'black': (30, 40),
           'red': (31, 41),
           'green': (32, 42),
           'yellow': (33, 43),
           'blue': (34, 44),
           'magenta': (35, 45),
           'cyan': (36, 46),
           'white': (37, 47)}

  def __call__(self, *args):
    codes = []

    for arg in args:
      if arg.startswith('bg-') or arg.startswith('no-'):
        codes.append(self.CODES[arg[3:]][1])
      else:
        codes.append(self.CODES[arg][0])

    return '\033[%sm' % ';'.join(map(str, codes))


class CustomFormatter(logging.Formatter):
  COLORS = {'DEBUG': ('white',),
            'INFO': ('green',),
            'WARN': ('yellow', 'bold'),
            'ERROR': ('red', 'bold'),
            'CRIT': ('red', 'inverse', 'bold')}

  def __init__(self, coder, display_flags={}):
    items = []

    if display_flags.get('datetime', True):
      items.append('%(asctime)s')
    if display_flags.get('level', True):
      items.append('%(levelname)s')
    if display_flags.get('name', True):
      items.append(coder('cyan') + '[%(threadName)s:%(name)s]' + coder('reset'))
    items.append('%(prefix)s%(message)s')

    logging.Formatter.__init__(self, fmt=' '.join(items))

    self._coder = coder

  def formatTime(self, record):
    ct = self.converter(record.created)
    t = time.strftime('%Y-%m-%d %H:%M:%S', ct)
    return '%s.%02d' % (t, record.msecs / 10)

  def formatLevelName(self, record):
    if record.levelname in ['WARNING', 'CRITICAL']:
      levelname = record.levelname[:4]
    else:
      levelname = record.levelname

    return ''.join([self._coder(*self.COLORS[levelname]), levelname,
                    self._coder('reset')])

  def formatMessagePrefix(self, record):
    try:
      return ' %s%s:%s ' % (self._coder('black', 'bold'), record.prefix,
                            self._coder('reset'))
    except AttributeError:
      return ''

  def format(self, record):
    if record.exc_info:
      if not record.exc_text:
        record.exc_text = self.formatException(record.exc_info)
    else:
      record.exc_text = ''

    fmt = record.__dict__.copy()
    fmt.update({'levelname': self.formatLevelName(record),
                'asctime': self.formatTime(record),
                'prefix': self.formatMessagePrefix(record)})

    s = []

    for line in chain(record.getMessage().splitlines(),
                      record.exc_text.splitlines()):
      fmt['message'] = line

      s.append(self._fmt % fmt)

    return '\n'.join(s)


class CompressedFileHandler(logging.FileHandler):

  def _open(self):
    return gzip.open(self.baseFilename + '.gz', self.mode, 9)


def HandleUncaughtExceptions(fun):
  """Catches all exceptions that would go outside decorated fun scope."""

  def _Interceptor(*args, **kwargs):
    try:
      return fun(*args, **kwargs)
    except StandardError:
      logging.exception('Uncaught exception:')

  return _Interceptor
