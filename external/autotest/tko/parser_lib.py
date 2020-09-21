import autotest_lib.tko.parsers.version_0
import autotest_lib.tko.parsers.version_1


def parser(version):
    """Creates a parser instance for the requested version.
       @param version: the requested version.
    """
    return {
        0: autotest_lib.tko.parsers.version_0.parser,
        1: autotest_lib.tko.parsers.version_1.parser,
    }[version]()
