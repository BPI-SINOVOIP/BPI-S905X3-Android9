# PythonVirtualenvPreparerTest

This directory tests the functionality of VtsPythonVirtualenvPreparer.


Two modules are included in this project:

* VtsSelfTestPythonVirtualenvPreparerTestPart1
* VtsSelfTestPythonVirtualenvPreparerTestPart2


VtsSelfTestPythonVirtualenvPreparerTestPart1 uses a module level
VtsPythonVirtualenvPreparer and VtsSelfTestPythonVirtualenvPreparerTestPart2
does not. The naming of `Part1` and `Part2` is to ensure Part1 runs before
Part2 to test whether a module level VtsPythonVirtualenvPreparer's tearDown
functions will affect plan level VtsPythonVirtualenvPreparer.

