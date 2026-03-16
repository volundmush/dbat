upgrade = """
CREATE SCHEMA dbat;

CREATE TABLE dbat.incoming_events (
    pc_id UUID REFERENCES pcs(id),
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    event_type LTREE NOT NULL,
    data JSONB NOT NULL DEFAULT '{}'::jsonb
);

CREATE TABLE dbat.api_requests (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID REFERENCES public.users(id) ON DELETE CASCADE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    request_type LTREE NOT NULL,
    data JSONB NOT NULL DEFAULT '{}'::jsonb,
    response_status INT DEFAULT -1,
    response_data JSONB NOT NULL DEFAULT '{}'::jsonb
);

CREATE FUNCTION dbat.notify_api_request_responded_trigger() RETURNS trigger AS $$
BEGIN
    IF NEW.response_status <> -1 THEN
        PERFORM pg_notify('dbat_api_request_responded', NEW.id::text);
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER api_request_responded_notify
AFTER UPDATE ON dbat.api_requests
FOR EACH ROW
WHEN (NEW.response_status <> -1)
EXECUTE FUNCTION dbat.notify_api_request_responded_trigger();

CREATE TABLE dbat.zones (
    id SERIAL PRIMARY KEY,
    name TEXT NOT NULL,
    data JSONB NOT NULL DEFAULT '{}'::jsonb
);

CREATE TABLE dbat.help (
    id SERIAL PRIMARY KEY,
    keywords TEXT NOT NULL,
    entry TEXT NOT NULL,
    min_level INT NOT NULL DEFAULT 0
);

CREATE TABLE dbat.dgproto (
    id SERIAL PRIMARY KEY,
    name TEXT NOT NULL,
    attach_type TEXT NOT NULL,
    trigger_type INT NOT NULL DEFAULT 0,
    narg INT NOT NULL DEFAULT 0,
    arglist TEXT NOT NULL DEFAULT '',
    body TEXT NOT NULL DEFAULT ''
);

CREATE TABLE dbat.shops (
    id SERIAL PRIMARY KEY,
    data JSONB NOT NULL DEFAULT '{}'::jsonb
);

CREATE TABLE dbat.guilds (
    id SERIAL PRIMARY KEY,
    data JSONB NOT NULL DEFAULT '{}'::jsonb
);

CREATE TABLE dbat.globals (
    key TEXT PRIMARY KEY,
    data JSONB NOT NULL DEFAULT '{}'::jsonb
);

CREATE TABLE dbat.rooms (
    id SERIAL PRIMARY KEY,
    data JSONB NOT NULL DEFAULT '{}'::jsonb,
    exits JSONB NOT NULL DEFAULT '{}'::jsonb
);

CREATE TABLE dbat.areas (
    id SERIAL PRIMARY KEY,
    data JSONB NOT NULL DEFAULT '{}'::jsonb
);

CREATE TABLE dbat.structures (
    id SERIAL PRIMARY KEY,
    data JSONB NOT NULL DEFAULT '{}'::jsonb
);

CREATE TABLE dbat.oproto (
    id SERIAL PRIMARY KEY,
    data JSONB NOT NULL DEFAULT '{}'::jsonb
);

CREATE TABLE dbat.nproto (
    id SERIAL PRIMARY KEY,
    data JSONB NOT NULL DEFAULT '{}'::jsonb
);

CREATE TABLE dbat.assemblies (
    id SERIAL PRIMARY KEY,
    data JSONB NOT NULL DEFAULT '{}'::jsonb
);

"""
