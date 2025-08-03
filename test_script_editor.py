#!/usr/bin/env python3
"""
Quick test to verify the ScriptEditorParser trigger functionality.
"""

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'dbat'))

from dbat.models.scripts import DgMobScript, MobTriggerType

def test_trigger_handling():
    """Test trigger bitmask handling."""
    # Create a test mob script
    script = DgMobScript(
        vn=1001,
        name="Test Script",
        trigger_type=MobTriggerType.GREET | MobTriggerType.SPEECH
    )
    
    print(f"Initial triggers: {script.get_trigger_names()}")
    print(f"Trigger value: {script.trigger_type.value}")
    
    # Test available triggers
    available = script.available_trigger_types()
    print(f"Available triggers: {available[:5]}...")  # Just show first 5
    
    # Test setting new triggers
    new_trigger_value = MobTriggerType.COMMAND | MobTriggerType.DEATH | MobTriggerType.FIGHT
    script.trigger_type = new_trigger_value
    print(f"New triggers: {script.get_trigger_names()}")
    print(f"New trigger value: {script.trigger_type.value}")
    
    print("Test passed!")

if __name__ == "__main__":
    test_trigger_handling()
