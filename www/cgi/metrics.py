import os
import sys
import json
import shutil
import subprocess
import platform
import re

def get_cpu_load():
    if platform.system() == "Darwin":  # macOS
        try:
            out = subprocess.check_output(["top", "-l", "1"]).decode()
            m = re.search(r"CPU usage: ([\d.]+)% user, ([\d.]+)% sys", out)
            if m:
                user, sys = map(float, m.groups())
                return user + sys
        except:
            return 0.0
    elif platform.system().startswith("Linux"):  # Linux
        try:
            with open("/proc/stat", "r") as f:
                lines = f.readlines()
            for line in lines:
                if line.startswith("cpu "):
                    parts = line.split()
                    user = int(parts[1])
                    nice = int(parts[2])
                    system = int(parts[3])
                    idle = int(parts[4])
                    total = user + nice + system + idle
                    active = user + nice + system
                    return (active / total) * 100
        except:
            return 0.0
    return 0.0

def get_memory():
    if platform.system() == "Darwin":  # macOS
        try:
            page_size = os.sysconf('SC_PAGE_SIZE')
            total_pages = os.sysconf('SC_PHYS_PAGES')
        except:
            return {'total': 0, 'used': 0}

        try:
            out = subprocess.check_output(['vm_stat'], stderr=subprocess.DEVNULL).decode()
        except:
            return {'total': 0, 'used': 0}

        free_pages = 0
        for line in out.splitlines():
            if line.startswith('Pages free:'):
                parts = re.findall(r'\d+', line)
                if parts:
                    free_pages = int(parts[0])
                break

        total = total_pages * page_size
        free = free_pages * page_size
        used = total - free

        return {'total': total, 'used': used}

    elif platform.system().startswith("Linux"):  # Linux
        try:
            with open('/proc/meminfo', 'r') as f:
                meminfo = f.read()
            total = int(re.search(r'MemTotal:\s+(\d+)', meminfo).group(1)) * 1024
            free = int(re.search(r'MemFree:\s+(\d+)', meminfo).group(1)) * 1024
            buffers = int(re.search(r'Buffers:\s+(\d+)', meminfo).group(1)) * 1024
            cached = int(re.search(r'Cached:\s+(\d+)', meminfo).group(1)) * 1024
            used = total - (free + buffers + cached)
            return {'total': total, 'used': used}
        except:
            return {'total': 0, 'used': 0}

    return {'total': 0, 'used': 0}
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