-- 007_market_resolution_settlement.sql
CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE market_resolutions
(
    market_id uuid PRIMARY KEY REFERENCES markets(id) ON DELETE CASCADE,

    -- ensures outcome belongs to the same market via composite FK
    winning_outcome_id uuid NOT NULL,
    resolved_by_user_id uuid NOT NULL REFERENCES users(id) ON DELETE RESTRICT,

    resolved_at timestamptz NOT NULL DEFAULT now(),

    FOREIGN KEY (winning_outcome_id, market_id)
        REFERENCES outcomes(id, market_id)
);

CREATE INDEX idx_market_resolutions_resolved_at
    ON market_resolutions (resolved_at DESC);

CREATE TABLE settlements
(
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),

    market_id uuid NOT NULL REFERENCES markets(id) ON DELETE CASCADE,
    user_id uuid NOT NULL REFERENCES users(id) ON DELETE CASCADE,

    winning_outcome_id uuid NOT NULL,
    payout_micros bigint NOT NULL CHECK (payout_micros >= 0),

    cash_ledger_id uuid NULL REFERENCES cash_ledger(id) ON DELETE SET NULL,

    created_at timestamptz NOT NULL DEFAULT now(),

    UNIQUE (market_id, user_id),

    FOREIGN KEY (winning_outcome_id, market_id)
        REFERENCES outcomes(id, market_id)
);

CREATE INDEX idx_settlements_user_created_at
    ON settlements (user_id, created_at DESC);

CREATE INDEX idx_settlements_market_created_at
    ON settlements (market_id, created_at DESC);