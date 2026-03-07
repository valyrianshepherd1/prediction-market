-- 001_init_auth_markets.sql
CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE users
(
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    email text NOT NULL UNIQUE,
    username text NOT NULL UNIQUE,
    password_hash text NOT NULL,
    role text NOT NULL CHECK (role IN ('user','admin')),
    created_at timestamptz NOT NULL DEFAULT now()
);

CREATE TABLE sessions
(
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id uuid NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    refresh_token_hash text NOT NULL,
    expires_at timestamptz NOT NULL,
    created_at timestamptz NOT NULL DEFAULT now()
);

CREATE INDEX idx_sessions_user_id ON sessions(user_id);
CREATE INDEX idx_sessions_expires_at ON sessions(expires_at);

CREATE TABLE markets
(
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    question text NOT NULL,
    status text NOT NULL CHECK (status IN ('OPEN','CLOSED','RESOLVED')),
    created_at timestamptz NOT NULL DEFAULT now(),
    closed_at timestamptz NULL
);

CREATE INDEX idx_markets_status_created_at ON markets(status, created_at DESC);

CREATE TABLE outcomes
(
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    market_id uuid NOT NULL REFERENCES markets(id) ON DELETE CASCADE,
    title text NOT NULL,
    outcome_index int NOT NULL CHECK (outcome_index >= 0),

    UNIQUE (market_id, outcome_index),
    -- важная строка для FK “outcome belongs to market” в 008:
    UNIQUE (id, market_id)
);

CREATE INDEX idx_outcomes_market_id ON outcomes(market_id);