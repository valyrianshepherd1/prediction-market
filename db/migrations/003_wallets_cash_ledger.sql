CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE wallets
(
    user_id uuid PRIMARY KEY REFERENCES users(id) ON DELETE CASCADE,
    available BIGINT NOT NULL CHECK(available >= 0) DEFAULT 0,
    reserved BIGINT NOT NULL CHECK(reserved >= 0) DEFAULT 0,
    updated_at timestamptz NOT NULL DEFAULT now(),
    created_at timestamptz NOT NULL DEFAULT now()
);

CREATE TABLE cash_ledger
(
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    kind text NOT NULL CHECK(kind in ('DEPOSIT', 'RESERVE', 'RELEASE', 'TRADE_DEBIT', 'TRADE_CREDIT', 'FEE', 'SETTLEMENT')),
    delta_available BIGINT NOT NULL,
    delta_reserved BIGINT NOT NULL,
    ref_type text NOT NULL,
    ref_id uuid NOT NULL,
    created_at timestamptz NOT NULL DEFAULT now()
);

CREATE INDEX idx_cash_ledger_user_created_at
    ON cash_ledger (user_id, created_at DESC);

-- cash_ledger: lookup by reference (e.g., order/trade id)
CREATE INDEX idx_cash_ledger_ref
    ON cash_ledger (ref_type, ref_id);

