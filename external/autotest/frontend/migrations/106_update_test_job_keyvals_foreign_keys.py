ADD_FOREIGN_KEYS = """
ALTER TABLE tko_job_keyvals DROP FOREIGN KEY tko_job_keyvals_ibfk_1;
ALTER TABLE tko_job_keyvals ADD CONSTRAINT tko_job_keyvals_ibfk_1
    FOREIGN KEY (job_id) REFERENCES tko_jobs (job_idx) ON DELETE CASCADE;
"""

DROP_FOREIGN_KEYS = """
ALTER TABLE tko_job_keyvals DROP FOREIGN KEY tko_job_keyvals_ibfk_1;
ALTER TABLE tko_job_keyvals ADD CONSTRAINT tko_job_keyvals_ibfk_1
    FOREIGN KEY (job_id) REFERENCES tko_jobs (job_idx);
"""

def migrate_up(mgr):
    mgr.execute_script(ADD_FOREIGN_KEYS)

def migrate_down(mgr):
    mgr.execute_script(DROP_FOREIGN_KEYS)
