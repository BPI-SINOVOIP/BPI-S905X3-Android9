UP_SQL = """
CREATE INDEX finished_time_idx ON tko_tests (finished_time);
"""

DOWN_SQL = """
DROP INDEX finished_time_idx ON tko_tests;
"""
