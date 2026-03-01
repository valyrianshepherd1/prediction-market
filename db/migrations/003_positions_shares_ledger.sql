-- 003_positions_shares_ledger.sql
CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE positions
(
    user_id uuid NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    outcome_id uuid NOT NULL REFERENCES outcomes(id) ON DELETE CASCADE,

    shares_available bigint NOT NULL DEFAULT 0 CHECK (shares_available >= 0),
    shares_reserved  bigint NOT NULL DEFAULT 0 CHECK (shares_reserved  >= 0),

    updated_at timestamptz NOT NULL DEFAULT now(),
    created_at timestamptz NOT NULL DEFAULT now(),

    PRIMARY KEY (user_id, outcome_id)
);

CREATE INDEX idx_positions_outcome_id
    ON positions (outcome_id);

CREATE TABLE shares_ledger
(
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id uuid NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    outcome_id uuid NOT NULL REFERENCES outcomes(id) ON DELETE CASCADE,

    kind text NOT NULL CHECK (kind IN (
                                       'MINT',          -- например, при покупке/settlement
                                       'BURN',          -- например, при погашении/settlement
                                       'RESERVE',
                                       'RELEASE',
                                       'TRADE_DEBIT',
                                       'TRADE_CREDIT',
                                       'SETTLEMENT'
        )),

    delta_available bigint NOT NULL,
    delta_reserved  bigint NOT NULL,

    ref_type text NOT NULL,
    ref_id uuid NOT NULL,

    created_at timestamptz NOT NULL DEFAULT now(),

    CHECK (delta_available <> 0 OR delta_reserved <> 0)
);

CREATE INDEX idx_shares_ledger_user_created_at
    ON shares_ledger (user_id, created_at DESC);

CREATE INDEX idx_shares_ledger_outcome_created_at
    ON shares_ledger (outcome_id, created_at DESC);

CREATE INDEX idx_shares_ledger_ref
    ON shares_ledger (ref_type, ref_id);