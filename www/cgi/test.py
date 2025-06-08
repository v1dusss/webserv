#!/usr/bin/env python3
import os
import sys
import time

# Print CGI response headers
print("Content-Type: text/html")
print("X-Custom-Header: Test")
print()  # Empty line separates headers from body

# Get environment variables
method = os.environ.get("REQUEST_METHOD", "UNKNOWN")
content_length = os.environ.get("CONTENT_LENGTH", "0")
query_string = os.environ.get("QUERY_STRING", "")
content_type = os.environ.get("CONTENT_TYPE", "")
script_name = os.environ.get("SCRIPT_NAME", "")
server_protocol = os.environ.get("SERVER_PROTOCOL", "")
server_software = os.environ.get("SERVER_SOFTWARE", "")

# Read POST data if any
post_data = ""
if method in ["POST", "PUT"] and content_length and int(content_length) > 0:
    post_data = sys.stdin.read(int(content_length))

# Start HTML output
print("<html>")
print("<head><title>CGI Test</title></head>")
print("<body>")
print("<h1>CGI Test Script</h1>")

# Print environment info
print("<h2>Environment Info:</h2>")
print("<ul>")
print(f"<li>REQUEST_METHOD: {method}</li>")
print(f"<li>CONTENT_LENGTH: {content_length}</li>")
print(f"<li>QUERY_STRING: {query_string}</li>")
print(f"<li>CONTENT_TYPE: {content_type}</li>")
print(f"<li>SCRIPT_NAME: {script_name}</li>")
print(f"<li>SERVER_PROTOCOL: {server_protocol}</li>")
print(f"<li>SERVER_SOFTWARE: {server_software}</li>")
print("</ul>")

# Print POST data if applicable
if post_data:
    print("<h2>POST/PUT Data:</h2>")
    print(f"<pre>{post_data}</pre>")

# Print query parameters if any
if query_string:
    print("<h2>Query Parameters:</h2>")
    print("<ul>")
    for param in query_string.split("&"):
        if "=" in param:
            key, value = param.split("=", 1)
            print(f"<li>{key}: {value}</li>")
        else:
            print(f"<li>{param}</li>")
    print("</ul>")

# Test response delay to check timeout handling
print("<h2>Simulating work...</h2>")
print("Waiting 2 seconds...<br>")
sys.stdout.flush()  # Ensure output is sent immediately
time.sleep(2)
print("Done!")

current_time = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
print(f"<p>Current server time: {current_time}</p>")

print("</body>")
print("</html>")