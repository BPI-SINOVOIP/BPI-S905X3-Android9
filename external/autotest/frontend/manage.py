#!/usr/bin/env python
import sys
import setup_django_environment

if __name__ == "__main__":
    from django.core.management import execute_from_command_line

    execute_from_command_line(sys.argv)
