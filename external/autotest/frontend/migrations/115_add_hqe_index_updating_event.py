UP_SQL = """
CREATE EVENT afe_add_entry_to_hqe_start_times
ON SCHEDULE EVERY 10 MINUTE DO
INSERT INTO afe_host_queue_entry_start_times (insert_time, highest_hqe_id)
SELECT NOW(), MAX(afe_host_queue_entries.id)
FROM afe_host_queue_entries;
"""

DOWN_SQL = """
DROP EVENT afe_add_entry_to_hqe_start_times;
"""
