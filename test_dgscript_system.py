#!/usr/bin/env python3
"""
Test script to demonstrate the new DgScript enum and validation system.
"""

import sys
import os

# Add the dbat module to the path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from dbat.models.names import UnitType, MobTriggerType, ObjectTriggerType, RoomTriggerType
from dbat.models.dgscript_utils import (
    parse_unit_type, parse_trigger_types, format_trigger_types,
    get_trigger_types_for_unit, validate_trigger_combination, get_trigger_help
)
from dbat.models.game import TrigProtoData


def test_enum_system():
    """Test the DgScript enum system."""
    print("=== Testing DgScript Enum System ===\n")
    
    # Test unit type parsing
    print("1. Unit Type Parsing:")
    for unit_str in ['mob', 'obj', 'room', 'character', 'object', 'invalid']:
        unit_type = parse_unit_type(unit_str)
        print(f"  '{unit_str}' -> {unit_type}")
    print()
    
    # Test trigger type parsing for different units
    print("2. Trigger Type Parsing:")
    
    # Mob triggers
    mob_type = UnitType.character
    mob_triggers = "speech greet random"
    mob_value = parse_trigger_types(mob_triggers, mob_type)
    formatted = format_trigger_types(mob_value, mob_type)
    print(f"  Mob '{mob_triggers}' -> {mob_value} -> '{formatted}'")
    
    # Object triggers
    obj_type = UnitType.object
    obj_triggers = "get drop wear"
    obj_value = parse_trigger_types(obj_triggers, obj_type)
    formatted = format_trigger_types(obj_value, obj_type)
    print(f"  Obj '{obj_triggers}' -> {obj_value} -> '{formatted}'")
    
    # Room triggers
    room_type = UnitType.room
    room_triggers = "enter speech command"
    room_value = parse_trigger_types(room_triggers, room_type)
    formatted = format_trigger_types(room_value, room_type)
    print(f"  Room '{room_triggers}' -> {room_value} -> '{formatted}'")
    print()
    
    # Test validation
    print("3. Validation Testing:")
    for unit_type, trigger_value in [
        (UnitType.character, MobTriggerType.SPEECH | MobTriggerType.GREET),
        (UnitType.object, ObjectTriggerType.GET | ObjectTriggerType.DROP),
        (UnitType.room, RoomTriggerType.ENTER | RoomTriggerType.SPEECH),
        (UnitType.character, 999999),  # Invalid trigger
    ]:
        is_valid, error = validate_trigger_combination(unit_type, trigger_value)
        status = "✓ Valid" if is_valid else f"✗ Invalid: {error}"
        print(f"  {unit_type.name} with triggers {trigger_value}: {status}")
    print()


def test_trigprotodata_model():
    """Test the TrigProtoData model with validation."""
    print("=== Testing TrigProtoData Model ===\n")
    
    # Create a valid mob script
    print("1. Creating valid mob script:")
    try:
        script = TrigProtoData(
            vn=1001,
            name="Test Mob Script",
            attach_type=UnitType.character,
            trigger_type=MobTriggerType.SPEECH | MobTriggerType.GREET,
            arglist="hello hi greetings"
        )
        print(f"  ✓ Created: {script.name}")
        print(f"    Type: {script.attach_type.name}")
        print(f"    Triggers: {script.get_trigger_type_names()}")
        print(f"    Args: {script.arglist}")
    except Exception as e:
        print(f"  ✗ Failed: {e}")
    print()
    
    # Test the helper methods
    print("2. Testing helper methods:")
    script.set_trigger_type_from_names(['speech', 'random', 'command'])
    print(f"  Set triggers from names: {script.get_trigger_type_names()}")
    print(f"  Trigger value: {script.trigger_type}")
    print()
    
    # Test invalid combination
    print("3. Testing invalid combination:")
    try:
        # Try to create a mob script with an undefined trigger bit (high bit)
        invalid_script = TrigProtoData(
            vn=1002,
            name="Invalid Script",
            attach_type=UnitType.character,
            trigger_type=1 << 25,  # Bit 25 is not defined for any trigger type
        )
        print(f"  ✗ Should have failed but didn't: {invalid_script}")
    except Exception as e:
        print(f"  ✓ Correctly rejected: {e}")
    
    # Test another invalid combination - object trigger with undefined bit
    try:
        invalid_script2 = TrigProtoData(
            vn=1003,
            name="Another Invalid Script", 
            attach_type=UnitType.object,
            trigger_type=(1 << 10) | (1 << 14),  # Bits 10 and 14 are not valid for objects
        )
        print(f"  ✗ Should have failed but didn't: {invalid_script2}")
    except Exception as e:
        print(f"  ✓ Correctly rejected: {e}")
    print()


def demo_admin_commands():
    """Demonstrate how the admin commands would work."""
    print("=== Admin Command Demo ===\n")
    
    # Simulate admin command sequence
    print("Admin session simulation:")
    print("> kind mob")
    unit_type = parse_unit_type("mob")
    print(f"  Script type set to: {unit_type.name}")
    
    print("> triggers speech greet random")
    trigger_value = parse_trigger_types("speech greet random", unit_type)
    is_valid, error = validate_trigger_combination(unit_type, trigger_value)
    if is_valid:
        triggers_formatted = format_trigger_types(trigger_value, unit_type)
        print(f"  Triggers set to: {triggers_formatted}")
    else:
        print(f"  Error: {error}")
    
    print("> list")
    print("  Available mob triggers:")
    triggers = get_trigger_types_for_unit(unit_type)
    for name, value in sorted(triggers.items(), key=lambda x: x[1]):
        bit_pos = (value - 1).bit_length() if value > 0 else 0
        print(f"    {name:<12} (1 << {bit_pos:2d}) = {value}")
    print()


if __name__ == "__main__":
    try:
        test_enum_system()
        test_trigprotodata_model()
        demo_admin_commands()
        print("All tests completed successfully! 🎉")
    except Exception as e:
        print(f"Test failed: {e}")
        import traceback
        traceback.print_exc()
