#!/usr/bin/env python3
"""
Database initialization script
Run this before starting the application to ensure tables are created
"""
import os
import sys
import logging
from app import app
from models import db

logging.basicConfig(
    level=logging.INFO,
    format='[%(asctime)s] %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

def initialize_database():
    """Initialize the database tables"""
    try:
        with app.app_context():
            logger.info("Starting database initialization...")
            db.create_all()
            logger.info("Database tables created successfully!")
            return True
    except Exception as e:
        logger.error(f"Failed to initialize database: {e}")
        return False

if __name__ == "__main__":
    success = initialize_database()
    sys.exit(0 if success else 1)
