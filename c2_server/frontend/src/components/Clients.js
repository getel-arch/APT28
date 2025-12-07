import React, { useState, useEffect } from 'react';
import { api } from '../api';

function Clients({ onClientSelect }) {
  const [clients, setClients] = useState({});
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    fetchClients();
    const interval = setInterval(fetchClients, 5000); // Refresh every 5 seconds
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
                    <button className="btn" onClick={() => onClientSelect && onClientSelect(client.id)}>
                      View Details
                    </button>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
      </div>
    </div>
  );
}

export default Clients;
