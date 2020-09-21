
'in', 'input':
	required, mergeable, values: 1,
	'<list of files>',
	"list of meta/coverage data files";

'r', 'report':
	required, mergeable, values: 1,
	'<list of {txt|html|lcov|xml}>',
	"coverage report type list";

'sp', 'sourcepath':
	optional, mergeable, values: 1,
	'<list of source directories>',
	"Java source path for generating reports";

'p', 'props', 'properties':
	optional, values: 1,
	'<properties file>',
	"properties override file";

'D':
	optional, mergeable, detailedonly, pattern, values: 1,
	'<value>',
	"generic property override";

'exit':
	optional, detailedonly, values: 0,
	"use System.exit() on termination";

'verbose':
	optional, detailedonly, values: 0,
	excludes {'silent', 'quiet', 'debug'},
	"verbose output operation";

'quiet':
	optional, detailedonly, values: 0,
	excludes {'silent', 'verbose', 'debug'},
	"quiet operation (ignore all but warnings and severe errors)";

'silent':
	optional, detailedonly, values: 0,
	excludes {'quiet', 'verbose', 'debug'},
	"extra-quiet operation (ignore all but severe errors)";

'debug', 'loglevel': 
	optional, detailedonly, values: ?,
	'[<debug trace level>]',
	excludes {'verbose', 'quiet', 'silent'},
	"debug tracing level";

'debugcls':
	optional, detailedonly, values: 1,
	'<debug trace class mask>',
	"class mask for debug tracing";


