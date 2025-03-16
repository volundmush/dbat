import pydantic
from mudforge.models import fields

class UserLogin(pydantic.BaseModel):
    username: fields.name_line
    password: pydantic.SecretStr