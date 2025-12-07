import React, { useState } from 'react';
import Dashboard from './components/Dashboard';
import Clients from './components/Clients';
import Commands from './components/Commands';
import Results from './components/Results';
import './index.css';

function App() {
  const [activeTab, setActiveTab] = useState('dashboard');

  return (
    <div className="container">
      <div className="header">
        <h1>⚡ APT28 C2 Server</h1>
        <p>Command & Control Dashboard - Docker Deployment</p>
      </div>

      <div className="nav">
        <button
          className={activeTab === 'dashboard' ? 'active' : ''}
          onClick={() => setActiveTab('dashboard')}
        >
          Dashboard
        </button>
        <button
          className={activeTab === 'clients' ? 'active' : ''}
          onClick={() => setActiveTab('clients')}
        >
          Clients
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
          Results
        </button>
      </div>

      <div className="content">
        {activeTab === 'dashboard' && <Dashboard />}
        {activeTab === 'clients' && <Clients />}
        {activeTab === 'commands' && <Commands />}
        {activeTab === 'results' && <Results />}
      </div>
    </div>
  );
}

export default App;
