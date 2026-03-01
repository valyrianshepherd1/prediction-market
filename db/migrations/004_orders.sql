-- 004_orders.sql
CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE orders
(
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),

    user_id uuid NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    outcome_id uuid NOT NULL REFERENCES outcomes(id) ON DELETE CASCADE,

    side text NOT NULL CHECK (side IN ('BUY','SELL')),

    -- price in basis points: 0..10000
    price_bp int NOT NULL CHECK (price_bp BETWEEN 0 AND 10000),

    -- quantity in micros (or smallest unit), must be > 0
    qty_total_micros bigint NOT NULL CHECK (qty_total_micros > 0),
    qty_remaining_micros bigint NOT NULL CHECK (qty_remaining_micros >= 0),

    status text NOT NULL CHECK (status IN ('OPEN','PARTIALLY_FILLED','FILLED','CANCELED')),

    created_at timestamptz NOT NULL DEFAULT now(),
    updated_at timestamptz NOT NULL DEFAULT now(),

    CHECK (qty_remaining_micros <= qty_total_micros)
);

-- User order history
CREATE INDEX idx_orders_user_created_at
    ON orders (user_id, created_at DESC);

-- Outcome order history
CREATE INDEX idx_orders_outcome_created_at
    ON orders (outcome_id, created_at DESC);

-- Order book indexes (price-time priority)
CREATE INDEX idx_orders_book_buy_open
    ON orders (outcome_id, price_bp DESC, created_at, id)
    WHERE side = 'BUY' AND status IN ('OPEN','PARTIALLY_FILLED');

CREATE INDEX idx_orders_book_sell_open
    ON orders (outcome_id, price_bp ASC, created_at, id)
    WHERE side = 'SELL' AND status IN ('OPEN','PARTIALLY_FILLED');