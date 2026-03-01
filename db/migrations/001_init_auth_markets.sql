-- 002_wallets_cash_ledger.sql
CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE wallets
(
    user_id uuid PRIMARY KEY REFERENCES users(id) ON DELETE CASCADE,
    available bigint NOT NULL DEFAULT 0 CHECK (available >= 0),
    reserved  bigint NOT NULL DEFAULT 0 CHECK (reserved  >= 0),
    updated_at timestamptz NOT NULL DEFAULT now(),
    created_at timestamptz NOT NULL DEFAULT now()
);

CREATE TABLE cash_ledger
(
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id uuid NOT NULL REFERENCES users(id) ON DELETE CASCADE,

    kind text NOT NULL CHECK (kind IN (
                                       'DEPOSIT',
                                       'RESERVE',
                                       'RELEASE',
                                       'TRADE_DEBIT',
                                       'TRADE_CREDIT',
                                       'FEE',
                                       'SETTLEMENT'
        )),

    -- delta model: one entry can represent move available -> reserved
    delta_available bigint NOT NULL,
    delta_reserved  bigint NOT NULL,

    ref_type text NOT NULL,
    ref_id uuid NOT NULL,

    created_at timestamptz NOT NULL DEFAULT now(),

    CHECK (delta_available <> 0 OR delta_reserved <> 0)
);

CREATE INDEX idx_cash_ledger_user_created_at
    ON cash_ledger (user_id, created_at DESC);

CREATE INDEX idx_cash_ledger_ref
    ON cash_ledger (ref_type, ref_id);