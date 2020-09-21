# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import logging
import re
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error


def get_histogram_text(tab, histogram_name):
     """
     This returns contents of the given histogram.

     @param tab: object, Chrome tab instance
     @param histogram_name: string, name of the histogram
     @returns string: contents of the histogram
     """
     docEle = 'document.documentElement'
     tab.Navigate('chrome://histograms/%s' % histogram_name)
     tab.WaitForDocumentReadyStateToBeComplete()
     raw_text = tab.EvaluateJavaScript(
          '{0} && {0}.innerText'.format(docEle))
     # extract the contents of the histogram
     histogram = raw_text[raw_text.find('Histogram:'):].strip()
     if histogram:
          logging.debug('chrome://histograms/%s:\n%s', histogram_name, histogram)
     else:
          logging.debug('No histogram is shown in chrome://histograms/%s',
                        histogram_name)
     return histogram


def loaded(tab, histogram_name, pattern):
     """
     Checks if the histogram page has been fully loaded.

     @param tab: object, Chrome tab instance
     @param histogram_name: string, name of the histogram
     @param pattern: string, required text to look for
     @returns re.MatchObject if the given pattern is found in the text
              None otherwise

     """
     return re.search(pattern, get_histogram_text(tab, histogram_name))


def  verify(cr, histogram_name, histogram_bucket_value):
     """
     Verifies histogram string and success rate in a parsed histogram bucket.
     The histogram buckets are outputted in debug log regardless of the
     verification result.

     Full histogram URL is used to load histogram. Example Histogram URL is :
     chrome://histograms/Media.GpuVideoDecoderInitializeStatus

     @param cr: object, the Chrome instance
     @param histogram_name: string, name of the histogram
     @param histogram_bucket_value: int, required bucket number to look for
     @raises error.TestError if histogram is not successful

     """
     bucket_pattern = '\n'+ str(histogram_bucket_value) +'.*100\.0%.*'
     error_msg_format = ('{} not loaded or histogram bucket not found '
                         'or histogram bucket found at < 100%')
     tab = cr.browser.tabs.New()
     msg = error_msg_format.format(histogram_name)
     utils.poll_for_condition(lambda : loaded(tab, histogram_name,
                                              bucket_pattern),
                              exception=error.TestError(msg),
                              sleep_interval=1)


def is_bucket_present(cr,histogram_name, histogram_bucket_value):
     """
     This returns histogram succes or fail to called function

     @param cr: object, the Chrome instance
     @param histogram_name: string, name of the histogram
     @param histogram_bucket_value: int, required bucket number to look for
     @returns True if histogram page was loaded and the bucket was found.
              False otherwise

     """
     try:
          verify(cr,histogram_name, histogram_bucket_value)
     except error.TestError:
          return False
     else:
          return True


def is_histogram_present(cr, histogram_name):
     """
     This checks if the given histogram is present and non-zero.

     @param cr: object, the Chrome instance
     @param histogram_name: string, name of the histogram
     @returns True if histogram page was loaded and the histogram is present
              False otherwise

     """
     histogram_pattern = 'Histogram: '+ histogram_name + ' recorded ' + \
                         r'[1-9][0-9]*' + ' samples'
     tab = cr.browser.tabs.New()
     try:
          utils.poll_for_condition(lambda : loaded(tab, histogram_name,
                                                   histogram_pattern),
                                   timeout=2,
                                   sleep_interval=0.1)
          return True
     except utils.TimeoutError:
          # the histogram is not present, and then returns false
          return False


def get_histogram(cr, histogram_name):
     """
     This returns contents of the given histogram.

     @param cr: object, the Chrome instance
     @param histogram_name: string, name of the histogram
     @returns string: contents of the histogram

     """
     tab = cr.browser.tabs.New()
     return get_histogram_text(tab, histogram_name)
