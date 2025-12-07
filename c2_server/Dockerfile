FROM node:18-alpine AS frontend-build

WORKDIR /frontend

# Copy frontend package files
COPY frontend/package*.json ./
RUN npm ci

# Copy frontend source and build
COPY frontend/ ./
RUN npm run build

# Production stage
FROM python:3.11-slim

WORKDIR /app

# Install dependencies
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy application
COPY app.py .
COPY cli.py .

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

# Run application
CMD ["gunicorn", "--bind", "0.0.0.0:8080", "--workers", "4", "--timeout", "300", "--access-logfile", "/app/logs/access.log", "--error-logfile", "/app/logs/error.log", "app:app"]
