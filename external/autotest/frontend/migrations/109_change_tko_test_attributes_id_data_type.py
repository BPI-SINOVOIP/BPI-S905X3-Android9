UP_SQL = """
ALTER TABLE tko_test_attributes MODIFY id bigint(20) AUTO_INCREMENT;
"""

DOWN_SQL = """
ALTER TABLE tko_test_attributes MODIFY id int(11) AUTO_INCREMENT;
"""
