# Coding style for autotest in Chrome OS / Android / Brillo
These rules elaborate on, but rarely deviate from PEP-8.  When in doubt, go
with PEP-8.


## Language
 * Use Python where possible
 * Prefer writing more Python to a smaller amount of shell script in host
   commands.  In practice, the Python tends to be easier to maintain.
 * Some tests use C/C++ in test dependencies, and this is also ok.


## Indentation & whitespace

Format your code for an 80 character wide screen.

Indentation is 4 spaces, as opposed to hard tabs (which it used to be).
This is the Python standard.

For hanging indentation, use 8 spaces plus all args should be on the new line.

```

     # Either of the following hanging indentation is considered acceptable.
YES: return 'class: %s, host: %s, args = %s' % (
             self.__class__.__name__, self.hostname, self.args)

YES: return 'class: %s, host: %s, args = %s' % (
             self.__class__.__name__,
             self.hostname,
             self.args)

     # Do not use 4 spaces for hanging indentation
NO:  return 'class: %s, host: %s, args = %s' % (
         self.__class__.__name__, self.hostname, self.args)

     # Do put all args on new line
NO:  return 'class: %s, host: %s, args = %s' % (self.__class__.__name__,
             self.hostname, self.args)
```

Don't leave trailing whitespace, or put whitespace on blank lines.


## Variable names and UpPeR cAsE
 * Use descriptive variable names where possible
 * Use `variable_names_like_this`
 * Use `method_and_function_names_like_this`
 * Use `UpperCamelCase` for class names


## Importing modules

The order of imports should be as follows:

 * Standard python modules
 * Non-standard python modules
 * Autotest modules

Within one of these three sections, all module imports using the from
keyword should appear after regular imports.
Each module should be imported on its own line.
Do not use Wildcard imports (`from x import *`) if possible.

Import modules, not classes.  For example:

```
from common_lib import error

def foo():
    raise error.AutoservError(...)
```

and not:

```
from common_lib.error import AutoservError
```

For example:

```
import os
import pickle
import random
import re
import select
import shutil
import signal
import subprocess

import common   # Magic autotest_lib module and sys.path setup code.
import MySQLdb  # After common so that we check our site-packages first.

from common_lib import error
```

## Testing None

Use `is None` rather than `== None` and `is not None` rather than `!= None`.
This way you'll never run into a case where someone's `__eq__` or `__ne__`
method does the wrong thing.


## Comments

Generally, you want your comments to tell WHAT your code does, not HOW.
We can figure out how from the code itself (or if not, your code needs fixing).

Try to describle the intent of a function and what it does in a triple-quoted
(multiline) string just after the def line. We've tried to do that in most
places, though undoubtedly we're not perfect. A high level overview is
incredibly helpful in understanding code.


## Hardcoded String Formatting

Strings should use only single quotes for hardcoded strings in the code. Double
quotes are acceptable when single quote is used as part of the string.
Multiline strings should not use '\' but wrap the string using parentheseses.

```
REALLY_LONG_STRING = ('This is supposed to be a really long string that is '
                      'over 80 characters and does not use a slash to '
                      'continue.')
```

## Docstrings

Docstrings are important to keep our code self documenting. While it's not
necessary to overdo documentation, we ask you to be sensible and document any
nontrivial function. When creating docstrings, please add a newline at the
beginning of your triple quoted string and another newline at the end of it. If
the docstring has multiple lines, please include a short summary line followed
by a blank line before continuing with the rest of the description. Please
capitalize and punctuate accordingly the sentences. If the description has
multiple lines, put two levels of indentation before proceeding with text. An
example docstring:

```
def foo(param1, param2):
    """
    Summary line.

    Long description of method foo.

    @param param1: A thing called param1 that is used for a bunch of stuff
            that has methods bar() and baz() which raise SpamError if
            something goes awry.

    @returns a list of integers describing changes in a source tree

    @raises exception that could be raised if a certain condition occurs.

    """
```

The tags that you can put inside your docstring are tags recognized by systems
like doxygen. Not all places need all tags defined, so choose them wisely while
writing code. Generally (if applicable) always list parameters, return value
(if there is one), and exceptions that can be raised to each docstring.

|   Tag    | Description                                                                              |
|----------|------------------------------------------------------------------------------------------|
| @author  | Code author                                                                              |
| @param   | Parameter description                                                                    |
| @raise   | If the function can throw an exception, this tag documents the possible exception types. |
| @raises  | Same as @raise.                                                                          |
| @return  | Return value description                                                                 |
| @returns | Same as @return                                                                          |
| @see     | Reference to other parts of the codebase.                                                |
| @warning | Call attention to potential problems with the code                                       |
| @var     | Documentation for a variable or enum value (either global or as a member of a class)     |
| @version | Version string                                                                           |

When in doubt refer to: http://doxygen.nl/commands.html

## Simple code

Keep it simple; this is not the right place to show how smart you are. We
have plenty of system failures to deal with without having to spend ages
figuring out your code, thanks ;-) Readbility, readability, readability.
Strongly prefer readability and simplicity over compactness.

"Debugging is twice as hard as writing the code in the first place. Therefore,
if you write the code as cleverly as possible, you are, by definition, not
smart enough to debug it."  Brian Kernighan


## Function length

Please keep functions short, under 30 lines or so if possible. Even though
you are amazingly clever, and can cope with it, the rest of us are busy and
stupid, so be nice and help us out. To quote the Linux Kernel coding style:

Functions should be short and sweet, and do just one thing.  They should
fit on one or two screenfuls of text (the ISO/ANSI screen size is 80x24,
as we all know), and do one thing and do that well.


## Exceptions

When raising exceptions, the preferred syntax for it is:

```
raise FooException('Exception Message')
```

Please don't raise string exceptions, as they're deprecated and will be removed
from future versions of python. If you're in doubt about what type of exception
you will raise, please look at http://docs.python.org/lib/module-exceptions.html
and client/common\_lib/error.py, the former is a list of python built in
exceptions and the later is a list of autotest/autoserv internal exceptions. Of
course, if you really need to, you can extend the exception definitions on
client/common\_lib/error.py.


## Submitting patches

Submit changes through the Chrome OS gerrit instance.  This process is
documented on
[chromium.org](http://dev.chromium.org/developers/contributing-code).
