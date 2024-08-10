#! /usr/bin/env python3
import sys
from pathlib import Path
import os
import json

def select_dump_folder(dump_directory: Path) -> Path:
    """
    Analyzes <dump_directory> and finds the latest dump folder.
    Dump folders are in the format of dump-YYYYMMDDHHMMSS, so a lexicographic sort should work.
    Raises an exception if there is no dump available.
    """
    if not dump_directory.exists() or not dump_directory.is_dir():
        raise FileNotFoundError(f"Directory {dump_directory} does not exist or is not a directory.")

    # List all directories in the dump_directory
    dump_folders = [d for d in dump_directory.iterdir() if d.is_dir() and d.name.startswith("dump-")]

    if not dump_folders:
        raise FileNotFoundError("No dump folders available.")

    # Sort the folders lexicographically and select the latest one
    latest_dump = sorted(dump_folders)[-1]

    return latest_dump

def readJsonFile(dump_dir: Path, file_name: str):
    with open(dump_dir / file_name) as f:
        data = json.load(f)
    return data

def tool_index_obj_apply(dump_dir: Path, args):
    location = int(args[0])

    data = readJsonFile(dump_dir, "itemPrototypes.json")

    print(f"Searching through {len(data)} prototypes for affected: {location}")
    total = 0
    for proto in data:
        affect = []

        for aff in proto.get("affected", list()):
            if (loc := aff.pop("location", -1)) == location:
                affect.append(aff)

        if affect:
            total += 1
            print(f"VN: {proto['vn']} - {proto['name']} - {len(affect)} affects found")
            for aff in affect:
                print(repr(aff))

    print(f"Total Found: {total}")


def tool_index_obj_flag(dump_dir: Path, args):
    flag = int(args[0])

    data = readJsonFile(dump_dir, "itemPrototypes.json")

    print(f"Searching through {len(data)} prototypes for flag: {flag}")
    total = 0
    for proto in data:

        if flag in set(proto.get("extra_flags", list())):
            total += 1
            print(f"VN: {proto['vn']} - {proto['name']}")

    print(f"Total Found: {total}")

tools = {
    # Provided an affect (and, optionally, a specific), lists everything with matching obj_affected_type
    "SearchObjApply": tool_index_obj_apply,
    "SearchObjFlag": tool_index_obj_flag
}

def main():
    # Read the dump directory from command line arguments or use default
    dump_directory = Path.cwd() / "lib" / "dumps"

    try:
        dump = select_dump_folder(dump_directory)
        print(f"Latest dump folder selected: {dump}")
        if len(sys.argv) < 2:
            raise ValueError("Not enough arguments to select operation.")
        if not (func := tools.get(sys.argv[1], None)):
            raise ValueError(f"{sys.argv[1]}: Not a valid operation.")
        print("Running operation...")
        func(dump, sys.argv[2:])
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()