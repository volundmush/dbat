from pydantic import BaseModel, Field, EmailStr
import uuid
import dbat

class Account(BaseModel):
    id: uuid.UUID = Field(..., description="The unique identifier for the account.")
    email: EmailStr = Field(..., description="The email address for the account.")
    username: str = Field("", description="The username for the account.")
    rpp: int = Field(0, description="The number of Roleplay Points the account has.")
    rpp_bank: int = Field(0, description="The number of banked Roleplay Points for the account.")
    admin_level: int = Field(0, description="The admin level for the account.")
    characters: list[uuid.UUID] = Field(default_factory=list, description="A list of character IDs associated with the account.")

    def save(self):
        dbat.DIRTY_ACCOUNTS.add(self.id)