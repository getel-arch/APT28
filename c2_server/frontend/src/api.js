import axios from 'axios';

const API_BASE_URL = '/api';

export const api = {
  // Get all clients
  getClients: async () => {
    const response = await axios.get(`${API_BASE_URL}/clients`);
    return response.data;
  },

  // Get specific client
  getClient: async (clientId) => {
    const response = await axios.get(`${API_BASE_URL}/client/${clientId}`);
    return response.data;
  },

  // Get all results
  getResults: async (limit = 100) => {
    const response = await axios.get(`${API_BASE_URL}/results?limit=${limit}`);
    return response.data;
  },

  // Get results for specific client
  getClientResults: async (clientId) => {
    const response = await axios.get(`${API_BASE_URL}/client/${clientId}/results`);
    return response.data;
  },

  // Send command to client
  sendCommand: async (clientId, capabilityId, args = '') => {
    const response = await axios.post(`${API_BASE_URL}/send_command`, {
      client_id: clientId,
      capability_id: capabilityId,
      args: args
    });
    return response.data;
  },

  // Get capabilities
  getCapabilities: async () => {
    const response = await axios.get(`${API_BASE_URL}/capabilities`);
    return response.data.capabilities;
  },

  // Get all exfiltrated files
  getFiles: async (clientId = null) => {
    const url = clientId ? `${API_BASE_URL}/files?client_id=${clientId}` : `${API_BASE_URL}/files`;
    const response = await axios.get(url);
    return response.data;
  },

  // Get specific file
  getFile: async (fileId) => {
    const response = await axios.get(`${API_BASE_URL}/files/${fileId}`);
    return response.data;
  },

  // Download file
  downloadFile: async (fileId, filename) => {
    const response = await axios.get(`${API_BASE_URL}/files/${fileId}/download`, {
      responseType: 'blob'
    });
    const url = window.URL.createObjectURL(new Blob([response.data]));
    const link = document.createElement('a');
    link.href = url;
    link.setAttribute('download', filename);
    document.body.appendChild(link);
    link.click();
    link.remove();
  },

  // Get files from specific client
  getClientFiles: async (clientId) => {
    const response = await axios.get(`${API_BASE_URL}/client/${clientId}/files`);
    return response.data;
  },

  // Health check
  getHealth: async () => {
    const response = await axios.get(`${API_BASE_URL}/health`);
    return response.data;
  }
};
