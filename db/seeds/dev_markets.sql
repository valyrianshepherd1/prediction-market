-- Dev/demo data: markets + outcomes (idempotent)

-- Market 1
WITH new_market AS (
INSERT INTO markets (question, status)
SELECT 'test_question1', 'OPEN'
    WHERE NOT EXISTS (
    SELECT 1 FROM markets WHERE question = 'test_question'
  )
  RETURNING id
),
market AS (
SELECT id FROM new_market
UNION ALL
SELECT id FROM markets WHERE question = 'test_question1' LIMIT 1
    )
INSERT INTO outcomes (market_id, title, outcome_index)
SELECT id, 'YES', 0 FROM market
UNION ALL
SELECT id, 'NO',  1 FROM market
    ON CONFLICT DO NOTHING;

-- Market 2
WITH new_market AS (
INSERT INTO markets (question, status)
SELECT 'test_question2', 'OPEN'
    WHERE NOT EXISTS (
    SELECT 1 FROM markets WHERE question = 'test_question2'
  )
  RETURNING id
),
market AS (
SELECT id FROM new_market
UNION ALL
SELECT id FROM markets WHERE question = 'test_question2?' LIMIT 1
    )
INSERT INTO outcomes (market_id, title, outcome_index)
SELECT id, 'YES', 0 FROM market
UNION ALL
SELECT id, 'NO',  1 FROM market
    ON CONFLICT DO NOTHING;

-- Market 3
WITH new_market AS (
INSERT INTO markets (question, status)
SELECT 'test_question3', 'OPEN'
    WHERE NOT EXISTS (
    SELECT 1 FROM markets WHERE question = 'test_question3?'
  )
  RETURNING id
),
market AS (
SELECT id FROM new_market
UNION ALL
SELECT id FROM markets WHERE question = 'test_question3' LIMIT 1
    )
INSERT INTO outcomes (market_id, title, outcome_index)
SELECT id, 'YES', 0 FROM market
UNION ALL
SELECT id, 'NO',  1 FROM market
    ON CONFLICT DO NOTHING;