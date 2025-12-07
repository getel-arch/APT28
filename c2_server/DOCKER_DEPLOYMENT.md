# Docker Deployment Instructions

## Prerequisites

- Docker 20.10 or higher
- Docker Compose 1.29 or higher

## Quick Start

```bash
cd c2_server
docker-compose up --build
```

Access the dashboard at: http://localhost:8080

## Commands

### Build and Start
```bash
docker-compose up --build
```

### Run in Background
```bash
docker-compose up -d
```

### View Logs
```bash
docker-compose logs -f
```

### Stop Server
```bash
docker-compose down
```

### Rebuild Everything
```bash
docker-compose down -v
docker-compose build --no-cache
docker-compose up
```

## Volumes

- `./logs` - Server logs (host-mounted)
- `c2_data` - Persistent data (Docker volume)

## Environment Variables

Configure in `docker-compose.yml`:
- `C2_HOST` - Bind address (default: 0.0.0.0)
- `C2_PORT` - Port number (default: 8080)
- `DEBUG` - Debug mode (default: False)
- `FLASK_ENV` - Flask environment (default: production)

## Production Deployment

1. **Change default port if needed**
2. **Add reverse proxy (nginx) with HTTPS**
3. **Configure firewall rules**
4. **Set up monitoring and alerting**
5. **Regular backups of volumes**

## Health Check

```bash
curl http://localhost:8080/api/health
```

Expected response:
```json
{"status": "healthy", ...}
```
