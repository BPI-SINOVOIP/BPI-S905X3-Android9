INDICES = (
    ('afe_host_queue_entries', 'active'),
    ('afe_host_queue_entries', 'complete'),
    ('afe_host_queue_entries', 'deleted'),
    ('afe_host_queue_entries', 'aborted'),
    ('afe_host_queue_entries', 'started_on'),
    ('afe_host_queue_entries', 'finished_on'),
    ('afe_host_queue_entries', 'job_id'),
)

def get_index_name(table, field):
    """Formats the index name from a |table| and |field|."""
    return table + '_' + field


def migrate_up(manager):
    """Creates the indices."""
    for table, field in INDICES:
        manager.execute('CREATE INDEX %s ON %s (%s)' %
                        (get_index_name(table, field), table, field))


def migrate_down(manager):
    """Removes the indices."""
    for table, field in INDICES:
        manager.execute('DROP INDEX %s ON %s' %
                        (get_index_name(table, field), table))
