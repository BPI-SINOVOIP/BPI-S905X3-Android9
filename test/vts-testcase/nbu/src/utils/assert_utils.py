"""Utility functions for handling async events.
"""


from mobly import asserts
from mobly.controllers.android_device_lib import callback_handler


def AssertAsyncSuccess(callback):
  """Wrapping the result of an asynchronous Rpc for convenience.

  This wrapper handles the event posted by the corresponding `FutureCallback`
  object signalling whether the async operation was successful or not with
  `onSuccess` and `onFailure` callbacks.

  Args:
    callback: CallbackHandler, the handler that's used to poll events triggered
    by the async Rpc call.

  Returns:
    dictionary, the event that carries the onSuccess signal.
  """
  try:
    status_event = callback.waitAndGet('onSuccess', 30)
  except callback_handler.TimeoutError:
    try:
      failure_event = callback.waitAndGet('onFailure', 30)
      asserts.fail('Operation failed: %s' % failure_event.data)
    except callback_handler.TimeoutError:
      asserts.fail('Timed out waiting for status event.')
  return status_event

