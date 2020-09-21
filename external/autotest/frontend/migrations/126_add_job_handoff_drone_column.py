UP_SQL = """
ALTER TABLE afe_job_handoffs ADD COLUMN (
  drone varchar(128) NULL
);
"""

DOWN_SQL = """
ALTER TABLE afe_job_handoffs DROP COLUMN drone;
"""
