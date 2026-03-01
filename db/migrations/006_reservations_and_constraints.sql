-- 006_reservations_and_constraints.sql
CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE idempotency_keys
(
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),

    user_id uuid NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    idempotency_key text NOT NULL,

    -- what endpoint/action this key belongs to
    scope text NOT NULL,

    -- response snapshot
    response_status int NULL,
    response_body jsonb NULL,

    created_at timestamptz NOT NULL DEFAULT now(),

    UNIQUE (user_id, scope, idempotency_key)
);

CREATE INDEX idx_idempotency_user_created_at
    ON idempotency_keys (user_id, created_at DESC);