#!/usr/bin/env python3
import os
import json
import datetime

print("Content-Type: application/json\n")

directory = os.path.join(os.path.dirname(__file__), "../../")

try:
    entries = []
    for item in os.listdir(directory):
        path = os.path.join(directory, item)
        stat = os.stat(path)
        info = {
            "name": item + ("/" if os.path.isdir(path) else ""),
            "type": "directory" if os.path.isdir(path) else "file",
            "size": stat.st_size if os.path.isfile(path) else None,
            "modified": datetime.datetime.fromtimestamp(stat.st_mtime).strftime('%Y-%m-%d %H:%M')
        }
        entries.append(info)
    print(json.dumps(entries))

except Exception as e:
    print(json.dumps({"error": str(e)}))
