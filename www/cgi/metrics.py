#!/usr/bin/env python3

import os
import sys
import json
import shutil
import subprocess
import re

def get_cpu_load():
    try:
        return os.getloadavg()[0]
    except:
        return 0.0

def get_memory():
    """
    On macOS, only 'Pages free' is truly unused.
    Compute:
      total = SC_PHYS_PAGES × SC_PAGE_SIZE
      free  = Pages free × SC_PAGE_SIZE
      used  = total – free
    """
    # get page size and total pages
    try:
        page_size = os.sysconf('SC_PAGE_SIZE')
        total_pages = os.sysconf('SC_PHYS_PAGES')
    except:
        return {'total': 0, 'used': 0}

    # parse vm_stat
    try:
        out = subprocess.check_output(['vm_stat'], stderr=subprocess.DEVNULL).decode()
    except:
        return {'total': 0, 'used': 0}

    # extract free pages
    free_pages = 0
    for line in out.splitlines():
        if line.startswith('Pages free:'):
            parts = re.findall(r'\d+', line)
            if parts:
                free_pages = int(parts[0])
            break

    total = total_pages * page_size
    free  = free_pages  * page_size
    used  = total - free

    return {'total': total, 'used': used}

def get_disk():
    try:
        du = shutil.disk_usage('/')
        return {'total': du.total, 'used': du.used}
    except:
        return {'total': 0, 'used': 0}

def main():
    payload = {
        'cpu_load': get_cpu_load(),
        'memory':   get_memory(),
        'disk':     get_disk()
    }
    # CGI headers
    print("Content-Type: application/json")
    print()
    sys.stdout.write(json.dumps(payload))

if __name__ == '__main__':
    main()