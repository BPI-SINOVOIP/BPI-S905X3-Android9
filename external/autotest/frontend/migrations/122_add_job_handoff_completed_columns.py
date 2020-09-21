UP_SQL = """
ALTER TABLE afe_job_handoffs ADD COLUMN (
  created datetime NOT NULL,
  completed tinyint(1) NOT NULL
);
UPDATE afe_job_handoffs SET completed = 1;
"""

DOWN_SQL = """
ALTER TABLE afe_job_handoffs DROP COLUMN created, DROP COLUMN completed;
"""
