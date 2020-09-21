# Test Finder Developer Guide

Learn about test finders and how to create a new test finder class.

##### Table of Contents
1. [Test Finder Details](#test-finder-details)
2. [Creating a Test Finder](#creating-a-test-finder)

## <a name="test-finder-details">Test Finder Details</a>

A test finder class holds find methods. A find method is given a string (the
user input) and should try to resolve that string into a ```TestInfo``` object.
A ```TestInfo``` object holds the test name, test dependencies, test runner, and
a data field to hold misc bits like filters and extra args for the test.  The
test finder class can hold multiple find methods. The find methods are grouped
together in a class so they can share metadata for optimal test finding.
Examples of metadata would be the ```ModuleInfo``` object or the dirs that hold
the test configs for the ```TFIntegrationFinder```.

**When should I create a new test finder class?**

If the metadata used to find a test is unlike existing test finder classes,
that is the right time to create a new class. Metadata can be anything like
file name patterns, a special file in a dir to indicate it's a test, etc. The
default test finder classes use the module-info.json and specific dir paths
metadata (```ModuleFinder``` and ```TFIntegrationFinder``` respectively).

## <a name="creating-a-test-finder">Creating a Test Finder</a>

First thing to choose is where to put the test finder. This will primarily
depend on if the test finder will be public or private. If public,
```test_finders/``` is the default location.

> If it will be private, then you can
> follow the same instructions for ```vendorsetup.sh``` in
> [constants override](atest_structure.md#constants-override) where you will
> add the path of where the test finder lives into ```$PYTHONPATH```. Same
> rules apply, rerun ```build/envsetup.sh``` to update ```$PYTHONPATH```.

Now define your class and decorate it with the
```test_finder_base.find_method_register``` decorator. This decorator will
create a list of find methods that ```test_finder_handler``` will use to collect
the find methods from your test finder class. Take a look at
```test_finders/example_test_finder.py``` as an example.

Define the find methods in your test finder class. These find methods must
return a ```TestInfo``` object. Extra bits of info can be stored in the data
field as a dict.  Check out ```ExampleFinder``` to see how the data field is
used.

Decorate each find method with the ```test_finder_base.register``` decorator.
This is used by the class decorator to identify the find methods of the class.

Final bit is to add your test finder class to ```test_finder_handler```.
Try-except import it in the ```_get_test_finders``` method and that should be
it. The find methods will be collected and executed before the default find
methods.
```python
try:
    from test_finders import new_test_finder
    test_finders_list.add(new_test_finder.NewTestFinder)
except ImportError:
    pass
```
