-- 005_trades.sql
CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE trades
(
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),

    outcome_id uuid NOT NULL REFERENCES outcomes(id) ON DELETE CASCADE,

    maker_user_id uuid NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    taker_user_id uuid NOT NULL REFERENCES users(id) ON DELETE CASCADE,

    maker_order_id uuid NOT NULL REFERENCES orders(id) ON DELETE RESTRICT,
    taker_order_id uuid NOT NULL REFERENCES orders(id) ON DELETE RESTRICT,

    price_bp int NOT NULL CHECK (price_bp BETWEEN 0 AND 10000),
    qty_micros bigint NOT NULL CHECK (qty_micros > 0),

    created_at timestamptz NOT NULL DEFAULT now()
);

CREATE INDEX idx_trades_outcome_created_at
    ON trades (outcome_id, created_at DESC);

CREATE INDEX idx_trades_maker_user_created_at
    ON trades (maker_user_id, created_at DESC);

CREATE INDEX idx_trades_taker_user_created_at
    ON trades (taker_user_id, created_at DESC);

CREATE INDEX idx_trades_maker_order_id
    ON trades (maker_order_id);

CREATE INDEX idx_trades_taker_order_id
    ON trades (taker_order_id);