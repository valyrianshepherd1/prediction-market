-- 008_ws_outbox.sql
CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE outbox_events
(
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),

    event_type text NOT NULL,
    aggregate_type text NOT NULL,
    aggregate_id uuid NULL,

    payload jsonb NOT NULL,

    created_at timestamptz NOT NULL DEFAULT now(),
    available_at timestamptz NOT NULL DEFAULT now(),

    processed_at timestamptz NULL,
    attempts int NOT NULL DEFAULT 0 CHECK (attempts >= 0),
    last_error text NULL
);

CREATE INDEX idx_outbox_unprocessed
    ON outbox_events (available_at, created_at)
    WHERE processed_at IS NULL;

CREATE INDEX idx_outbox_aggregate
    ON outbox_events (aggregate_type, aggregate_id, created_at DESC);