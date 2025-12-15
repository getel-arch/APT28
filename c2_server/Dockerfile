FROM node:18-alpine AS frontend-build

WORKDIR /frontend

# Copy frontend package files
COPY frontend/package*.json ./
RUN npm install

# Copy frontend source and build
COPY frontend/ ./
RUN npm run build

# Production stage
FROM python:3.11-slim

WORKDIR /app

# Install system dependencies for PostgreSQL
RUN apt-get update && apt-get install -y \
    libpq-dev \
    gcc \
    && rm -rf /var/lib/apt/lists/*

# Install dependencies
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy application
COPY app.py .
COPY cli.py .
COPY models.py .
COPY init_db.py .

# Copy frontend build from previous stage
COPY --from=frontend-build /frontend/build ./static

# Create logs directory
RUN mkdir -p /app/logs

# Expose port
EXPOSE 8080

# Environment variables
ENV C2_HOST=0.0.0.0
ENV C2_PORT=8080
ENV DEBUG=False
ENV FLASK_ENV=production

# Create entrypoint script
RUN echo '#!/bin/sh' > /app/entrypoint.sh && \
    echo 'python init_db.py' >> /app/entrypoint.sh && \
    echo 'exec gunicorn --bind 0.0.0.0:8080 --workers 4 --timeout 300 --preload --access-logfile /app/logs/access.log --error-logfile /app/logs/error.log app:app' >> /app/entrypoint.sh && \
    chmod +x /app/entrypoint.sh

# Run application with proper initialization
CMD ["/app/entrypoint.sh"]
