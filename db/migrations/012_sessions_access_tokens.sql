-- 011_sessions_access_tokens.sql

ALTER TABLE sessions
    ADD COLUMN IF NOT EXISTS access_token_hash text,
    ADD COLUMN IF NOT EXISTS access_expires_at timestamptz;

UPDATE sessions
SET access_token_hash = COALESCE(access_token_hash, encode(digest(gen_random_uuid()::text, 'sha256'), 'hex')),
    access_expires_at = COALESCE(access_expires_at, now())
WHERE access_token_hash IS NULL
   OR access_expires_at IS NULL;

ALTER TABLE sessions
    ALTER COLUMN access_token_hash SET NOT NULL,
    ALTER COLUMN access_expires_at SET NOT NULL;

CREATE UNIQUE INDEX IF NOT EXISTS idx_sessions_access_token_hash
    ON sessions(access_token_hash);

CREATE INDEX IF NOT EXISTS idx_sessions_access_expires_at
    ON sessions(access_expires_at);
