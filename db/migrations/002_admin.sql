INSERT INTO users (email, username, password_hash, role)
VALUES
(
    'valyrianshepherd@gmail.com',
    'admin_sergey',
    'test_hash',
    'admin'
)
ON CONFLICT DO NOTHING;

