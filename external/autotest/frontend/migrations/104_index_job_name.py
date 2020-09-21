UP_SQL = """
CREATE INDEX name_index ON afe_jobs (name);
"""

DOWN_SQL = """
DROP INDEX name_index ON afe_jobs;
"""
