UP_SQL = """
CREATE TABLE afe_host_queue_entry_start_times (
    id INT NOT NULL AUTO_INCREMENT,
    insert_time TIMESTAMP NOT NULL,
    highest_hqe_id INT NOT NULL,
    PRIMARY KEY (id),
    INDEX afe_hqe_insert_times_index (insert_time)
);
"""

DOWN_SQL = """
DROP TABLE afe_host_queue_entry_start_times;
"""
