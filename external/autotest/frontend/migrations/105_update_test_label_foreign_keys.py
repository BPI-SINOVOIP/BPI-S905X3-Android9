ADD_FOREIGN_KEYS = """
ALTER TABLE tko_test_labels_tests DROP FOREIGN KEY tests_labels_tests_ibfk_1;
ALTER TABLE tko_test_labels_tests ADD CONSTRAINT tests_labels_tests_ibfk_1
    FOREIGN KEY (testlabel_id) REFERENCES tko_test_labels (id)
    ON DELETE CASCADE;

ALTER TABLE tko_test_labels_tests DROP FOREIGN KEY tests_labels_tests_ibfk_2;
ALTER TABLE tko_test_labels_tests ADD CONSTRAINT tests_labels_tests_ibfk_2
    FOREIGN KEY (test_id) REFERENCES tko_tests (test_idx) ON DELETE CASCADE;
"""

DROP_FOREIGN_KEYS = """
ALTER TABLE tko_test_labels_tests DROP FOREIGN KEY tests_labels_tests_ibfk_1;
ALTER TABLE tko_test_labels_tests ADD CONSTRAINT tests_labels_tests_ibfk_1
    FOREIGN KEY (testlabel_id) REFERENCES tko_test_labels (id);

ALTER TABLE tko_test_labels_tests DROP FOREIGN KEY tests_labels_tests_ibfk_2;
ALTER TABLE tko_test_labels_tests ADD CONSTRAINT tests_labels_tests_ibfk_2
    FOREIGN KEY (test_id) REFERENCES tko_tests (test_idx);
"""

def migrate_up(mgr):
    mgr.execute_script(ADD_FOREIGN_KEYS)

def migrate_down(mgr):
    mgr.execute_script(DROP_FOREIGN_KEYS)
