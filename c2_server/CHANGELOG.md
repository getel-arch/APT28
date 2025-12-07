# C2 Server v2.0 - Docker + React Dashboard

## What's New

### Docker-Only Deployment
- Multi-stage Dockerfile with Node.js frontend build
- Production-ready docker-compose configuration
- Persistent volumes for logs and data
- Health checks and auto-restart
- Network isolation

### React Web Dashboard
A modern, responsive web interface with:
- **Dashboard**: Real-time statistics and system overview
- **Clients**: Monitor connected clients and their status
- **Commands**: Send capability commands via web UI
- **Results**: View and analyze execution results

### Enhanced Features
- Static file serving integrated with Flask
- Improved logging and error handling
- API endpoint for frontend integration
- Real-time data refresh (3-5 second intervals)
- Dark theme UI optimized for C2 operations

## File Structure

```
c2_server/
├── Dockerfile                  # Multi-stage build (Node + Python)
├── docker-compose.yml          # Orchestration config
├── app.py                      # Flask backend (updated for frontend)
├── cli.py                      # CLI management tool
├── requirements.txt            # Python dependencies
├── frontend/                   # React application
│   ├── package.json
│   ├── public/
│   │   └── index.html
│   └── src/
│       ├── index.js
│       ├── index.css          # Dark theme styles
│       ├── App.js             # Main component
│       ├── api.js             # API client
│       └── components/
│           ├── Dashboard.js   # Stats & overview
│           ├── Clients.js     # Client management
│           ├── Commands.js    # Command interface
│           └── Results.js     # Results viewer
├── logs/                      # Mounted volume (runtime)
└── README.md                  # Updated documentation
```

## Deployment

### One-Command Start
```bash
cd c2_server
docker-compose up --build
```

### Access
- **Web Dashboard**: http://localhost:8080
- **API**: http://localhost:8080/api/
- **Health**: http://localhost:8080/api/health

## Key Improvements

1. **No Manual Setup**: Everything runs in Docker
2. **Frontend Build**: Automated React build during Docker image creation
3. **Persistent Data**: Logs and data survive container restarts
4. **Production Ready**: Gunicorn WSGI server, proper logging
5. **Easy Scaling**: Docker Compose makes it simple to deploy anywhere

## Configuration

All configuration via docker-compose.yml:
```yaml
environment:
  - C2_HOST=0.0.0.0
  - C2_PORT=8080
  - DEBUG=False
  - FLASK_ENV=production
```

## Management

```bash
# Start in background
docker-compose up -d

# View logs
docker-compose logs -f

# Stop server
docker-compose down

# Rebuild after changes
docker-compose up --build --force-recreate
```

## New Capability Added

**Command Executor (ID: 6)**
- Executes system commands with cmdline evasion
- Bypasses EDR/Sysmon command-line logging
- Uses PEB manipulation technique
- Returns success/failure status

## Browser Compatibility

The dashboard works on:
- Chrome/Edge (recommended)
- Firefox
- Safari
- Any modern browser with JavaScript enabled

## Security Notes

- Dashboard has no authentication (add reverse proxy with auth)
- HTTP only (add HTTPS via nginx reverse proxy for production)
- In-memory storage (consider Redis/PostgreSQL for persistence)
- Rate limiting not implemented (add middleware if needed)

## Next Steps

For production deployment:
1. Add reverse proxy (nginx) with HTTPS
2. Implement authentication (API keys or OAuth)
3. Add database backend (PostgreSQL/MySQL)
4. Configure firewall rules
5. Set up monitoring and alerts
6. Regular backups of volumes
