BEGIN;

ALTER TABLE markets
    DROP CONSTRAINT IF EXISTS markets_status_check;

ALTER TABLE markets
    ADD CONSTRAINT markets_status_check
        CHECK (status IN ('OPEN','CLOSED','RESOLVED','ARCHIVED'));

COMMIT;