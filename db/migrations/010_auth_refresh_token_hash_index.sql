-- 010_auth_refresh_token_hash_index.sql

CREATE INDEX IF NOT EXISTS idx_sessions_refresh_token_hash
    ON sessions(refresh_token_hash);
