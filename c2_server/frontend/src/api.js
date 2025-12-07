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

  // Health check
  getHealth: async () => {
    const response = await axios.get(`${API_BASE_URL}/health`);
    return response.data;
  }
};
