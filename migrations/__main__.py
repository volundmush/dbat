import sqlite3
from pathlib  import Path

MIGRATE_TABLE = """
CREATE TABLE IF NOT EXISTS migrations (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
"""

def perform_migrations(db, path: Path):
    """
    Search in <cwd>/schemas for all .sql files, execute them in lexicographic order, each in their own transaction.
    """
    sql_files = sorted(path.glob("*.sql"))
    for sql_file in sql_files:
        with open(sql_file, "r") as f:
            sql_script = f.read()
        db.executescript(sql_script)
        db.execute("INSERT INTO migrations (name) VALUES (?)", (sql_file.name,))
        print(f"Executed migration {sql_file.name}")

def main():
    path = Path.cwd() / "data"

    sql_file = path / "database.sqlite3"
    sql = sqlite3.connect(sql_file)
    schema_path = Path.cwd() / "migrations"
    with sql:
        sql.execute(MIGRATE_TABLE)
        perform_migrations(sql, schema_path)

main()