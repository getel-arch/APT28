#!/usr/bin/env python3
"""
Test client to simulate C2 agent behavior
Demonstrates communication with the C2 server
"""

import requests
import json
import time
import argparse
import random
import sys
from datetime import datetime

class C2Client:
    def __init__(self, server_url, client_id=None):
        """Initialize C2 client"""
        self.server_url = server_url.rstrip('/')
        self.client_id = client_id or f"TEST_CLIENT_{int(time.time())}"
        self.session = requests.Session()
        
    def register(self):
        """Register with the C2 server"""
        try:
            url = f"{self.server_url}/api/register?id={self.client_id}"
            response = self.session.get(url)
            response.raise_for_status()
            print(f"[+] Registered successfully: {self.client_id}")
            print(f"    Response: {response.json()}")
            return True
        except Exception as e:
            print(f"[-] Registration failed: {str(e)}")
            return False
    
    def fetch_command(self):
        """Fetch pending command from server"""
        try:
            url = f"{self.server_url}/api/command?id={self.client_id}"
            response = self.session.get(url)
            response.raise_for_status()
            cmd = int(response.text.strip())
            return cmd
        except Exception as e:
            print(f"[-] Failed to fetch command: {str(e)}")
            return 0
    
    def report_result(self, capability_id, result):
        """Report execution result to server"""
        try:
            url = f"{self.server_url}/api/report"
            payload = {
                "id": self.client_id,
                "capability": capability_id,
                "result": result
            }
            response = self.session.post(url, json=payload)
            response.raise_for_status()
            print(f"[+] Result reported successfully")
            print(f"    Response: {response.json()}")
            return True
        except Exception as e:
            print(f"[-] Failed to report result: {str(e)}")
            return False
    
    def execute_capability(self, capability_id):
        """Simulate capability execution"""
        capabilities = {
            0: ("idle", "No command, idle state"),
            1: ("audio", "Audio recorded: 30 seconds of system audio"),
            2: ("clipboard", "Clipboard contents: Lorem ipsum dolor sit amet..."),
            3: ("keylog", "Keylogger active: Captured 1,234 keystrokes"),
            4: ("screenshot", "Screenshot captured: 1920x1080 PNG (245KB)"),
            5: ("info", "System Info: Windows 10 Pro, Intel i7, 16GB RAM, Hostname: DESKTOP-ABC123"),
        }
        
        if capability_id in capabilities:
            name, result = capabilities[capability_id]
            print(f"[*] Executing capability {capability_id}: {name}")
            return result
        else:
            return f"Unknown capability {capability_id}"
    
    def run_loop(self, iterations=None, delay_min=2, delay_max=5):
        """Run the C2 client loop"""
        iteration = 0
        
        try:
            while True:
                iteration += 1
                
                # Check iteration limit
                if iterations and iteration > iterations:
                    break
                
                print(f"\n[*] --- Iteration {iteration} at {datetime.now().isoformat()} ---")
                
                # Fetch command
                cmd = self.fetch_command()
                print(f"[*] Received command: {cmd}")
                
                # Execute if not idle
                if cmd != 0:
                    result = self.execute_capability(cmd)
                    print(f"[*] Execution result: {result}")
                    
                    # Report result
                    self.report_result(cmd, result)
                else:
                    print(f"[*] No command, standing by...")
                
                # Sleep for random interval
                sleep_time = random.randint(delay_min, delay_max)
                print(f"[*] Sleeping for {sleep_time} seconds...")
                time.sleep(sleep_time)
                
        except KeyboardInterrupt:
            print(f"\n[!] Interrupted by user")
        except Exception as e:
            print(f"[-] Error in main loop: {str(e)}")


def main():
    parser = argparse.ArgumentParser(description='C2 Test Client')
    parser.add_argument('--server', default='http://localhost:8080', help='C2 server URL')
    parser.add_argument('--client-id', help='Custom client ID')
    parser.add_argument('--iterations', type=int, help='Number of iterations to run')
    parser.add_argument('--delay-min', type=int, default=2, help='Minimum delay between checks (seconds)')
    parser.add_argument('--delay-max', type=int, default=5, help='Maximum delay between checks (seconds)')
    parser.add_argument('--register-only', action='store_true', help='Only register and exit')
    parser.add_argument('--send-command', type=int, help='Send command directly (requires --client-id)')
    
    args = parser.parse_args()
    
    # Validate arguments
    if args.send_command and not args.client_id:
        print("[-] --send-command requires --client-id")
        sys.exit(1)
    
    # Create client
    client = C2Client(args.server, args.client_id)
    
    print(f"[*] C2 Test Client")
    print(f"[*] Server: {args.server}")
    print(f"[*] Client ID: {client.client_id}")
    
    # Register
    if not client.register():
        sys.exit(1)
    
    # Register only mode
    if args.register_only:
        sys.exit(0)
    
    # Send command mode
    if args.send_command:
        print(f"[*] Sending command {args.send_command} to {client.client_id}")
        # Create a second session to send command
        server_url = args.server.rstrip('/')
        try:
            response = requests.post(
                f"{server_url}/api/send_command",
                json={
                    "client_id": client.client_id,
                    "capability_id": args.send_command
                }
            )
            print(f"[+] Command sent: {response.json()}")
        except Exception as e:
            print(f"[-] Failed to send command: {str(e)}")
        sys.exit(0)
    
    # Run normal loop
    client.run_loop(
        iterations=args.iterations,
        delay_min=args.delay_min,
        delay_max=args.delay_max
    )


if __name__ == '__main__':
    main()
