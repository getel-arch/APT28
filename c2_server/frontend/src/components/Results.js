import React, { useState, useEffect } from 'react';
import { api, CAPABILITIES } from '../api';

function Results() {
  const [results, setResults] = useState([]);
  const [loading, setLoading] = useState(true);
  const [filter, setFilter] = useState('all');
  const [selectedResult, setSelectedResult] = useState(null);

  useEffect(() => {
    fetchResults();
    const interval = setInterval(fetchResults, 3000); // Refresh every 3 seconds
    return () => clearInterval(interval);
  }, []);

  const fetchResults = async () => {
    try {
      const data = await api.getResults(200);
      setResults(data.results || []);
      setLoading(false);
    } catch (error) {
      console.error('Error fetching results:', error);
      setLoading(false);
    }
  };

  const filteredResults = filter === 'all' 
    ? results 
    : results.filter(r => r.capability === parseInt(filter));

  const viewResultDetails = (result) => {
    setSelectedResult(result);
  };

  if (loading) {
    return <div className="loading">Loading results...</div>;
  }

  return (
    <div>
      <div className="card">
        <h2>Execution Results ({results.length})</h2>
        
        <div className="form-group" style={{ maxWidth: '300px', marginBottom: '20px' }}>
          <label htmlFor="filter">Filter by Capability</label>
          <select
            id="filter"
            value={filter}
            onChange={(e) => setFilter(e.target.value)}
          >
            <option value="all">All Capabilities</option>
            {Object.entries(CAPABILITIES)
              .filter(([id]) => id !== '0')
              .map(([id, name]) => (
                <option key={id} value={id}>
                  {name}
                </option>
              ))}
          </select>
        </div>

        {filteredResults.length === 0 ? (
          <p>No results yet.</p>
        ) : (
          <table className="table">
            <thead>
              <tr>
                <th>Timestamp</th>
                <th>Client ID</th>
                <th>Capability</th>
                <th>Result Preview</th>
                <th>Actions</th>
              </tr>
            </thead>
            <tbody>
              {filteredResults.slice().reverse().map((result, idx) => (
                <tr key={idx}>
                  <td>{new Date(result.timestamp).toLocaleString()}</td>
                  <td><code>{result.client}</code></td>
                  <td>{CAPABILITIES[result.capability] || `Unknown (${result.capability})`}</td>
                  <td>
                    <code>
                      {result.result && result.result.length > 50 
                        ? result.result.substring(0, 50) + '...' 
                        : result.result || 'N/A'}
                    </code>
                  </td>
                  <td>
                    <button className="btn" onClick={() => viewResultDetails(result)}>
                      View Full
                    </button>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
      </div>

      {selectedResult && (
        <div className="card">
          <h2>Result Details</h2>
          <button className="btn" style={{ marginBottom: '15px' }} onClick={() => setSelectedResult(null)}>
            Close
          </button>
          
          <div style={{ marginBottom: '15px' }}>
            <strong>Client:</strong> <code>{selectedResult.client}</code><br />
            <strong>Capability:</strong> {CAPABILITIES[selectedResult.capability]}<br />
            <strong>Timestamp:</strong> {new Date(selectedResult.timestamp).toLocaleString()}<br />
            <strong>IP:</strong> {selectedResult.ip}
          </div>

          <div className="form-group">
            <label>Result Data</label>
            <textarea
              readOnly
              rows="15"
              value={selectedResult.result || 'No result data'}
              style={{ fontFamily: 'monospace', fontSize: '12px' }}
            />
          </div>
        </div>
      )}
    </div>
  );
}

export default Results;
