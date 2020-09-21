# pylint: disable=missing-docstring

import sys, re, traceback

# these statuses are ordered such that a status earlier in the list will
# override a status later in a list (e.g. ERROR during a test will override
# prior GOOD results, but WARN will not override a FAIL)
job_statuses = ["TEST_NA", "ABORT", "ERROR", "FAIL", "WARN", "GOOD", "ALERT",
                "RUNNING", "NOSTATUS"]

def is_valid_status(status):
    if not re.match(r'(START|INFO|(END )?(' + '|'.join(job_statuses) + '))$',
                    status):
        return False
    else:
        return True


def log_and_ignore_errors(msg):
    """ A decorator for wrapping functions in a 'log exception and ignore'
    try-except block. """
    def decorator(fn):
        def decorated_func(*args, **dargs):
            try:
                fn(*args, **dargs)
            except Exception:
                print >> sys.stderr, msg
                traceback.print_exc(file=sys.stderr)
        return decorated_func
    return decorator
