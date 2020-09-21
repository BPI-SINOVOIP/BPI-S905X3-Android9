RAPPOR
======

RAPPOR is a novel privacy technology that allows inferring statistics about
populations while preserving the privacy of individual users.

This repository contains simulation and analysis code in Python and R.

For a detailed description of the algorithms, see the
[paper](http://arxiv.org/abs/1407.6981) and links below.

Feel free to send feedback to
[rappor-discuss@googlegroups.com][group].

-------------

- [RAPPOR Data Flow](http://google.github.io/rappor/doc/data-flow.html)

Publications
------------

- [RAPPOR: Randomized Aggregatable Privacy-Preserving Ordinal Response](http://arxiv.org/abs/1407.6981)
- [Building a RAPPOR with the Unknown: Privacy-Preserving Learning of Associations and Data Dictionaries](http://arxiv.org/abs/1503.01214)

Links
-----

- [Google Blog Post about RAPPOR](http://googleresearch.blogspot.com/2014/10/learning-statistics-with-privacy-aided.html)
- [RAPPOR implementation in Chrome](http://www.chromium.org/developers/design-documents/rappor)
  - This is a production quality C++ implementation, but it's somewhat tied to
    Chrome, and doesn't support all privacy parameters (e.g. only a few values
    of p and q).  On the other hand, the code in this repo is not yet
    production quality, but supports experimentation with different parameters
    and data sets.  Of course, anyone is free to implement RAPPOR independently
    as well.
- Mailing list: [rappor-discuss@googlegroups.com][group]

[group]: https://groups.google.com/forum/#!forum/rappor-discuss
