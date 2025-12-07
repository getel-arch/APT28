import React, { useState } from 'react';
import Dashboard from './components/Dashboard';
import Clients from './components/Clients';
import Commands from './components/Commands';
import Results from './components/Results';
import './index.css';

function App() {
  const [activeView, setActiveView] = useState('dashboard');

  return (
    <div className="app">
      <header className="header">
        <h1>⚡ APT28 C2</h1>
        <nav className="nav">
          <button
            className={activeView === 'dashboard' ? 'nav-btn active' : 'nav-btn'}
            onClick={() => setActiveView('dashboard')}
          >
            📊 Dashboard
          </button>
          <button
            className={activeView === 'clients' ? 'nav-btn active' : 'nav-btn'}
            onClick={() => setActiveView('clients')}
          >
            💻 Clients
          </button>
          <button
            className={activeView === 'commands' ? 'nav-btn active' : 'nav-btn'}
            onClick={() => setActiveView('commands')}
          >
            ⚙️ Send Command
          </button>
          <button
            className={activeView === 'results' ? 'nav-btn active' : 'nav-btn'}
            onClick={() => setActiveView('results')}
          >
            📋 Results
          </button>
        </nav>
      </header>

      <main className="main-content">
        {activeView === 'dashboard' && <Dashboard />}
        {activeView === 'clients' && <Clients />}
        {activeView === 'commands' && <Commands />}
        {activeView === 'results' && <Results />}
      </main>
    </div>
  );
}

export default App;
