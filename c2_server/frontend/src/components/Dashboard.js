import React, { useState, useEffect } from 'react';
import { api } from '../api';

function Dashboard() {
  const [stats, setStats] = useState({
    total: 0,
    active: 0,
    results: 0
  });
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    fetchStats();
    const interval = setInterval(fetchStats, 5000);
    return () => clearInterval(interval);
  }, []);

  const fetchStats = async () => {
    try {
      const [clientsData, resultsData] = await Promise.all([
        api.getClients(),
        api.getResults(1)
      ]);

      const clients = Object.values(clientsData.clients || {});
      const fiveMinutesAgo = new Date(Date.now() - 5 * 60 * 1000);
      const activeCount = clients.filter(c => new Date(c.last_seen) > fiveMinutesAgo).length;

      setStats({
        total: clients.length,
        active: activeCount,
        results: resultsData.count || 0
      });
      setLoading(false);
    } catch (error) {
      console.error('Error fetching stats:', error);
      setLoading(false);
    }
  };

  if (loading) {
    return <div className="loading">Loading...</div>;
  }

  return (
    <div>
      <div className="stats-grid">
        <div className="stat-card">
          <h3>Total Clients</h3>
          <div className="value">{stats.total}</div>
        </div>
        <div className="stat-card active">
          <h3>Active Now</h3>
          <div className="value">{stats.active}</div>
        </div>
        <div className="stat-card">
          <h3>Total Results</h3>
          <div className="value">{stats.results}</div>
        </div>
      </div>

      <div className="card">
        <h2>Quick Start</h2>
        <div className="info-block">
          <div><strong>1.</strong> Check connected clients in the Clients tab</div>
          <div><strong>2.</strong> Send commands to clients using the Send Command tab</div>
          <div><strong>3.</strong> View execution results in the Results tab</div>
        </div>
      </div>
    </div>
  );
}

export default Dashboard;
