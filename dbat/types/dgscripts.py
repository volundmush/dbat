class DgScript:
    
    def __init__(self):
        self.id: str = ""


class HasDgScripts:

    __slots__ = ("proto_script", "dgscripts", "script_order", "dg_variables")
    
    def __init__(self):
        self.proto_script: list[str] = list()
        self.dgscripts: dict[str, "DgScriptInstance"] = dict()
        self.script_order: list[str] | None = None
        self.dg_variables: dict[str, str] = dict()
    
    def evaluate_dgscript_member(self, script: "DgScriptInstance", field: str, arg: str) -> None | str:
        return None


class DgScriptInstance:
    
    def __init__(self, script: DgScript, owner: HasDgScripts):
        self.script = script
        self.owner = owner
    
    def __bool__(self):
        return bool(self.owner)