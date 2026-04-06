import typing
import uuid
from pydantic import BaseModel, Field, ConfigDict

class Shop(BaseModel):
    model_config = ConfigDict(validate_assignment=True)
