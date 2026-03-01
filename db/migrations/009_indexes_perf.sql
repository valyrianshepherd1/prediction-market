-- 009_indexes_perf.sql

-- Orders: user view filtered by status
CREATE INDEX IF NOT EXISTS idx_orders_user_status_created_at
    ON orders (user_id, status, created_at DESC);

-- Trades: global recent trades
CREATE INDEX IF NOT EXISTS idx_trades_created_at
    ON trades (created_at DESC);

-- Markets: fast admin lists by creation time
CREATE INDEX IF NOT EXISTS idx_markets_created_at
    ON markets (created_at DESC);