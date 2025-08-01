import typing
from dbat.models.scripts import TrigProtoData
from dbat_ext import script_db

def list_scripts() -> typing.Generator[TrigProtoData, None, None]:
    for script in script_db.list_scripts():
        yield TrigProtoData(**script)

def get_script(script_id: int) -> TrigProtoData:
    return TrigProtoData(**script_db.get_script(script_id))
