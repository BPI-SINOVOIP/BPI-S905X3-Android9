UP_SQL = """
ALTER TABLE afe_stable_versions
  ADD COLUMN archive_url TEXT;
"""

DOWN_SQL = """
ALTER TABLE afe_stable_versions
  DROP COLUMN archive_url;
"""
