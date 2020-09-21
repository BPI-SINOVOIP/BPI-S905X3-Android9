# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This module is designed to report metadata in a separated thread to avoid the
performance overhead of sending data to Elasticsearch using HTTP.

"""

import logging
import Queue
import socket
import time
import threading

import common
from autotest_lib.client.common_lib import utils

try:
    from chromite.lib import metrics
except ImportError:
    metrics = utils.metrics_mock


_METADATA_METRICS_PREFIX = 'chromeos/autotest/es_metadata_reporter/'

# Number of seconds to wait before checking queue again for uploading data.
_REPORT_INTERVAL_SECONDS = 5

_MAX_METADATA_QUEUE_SIZE = 1000000
_MAX_UPLOAD_SIZE = 50000
# The number of seconds for upload to fail continuously. After that, upload will
# be limited to 1 entry.
_MAX_UPLOAD_FAIL_DURATION = 600
# Number of entries to retry when the previous upload failed continueously for
# the duration of _MAX_UPLOAD_FAIL_DURATION.
_MIN_RETRY_ENTRIES = 10
# Queue to buffer metadata to be reported.
metadata_queue = Queue.Queue(_MAX_METADATA_QUEUE_SIZE)

_report_lock = threading.Lock()
_abort = threading.Event()
_queue_full = threading.Event()
_metrics_fields = {}

def  _get_metrics_fields():
    """Get the fields information to be uploaded to metrics."""
    if not _metrics_fields:
        _metrics_fields['hostname'] = socket.gethostname()

    return _metrics_fields


def queue(data):
    """Queue metadata to be uploaded in reporter thread.

    If the queue is full, an error will be logged for the first time the queue
    becomes full. The call does not wait or raise Queue.Full exception, so
    there is no overhead on the performance of caller, e.g., scheduler.

    @param data: A metadata entry, which should be a dictionary.
    """
    if not is_running():
        return

    try:
        metadata_queue.put_nowait(data)
        if _queue_full.is_set():
            logging.info('Metadata queue is available to receive new data '
                         'again.')
            _queue_full.clear()
    except Queue.Full:
        if not _queue_full.is_set():
            _queue_full.set()
            logging.error('Metadata queue is full, cannot report data. '
                          'Consider increasing the value of '
                          '_MAX_METADATA_QUEUE_SIZE. Its current value is set '
                          'to %d.', _MAX_METADATA_QUEUE_SIZE)


def _run():
    """Report metadata in the queue until being aborted.
    """
    # Time when the first time upload failed. None if the last upload succeeded.
    first_failed_upload = None
    upload_size = _MIN_RETRY_ENTRIES

    try:
        while True:
            start_time = time.time()
            data_list = []
            if (first_failed_upload and
                time.time() - first_failed_upload > _MAX_UPLOAD_FAIL_DURATION):
                upload_size = _MIN_RETRY_ENTRIES
            else:
                upload_size = min(upload_size*2, _MAX_UPLOAD_SIZE)
            while (not metadata_queue.empty() and len(data_list) < upload_size):
                data_list.append(metadata_queue.get_nowait())
            if data_list:
                success = False
                fields = _get_metrics_fields().copy()
                fields['success'] = success
                metrics.Gauge(
                        _METADATA_METRICS_PREFIX + 'upload/batch_sizes').set(
                                len(data_list), fields=fields)
                metrics.Counter(
                        _METADATA_METRICS_PREFIX + 'upload/attempts').increment(
                                fields=fields);

            metrics.Gauge(_METADATA_METRICS_PREFIX + 'queue_size').set(
                    metadata_queue.qsize(), fields=_get_metrics_fields())
            sleep_time = _REPORT_INTERVAL_SECONDS - time.time() + start_time
            if sleep_time < 0:
                sleep_time = 0.5
            _abort.wait(timeout=sleep_time)
    except Exception as e:
        logging.exception('Metadata reporter thread failed with error: %s', e)
        raise
    finally:
        logging.info('Metadata reporting thread is exiting.')
        _abort.clear()
        _report_lock.release()


def is_running():
    """Check if metadata_reporter is running.

    @return: True if metadata_reporter is running.
    """
    return _report_lock.locked()


def start():
    """Start the thread to report metadata.
    """
    # The lock makes sure there is only one reporting thread working.
    if is_running():
        logging.error('There is already a metadata reporter thread.')
        return

    logging.warn('Elasticsearch db deprecated, no metadata will be '
                 'reported.')

    _report_lock.acquire()
    reporting_thread = threading.Thread(target=_run)
    # Make it a daemon thread so it doesn't need to be closed explicitly.
    reporting_thread.setDaemon(True)
    reporting_thread.start()
    logging.info('Metadata reporting thread is started.')


def abort():
    """Abort the thread to report metadata.

    The call will wait up to 5 seconds for existing data to be uploaded.
    """
    if  not is_running():
        logging.error('The metadata reporting thread has already exited.')
        return

    _abort.set()
    logging.info('Waiting up to %s seconds for metadata reporting thread to '
                 'complete.', _REPORT_INTERVAL_SECONDS)
    _abort.wait(_REPORT_INTERVAL_SECONDS)
