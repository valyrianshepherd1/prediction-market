-- First migration with the most basic tables
-- register/login/session
-- market/outcome

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

CREATE TABLE markets
(
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    question text NOT NULL,
    status text NOT NULL CHECK (status IN ('OPEN','RESOLVED','CLOSED')),
    resolved_outcome_id uuid NULL,
    created_at timestamptz NOT NULL DEFAULT now()
);

CREATE TABLE outcomes
(
    id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
    market_id uuid NOT NULL REFERENCES markets(id) ON DELETE CASCADE,
    title text NOT NULL,
    outcome_index int NOT NULL,
    UNIQUE (market_id, outcome_index)
);

CREATE INDEX idx_sessions_user_id ON sessions(user_id);
CREATE INDEX idx_outcomes_market_id ON outcomes(market_id);
CREATE INDEX idx_markets_status ON markets(status);