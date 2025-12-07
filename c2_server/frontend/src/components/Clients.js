import React, { useState, useEffect } from 'react';
import { api } from '../api';

function Clients() {
  const [clients, setClients] = useState([]);
  const [selectedClient, setSelectedClient] = useState(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    fetchClients();
    const interval = setInterval(fetchClients, 5000);
    return () => clearInterval(interval);
  }, []);

  const fetchClients = async () => {
    try {
      const data = await api.getClients();
      setClients(Object.values(data.clients || {}));
      setLoading(false);
    } catch (error) {
      console.error('Error fetching clients:', error);
      setLoading(false);
    }
  };

  const isActive = (lastSeen) => {
    return new Date(Date.now() - 5 * 60 * 1000) < new Date(lastSeen);
  };

  const viewDetails = async (clientId) => {
    try {
      const data = await api.getClient(clientId);
      const results = await api.getClientResults(clientId);
      setSelectedClient({
        ...data.client,
        results: results.results || []
      });
    } catch (error) {
      console.error('Error fetching client details:', error);
    }
  };

  if (loading) {
    return <div className="loading">Loading clients...</div>;
  }

  if (selectedClient) {
    return (
      <div>
        <div className="card">
          <button className="btn btn-secondary" onClick={() => setSelectedClient(null)}>
            ← Back to Clients
          </button>
          <h2 style={{ marginTop: '1rem' }}>Client Details</h2>
          
          <div className="info-block" style={{ marginTop: '1rem' }}>
            <div><strong>Client ID:</strong> <code>{selectedClient.id}</code></div>
            <div><strong>IP Address:</strong> {selectedClient.ip}</div>
            <div>
              <strong>Status:</strong>{' '}
              <span className={`status-badge ${isActive(selectedClient.last_seen) ? 'active' : 'inactive'}`}>
                {isActive(selectedClient.last_seen) ? 'Active' : 'Inactive'}
              </span>
            </div>
            <div><strong>First Seen:</strong> {new Date(selectedClient.registered_at).toLocaleString()}</div>
            <div><strong>Last Seen:</strong> {new Date(selectedClient.last_seen).toLocaleString()}</div>
          </div>
        </div>

        <div className="card">
          <h2>Recent Results ({selectedClient.results.length})</h2>
          {selectedClient.results.length === 0 ? (
            <div className="empty-state">
              <p>No results yet</p>
            </div>
          ) : (
            <div className="table-container">
              <table className="table">
                <thead>
                  <tr>
                    <th>Timestamp</th>
                    <th>Capability</th>
                    <th>Type</th>
                  </tr>
                </thead>
                <tbody>
                  {selectedClient.results.slice(0, 10).map((result, idx) => (
                    <tr key={idx}>
                      <td>{new Date(result.timestamp).toLocaleString()}</td>
                      <td>{result.capability}</td>
                      <td>{result.result_type || 'unknown'}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          )}
        </div>
      </div>
    );
  }

  return (
    <div className="card">
      <h2>Connected Clients ({clients.length})</h2>
      {clients.length === 0 ? (
        <div className="empty-state">
          <p>No clients connected</p>
          <small>Waiting for clients to connect...</small>
        </div>
      ) : (
        <div className="table-container">
          <table className="table">
            <thead>
              <tr>
                <th>Client ID</th>
                <th>IP Address</th>
                <th>Status</th>
                <th>Last Seen</th>
                <th>Actions</th>
              </tr>
            </thead>
            <tbody>
              {clients.map((client) => (
                <tr key={client.id}>
                  <td><code>{client.id}</code></td>
                  <td>{client.ip}</td>
                  <td>
                    <span className={`status-badge ${isActive(client.last_seen) ? 'active' : 'inactive'}`}>
                      {isActive(client.last_seen) ? 'Active' : 'Inactive'}
                    </span>
                  </td>
                  <td>{new Date(client.last_seen).toLocaleString()}</td>
                  <td>
                    <button className="btn" onClick={() => viewDetails(client.id)}>
                      View Details
                    </button>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}
    </div>
  );
}

export default Clients;
