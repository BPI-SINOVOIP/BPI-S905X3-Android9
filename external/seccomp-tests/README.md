# Seccomp-BPF Kernel Self-Test Suite

This repository contains a mirror of the upstream Linux kernel test suite for the Seccomp-BPF
system call filter. The test suite runs as part of CTS, but it is maintained in a separate
repository because the code is GPL.

## Syncing to Upstream

Rather than hold the entire Linux history in this repository, only the subdirectory for the Seccomp
selftests are preserved here. In order to sync this repository to the upstream Linux, follow these
instructions.

The pristine copy of the upstream source is kept on a branch called upstream-master. This branch is
then merged into an Android development branch.

### First-Time Setup

These instructions only need to be followed for the first time you are updating the repository from
a checkout.

1. Configure a remote to use as the source repository (limited to only syncing the master branch):
    ```
    git remote add upstream-linux git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git -t master --no-tags
    ```

### Updating the Source

Perform these steps every time you need to update the test suite from upstream.

1. Update the remote to fetch the latest sources:
    ```
    git remote update upstream-linux
    ```

2. Create a new local branch from the updated source, replacing YYYYMMDD with today's date:
    ```
    git checkout -b update-YYYYMMDD upstream-linux/master
    ```

3. Filter the branch to just the subtree containing the Seccomp test suite:
    ```
    git filter-branch --subdirectory-filter tools/testing/selftests/seccomp
    ```

4. Check out the upstream-master branch, which contains the pristine, filter-branch'd copy of the
source code. Pushing non-merge commits with a "forged" author/committer can only be done against
the upstream-master branch.
    ```
    git checkout -b upstream-master aosp/upstream-master
    ````

5. Update this upstream-master branch to the newly filtered branch of upstream-linux.
    ```
    git merge --ff-only update-YYYYMMDD
    ```

6. Upload the changes on upstream-master for review and submit them.

7. Merge the changes from upstream-master into the Android development branch (typically master).
Resolve any conflicts with the local modifications present in the repository.
    ```
    repo start sync-upstream .
    git subtree merge -P linux/ upstream-master
    ```

Now build and test the changes by running CTS:

    $ mmma cts/tests/tests/os
    $ cts-tradefed run singleCommand cts -m CtsOsTestCases -t android.os.cts.SeccompTest

The tests are expected to pass on arm, arm64, x86, and x86\_64. If they pass, then repo
upload/submit the CL branch. Afterwards, you can remove the update-YYYYMMDD branch.

### Linux Space-Saving

If you already have a Linux kernel checkout, you can skip adding Linux as a remote and instead
perform steps 1-3 of "Updating the Source" in the kernel checkout. Then simply fetch the filtered
branch into the seccomp-tests repository and subtree merge it (as FETCH\_HEAD). This will avoid
copying the entire kernel history into your local checkout.
