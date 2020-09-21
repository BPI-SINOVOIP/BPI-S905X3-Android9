UP_SQL = """
CREATE TABLE `afe_static_host_attributes` (
    `id` integer AUTO_INCREMENT NOT NULL PRIMARY KEY,
    `host_id` integer NOT NULL,
    `attribute` varchar(90) NOT NULL,
    `value` varchar(300) NOT NULL,
    FOREIGN KEY (host_id)
    REFERENCES afe_hosts (id) ON DELETE CASCADE,
    KEY (attribute)
) ENGINE=InnoDB;
"""

DOWN_SQL = """
DROP TABLE IF EXISTS afe_static_host_attributes;
"""
