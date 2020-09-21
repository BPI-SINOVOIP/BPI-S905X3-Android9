import glob
import sys
import unittest

sys.path.insert(0, 'server')
sys.path.insert(0, 'clients')
sys.path.insert(0, 'common')

test_file_strings = glob.glob('*/*_test.py')
module_strings = [str[0:len(str) - 3] for str in test_file_strings]
for i in range(len(module_strings)):
  module_strings[i] = module_strings[i].split('/')[-1]
suites = [unittest.defaultTestLoader.loadTestsFromName(str)
          for str in module_strings]
testSuite = unittest.TestSuite(suites)
text_runner = unittest.TextTestRunner().run(testSuite)
