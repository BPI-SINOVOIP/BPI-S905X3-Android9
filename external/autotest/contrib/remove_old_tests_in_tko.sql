-- -----------------------------------------------------------------------------
-- Procedure to delete records in TKO database older than 180 days.
-- -----------------------------------------------------------------------------
DELIMITER $$
CREATE PROCEDURE remove_old_tests_sp()
BEGIN
  -- Delete tko_tests older than 180 days in batches of 5k. Wait for 5 seconds to
  -- avoid database lock for too long.
  SET @cutoff_date = DATE_SUB(CURDATE(),INTERVAL 180 DAY);

  WHILE EXISTS (SELECT test_idx FROM tko_tests WHERE started_time < @cutoff_date LIMIT 1) DO
    BEGIN
      SELECT concat("Deleting 5k records in tko_tests older than ", @cutoff_date);
      DELETE FROM tko_tests WHERE started_time < @cutoff_date LIMIT 5000;
      SELECT SLEEP(5);
    END;
  END WHILE;

  -- Some tests may have started_time being NULL, but with finished_time set.
  -- Deletion for these records is done in a different while loop to make the
  -- query go faster as finished_time is not indexed.
  WHILE EXISTS (SELECT test_idx FROM tko_tests where started_time IS NULL and finished_time < @cutoff_date LIMIT 1) DO
    BEGIN
      SELECT concat("Deleting 5k records in tko_tests with finished time older than ", @cutoff_date);
      DELETE FROM tko_tests WHERE started_time IS NULL AND finished_time < @cutoff_date LIMIT 5000;
      SELECT SLEEP(5);
    END;
  END WHILE;

  -- After tko_tests is cleaned up, we can start cleaning up tko_jobs, due to FK
  -- constrain. Move the cutoff time to 5 days older as some tko_jobs records
  -- may have later started_time comparing to tko_tests. Deleting these records
  -- may lead to FK constrain violation.
  SET @cutoff_date = DATE_SUB(@cutoff_date,INTERVAL 5 DAY);
  WHILE EXISTS (SELECT job_idx FROM tko_jobs WHERE started_time < @cutoff_date LIMIT 1) DO
    BEGIN
      SELECT concat("Deleting 5k records in tko_jobs older than ", @cutoff_date);
      DELETE FROM tko_jobs WHERE started_tim < @cutoff_date LIMIT 5000;
      SELECT SLEEP(5);
    END;
  END WHILE;
END;$$
DELIMITER ;
