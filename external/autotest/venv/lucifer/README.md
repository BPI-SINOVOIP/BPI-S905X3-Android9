# lucifer

[TOC]

This is the Python component of lucifer.  See the [design
doc](http://goto.google.com/monitor_db_per_job_refactor).

See also the Go
[component](https://chromium.googlesource.com/chromiumos/infra/lucifer)

## Overview

lucifer provides two commands.  From the repository root, they are:

- `bin/job_reporter`
- `bin/job_aborter`

`job_reporter` runs an Autotest job. `job_aborter` is a daemon that
cleans up jobs that crash and aborts jobs using the AFE database.

## Development

To run all tests, in the repository root, run:

    $ bin/test_lucifer

To skip somewhat slower tests (0.10s or more):

    $ bin/test_lucifer --skipslow
