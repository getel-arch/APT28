import React, { useState, useEffect } from 'react';
import { api } from '../api';

function Dashboard() {
  const [stats, setStats] = useState({
    clients_count: 0,
    results_count: 0,
    active_clients: 0
  });
  const [health, setHealth] = useState(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    fetchDashboardData();
    const interval = setInterval(fetchDashboardData, 5000); // Refresh every 5 seconds
    return () => clearInterval(interval);
  }, []);

  const fetchDashboardData = async () => {
    try {
      const [healthData, clientsData, resultsData] = await Promise.all([
        api.getHealth(),
        api.getClients(),
        api.getResults()
      ]);

      setHealth(healthData);
      
      // Calculate active clients (seen in last 5 minutes)
      const fiveMinutesAgo = new Date(Date.now() - 5 * 60 * 1000);
      const activeCount = Object.values(clientsData.clients || {}).filter(client => {
        return new Date(client.last_seen) > fiveMinutesAgo;
      }).length;

      setStats({
        clients_count: clientsData.count || 0,
        results_count: resultsData.count || 0,
        active_clients: activeCount
      });

      setLoading(false);
    } catch (error) {
      console.error('Error fetching dashboard data:', error);
      setLoading(false);
    }
  };

  if (loading) {
    return <div className="loading">Loading dashboard...</div>;
  }

  return (
    <div>
      <div className="stats">
        <div className="stat-card">
          <h3>Total Clients</h3>
          <div className="value">{stats.clients_count}</div>
        </div>
        <div className="stat-card">
          <h3>Active Clients</h3>
          <div className="value">{stats.active_clients}</div>
        </div>
        <div className="stat-card">
          <h3>Total Results</h3>
          <div className="value">{stats.results_count}</div>
        </div>
        <div className="stat-card">
          <h3>Server Status</h3>
          <div className="value" style={{ fontSize: '1.5em' }}>
            {health?.status === 'healthy' ? '✓ Online' : '✗ Offline'}
          </div>
        </div>
      </div>

      <div className="card">
        <h2>System Information</h2>
        <div className="code-block">
          <div><strong>Server:</strong> APT28 C2 Server v2.0.0</div>
          <div><strong>Deployment:</strong> Docker-Only</div>
          <div><strong>Status:</strong> {health?.status || 'Unknown'}</div>
          <div><strong>Last Updated:</strong> {new Date().toLocaleString()}</div>
        </div>
      </div>

      <div className="card">
        <h2>Capabilities</h2>
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
              <td>Record audio from client microphone</td>
            </tr>
            <tr>
              <td>2</td>
              <td>Clipboard Monitor</td>
              <td>Monitor clipboard activity</td>
            </tr>
            <tr>
              <td>3</td>
              <td>Keylogger</td>
              <td>Capture keyboard input</td>
            </tr>
            <tr>
              <td>4</td>
              <td>Screenshot</td>
              <td>Take screenshots of client display</td>
            </tr>
            <tr>
              <td>5</td>
              <td>Info Collector</td>
              <td>Collect system information</td>
            </tr>
            <tr>
              <td>6</td>
              <td>Command Executor</td>
              <td>Execute commands with cmdline evasion</td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>
  );
}

export default Dashboard;
