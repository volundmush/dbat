#!/usr/bin/env python3
"""
Debug script to examine trigger flag overlaps and fix validation.
"""

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from dbat.models.names import MobTriggerType, ObjectTriggerType, RoomTriggerType

def analyze_trigger_overlaps():
    """Analyze trigger flag overlaps between different types."""
    print("=== Trigger Flag Analysis ===\n")
    
    mob_flags = {flag.name: flag.value for flag in MobTriggerType}
    obj_flags = {flag.name: flag.value for flag in ObjectTriggerType}
    room_flags = {flag.name: flag.value for flag in RoomTriggerType}
    
    print("Mob triggers:")
    for name, value in sorted(mob_flags.items(), key=lambda x: x[1]):
        print(f"  {name:<12} = {value:>7}")
    
    print("\nObject triggers:")
    for name, value in sorted(obj_flags.items(), key=lambda x: x[1]):
        print(f"  {name:<12} = {value:>7}")
    
    print("\nRoom triggers:")
    for name, value in sorted(room_flags.items(), key=lambda x: x[1]):
        print(f"  {name:<12} = {value:>7}")
    
    print("\n=== Overlap Analysis ===")
    
    # Check for specific overlaps
    test_values = [32, 64]  # DEATH and GREET/GET
    
    for value in test_values:
        print(f"\nValue {value}:")
        mob_matches = [name for name, val in mob_flags.items() if val == value]
        obj_matches = [name for name, val in obj_flags.items() if val == value]
        room_matches = [name for name, val in room_flags.items() if val == value]
        
        print(f"  Mob: {mob_matches}")
        print(f"  Obj: {obj_matches}")
        print(f"  Room: {room_matches}")

if __name__ == "__main__":
    analyze_trigger_overlaps()
