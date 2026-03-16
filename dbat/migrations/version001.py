upgrade = """
CREATE SCHEMA dbat;

CREATE TABLE dbat.accounts (
    id UUID PRIMARY KEY REFERENCES public.users(id) ON DELETE CASCADE,
    dbat_id INT NOT NULL UNIQUE
);

CREATE TABLE dbat.pcs (
    id UUID PRIMARY KEY REFERENCES public.pcs(id) ON DELETE CASCADE,
    dbat_id BIGINT NOT NULL UNIQUE
);

CREATE TABLE dbat.incoming_events (
    pc_id UUID REFERENCES pcs(id),
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    event_type LTREE NOT NULL,
    data JSONB NOT NULL DEFAULT '{}'::jsonb
);

CREATE TABLE dbat.api_requests (
    id UUID PRIMARY KEY,
    user_id UUID REFERENCES public.users(id) ON DELETE CASCADE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    request_type LTREE NOT NULL,
    data JSONB NOT NULL DEFAULT '{}'::jsonb,
    response_status INT DEFAULT -1,
    response_data JSONB NOT NULL DEFAULT '{}'::jsonb
);

"""
