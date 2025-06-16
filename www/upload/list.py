#!/usr/bin/env python3
import os
import json
import datetime

print("Content-Type: application/json\n")

directory = os.path.abspath(os.path.dirname(__file__))

try:
    entries = []
    for item in os.listdir(directory):
        path = os.path.join(directory, item)
        # Check if the item is a file and has an image extension
        if os.path.isfile(path) and item.lower().endswith(('.jpg', '.jpeg', '.png', '.gif', '.webp')):
            stat = os.stat(path)
            info = {
                "name": item,
                "type": "file",
                "size": stat.st_size,
                "modified": datetime.datetime.fromtimestamp(stat.st_mtime).strftime('%Y-%m-%d %H:%M')
            }
            entries.append(info)
    print(json.dumps(entries))

except Exception as e:
    print(json.dumps({"error": str(e)}))
