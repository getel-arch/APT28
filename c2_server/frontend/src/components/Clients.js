import React, { useState, useEffect } from 'react';
import { api } from '../api';

function Clients() {
  const [clients, setClients] = useState({});
  const [loading, setLoading] = useState(true);
  const [selectedClient, setSelectedClient] = useState(null);

  useEffect(() => {
    fetchClients();
    const interval = setInterval(fetchClients, 3000); // Refresh every 3 seconds
    return () => clearInterval(interval);
  }, []);

  const fetchClients = async () => {
    try {
      const data = await api.getClients();
      setClients(data.clients || {});
      setLoading(false);
    } catch (error) {
      console.error('Error fetching clients:', error);
      setLoading(false);
    }
  };

  const isClientActive = (lastSeen) => {
    const fiveMinutesAgo = new Date(Date.now() - 5 * 60 * 1000);
    return new Date(lastSeen) > fiveMinutesAgo;
  };

  const viewClientDetails = async (clientId) => {
    try {
      const data = await api.getClient(clientId);
      setSelectedClient(data);
    } catch (error) {
      console.error('Error fetching client details:', error);
    }
  };

  if (loading) {
    return <div className="loading">Loading clients...</div>;
  }

  const clientArray = Object.values(clients);

  return (
    <div>
      <div className="card">
        <h2>Connected Clients ({clientArray.length})</h2>
        {clientArray.length === 0 ? (
          <p>No clients connected yet.</p>
        ) : (
          <table className="table">
            <thead>
              <tr>
                <th>Client ID</th>
                <th>IP Address</th>
                <th>Status</th>
                <th>Registered</th>
                <th>Last Seen</th>
                <th>Actions</th>
              </tr>
            </thead>
            <tbody>
              {clientArray.map((client) => (
                <tr key={client.id}>
                  <td><code>{client.id}</code></td>
                  <td>{client.ip}</td>
                  <td>
                    <span className={isClientActive(client.last_seen) ? 'status-active' : 'status-inactive'}>
                      {isClientActive(client.last_seen) ? '● Active' : '○ Inactive'}
                    </span>
                  </td>
                  <td>{new Date(client.registered_at).toLocaleString()}</td>
                  <td>{new Date(client.last_seen).toLocaleString()}</td>
                  <td>
                    <button className="btn" onClick={() => viewClientDetails(client.id)}>
                      Details
                    </button>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
      </div>

      {selectedClient && (
        <div className="card">
          <h2>Client Details</h2>
          <button className="btn" style={{ marginBottom: '15px' }} onClick={() => setSelectedClient(null)}>
            Close
          </button>
          <div className="code-block">
            <pre>{JSON.stringify(selectedClient, null, 2)}</pre>
          </div>
        </div>
      )}
    </div>
  );
}

export default Clients;
