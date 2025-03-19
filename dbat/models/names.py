from enum import Enum
from dbat_ext import get_names

# Map the Python enum name to the corresponding category string that your C++ get_names() expects.
__categories = {
    "Race": "races",
    "Sensei": "senseis",
    "Form": "forms",
    "Skill": "skills",
    "RoomFlag": "room_flags",
    "SectorType": "sector_types",
    "Size": "sizes",
    "PlayerFlag": "player_flags",
    "MobFlag": "mob_flags",
    "PrefFlag": "pref_flags",
    "AffectFlag": "affect_flags",
    "ItemType": "item_types",
    "WearFlag": "wear_flags",
    "ItemFlag": "item_flags",
    "AdminFlag": "admin_flags",
    "Direction": "directions",
    "Attribute": "attributes",
    "AttributeTrain": "attribute_trains",
    "Appearance": "appearances",
    "Align": "aligns",
    "Money": "moneys",
    "Vital": "vitals",
    "Num": "nums",
    "Stat": "stats",
    "Dim": "dims",
    "ComStat": "com_stats",
    "ShopFlag": "shop_flags",
}

for enum_name, cat in __categories.items():
    names = get_names(cat)  # Returns a list[str] from the C++ side.
    # Create an enum where each member's name and value is the string from C++.
    globals()[enum_name] = Enum(enum_name, {name: name for name in names})