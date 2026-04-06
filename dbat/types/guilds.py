import typing
import uuid
from pydantic import BaseModel, Field, ConfigDict

class Guild(BaseModel):
    model_config = ConfigDict(validate_assignment=True)