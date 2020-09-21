UP_SQL = """
CREATE INDEX queued_time ON tko_jobs (queued_time);
CREATE INDEX finished_time ON tko_jobs (finished_time);
"""

DOWN_SQL = """
DROP INDEX queued_time ON tko_jobs;
DROP INDEX finished_time ON tko_jobs;
"""
