UP_SQL = """
CREATE TABLE afe_replaced_labels (
  id int(11) NOT NULL auto_increment,
  label_id int(11) default NULL,
  PRIMARY KEY (id),
  UNIQUE KEY label_id (label_id),
  FOREIGN KEY (label_id)
    REFERENCES afe_labels (id) ON DELETE CASCADE
) ENGINE=InnoDB;
"""

DOWN_SQL = """
DROP TABLE IF EXISTS afe_replaced_labels;
"""
