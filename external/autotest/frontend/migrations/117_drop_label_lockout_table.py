UP_SQL = """
DROP TABLE afe_label_lockout_times;
"""

DOWN_SQL = """
CREATE TABLE afe_label_lockout_times (
    id INT NOT NULL AUTO_INCREMENT,
    label_name VARCHAR(750) NOT NULL,
    lockout_end_time TIMESTAMP not null,
    PRIMARY KEY (id),
    INDEX afe_lockout_times_index (lockout_end_time)
);
"""
