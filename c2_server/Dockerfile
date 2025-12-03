FROM python:3.11-slim

WORKDIR /app

# Install dependencies
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy application
COPY app.py .

# Expose port
EXPOSE 8080

# Environment variables
ENV C2_HOST=0.0.0.0
ENV C2_PORT=8080
ENV DEBUG=False

# Run application
CMD ["gunicorn", "--bind", "0.0.0.0:8080", "--workers", "4", "--timeout", "300", "app:app"]
