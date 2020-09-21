UP_SQL = """
CREATE INDEX started_time_index ON tko_jobs (started_time);
"""

DOWN_SQL = """
DROP INDEX started_time_index ON tko_jobs;
"""
