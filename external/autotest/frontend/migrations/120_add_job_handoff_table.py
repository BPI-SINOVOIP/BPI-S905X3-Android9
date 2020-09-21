UP_SQL = """
CREATE TABLE afe_job_handoffs (
  job_id int(11) NOT NULL,
  PRIMARY KEY (job_id),
  CONSTRAINT job_fk FOREIGN KEY (job_id)
    REFERENCES afe_jobs(id)
    ON DELETE CASCADE
) ENGINE=INNODB;
"""

DOWN_SQL = """
DROP TABLE afe_job_handoffs;
"""
