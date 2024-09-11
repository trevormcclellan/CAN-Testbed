<!-- <template>
    <div>
      <h1>Home</h1>
      <p>Welcome to the home page!</p>
      <router-link to="/connect">Connect to a Device</router-link>
    </div>
  </template> -->
<template>
  <div>
    <h1>Upload CANdump File</h1>
    <form>
      <input @change="uploadFile" type="file" />
    </form>
    <!-- <p v-if="uploadStatus">{{ uploadStatus }}</p> -->
    <p>Unassigned CAN IDs: {{ uniqueIds.length }}</p>
    <div v-if="ports.length > 0">
      <h2>Nodes</h2>
      <ul>
        <li v-for="(port, index) in portData" :key="index">
          <strong>{{ port.name }}</strong> <!-- {{ node.description }} -->
          <div>
            <label for="selectIds">Select IDs:</label>
            <Multiselect @select="handleSelect" @deselect="handleDeselect" v-model="selectedIds[index]" mode="tags"
              :searchable=true :options="uniqueIds" placeholder="Select CAN IDs" label="ID" track-by="ID"
              :show-labels="false" />
          </div>
          <button @click="selectRemainingIds(index)">Select remaining CAN IDs</button>
        </li>
      </ul>
    </div>
    <button @click="uploadToECUs">Upload to ECUs</button>
  </div>
</template>


<script>
import Multiselect from '@vueform/multiselect';
import '@vueform/multiselect/themes/default.css';

export default {
  components: {
    Multiselect
  },
  data() {
    return {
      ports: [],
      portData: [],
      canData: {},
      uniqueIds: [],
      selectedIds: [],
      fileContent: null,
    };
  },

  methods: {
    handleSelect(value) {
      let index = this.uniqueIds.indexOf(value);
      if (index !== -1) {
        this.uniqueIds.splice(index, 1);
      }
      this.saveSelectedIds();
    },

    handleDeselect(value) {
      this.uniqueIds.push(value);
      this.saveSelectedIds();
    },

    uploadFile(event) {
      const file = event.target.files[0];
      if (!file) {
        this.error = 'No file selected';
        return;
      }

      const reader = new FileReader();
      reader.onload = (e) => {
        this.fileContent = e.target.result;
        this.processFile(this.fileContent);
      };
      reader.readAsText(file);
    },

    processFile(fileContent) {
      try {
        this.canData = this.parseCandumpFile(fileContent);
        this.uniqueIds = this.canData ? Object.keys(this.canData) : [];
        console.log("CAN Data:", this.canData);
        this.error = '';
      } catch (err) {
        this.error = 'Error processing file';
        console.error(err);
      }
    },

    // parseCandumpFile(content) {
    //   const canIds = new Set();
    //   const lines = content.split('\n');
    //   lines.forEach((line) => {
    //     const parts = line.trim().split(/\s+/);
    //     if (parts.length > 2) {
    //       // Example line format: (1290000000.000000) can0 585#0040000000020018
    //       const canId = parts[2].split('#')[0]; // Extract CAN ID before the #
    //       canIds.add(canId);
    //     }
    //   });
    //   return Array.from(canIds); // Convert Set to Array
    // },


    parseCandumpFile(content) {
      const canData = {}; // Object to store the transformed data
      let firstTimestamp = null; // Variable to store the first timestamp for relative calculation

      const lines = content.split('\n');
      lines.forEach((line) => {
        const parts = line.trim().split(/\s+/);

        if (parts.length > 2) {
          // Example line format: (1290000000.000000) can0 585#0040000000020018
          const timestamp = parseFloat(parts[0].replace(/[()]/g, '')); // Convert timestamp to a number
          const [canId, data] = parts[2].split('#'); // Split CAN ID and data

          // Set the first timestamp if not already set
          if (firstTimestamp === null) {
            firstTimestamp = timestamp;
          }

          // Calculate the relative timestamp
          const relativeTimestamp = timestamp - firstTimestamp;

          // If the CAN ID doesn't exist in the object, initialize it with an empty array
          if (!canData[canId]) {
            canData[canId] = [];
          }

          // Push the relevant data as an object into the array for this CAN ID
          canData[canId].push({
            timestamp: relativeTimestamp, // Use relative timestamp
            data: data || '' // Data part (after #), default to an empty string if missing
          });
        }
      });

      return canData; // Return the transformed object with relative timestamps
    },

    async getPreviouslyConnectedPorts() {
      try {
        this.ports = await navigator.serial.getPorts();
        this.portData = Array(this.ports.length).fill({ name: '', status: 'Checking...' });
        this.selectedIds = this.loadSelectedIds(this.ports.length);
        for (let i = 0; i < this.ports.length; i++) {
          console.log(`Checking port ${i}`);
          await this.checkIfFlashed(this.ports[i], i);
        }
      } catch (error) {
        console.error('Failed to get previously connected ports:', error);
      }
    },
    async checkIfFlashed(port, index) {
      try {
        // Only open the port if it's not already open
        if (!port.readable || !port.writable) {
          console.log(`Opening port ${index}`);
          await port.open({ baudRate: 115200 });
        }

        const writer = port.writable.getWriter();
        const data = new TextEncoder().encode("get_name\n");
        console.log(`Sending command to port ${index}`);
        await writer.write(data);
        writer.releaseLock();

        const reader = port.readable.getReader();
        const timeout = 10000; // Timeout in milliseconds (2 seconds)
        const timeoutPromise = new Promise((resolve) => setTimeout(resolve, timeout, { done: true }));

        // Attempt to read from the port, but with a timeout
        const { value, done } = await Promise.race([reader.read(), timeoutPromise]);
        reader.releaseLock();

        if (done || !value) {
          console.log(`Port ${index} - No response within timeout, marking as 'Not Flashed'`);
          this.portData[index].status = 'Not Flashed'; // Directly update the array
        } else {
          const response = new TextDecoder().decode(value).trim();
          console.log(`Port ${index} - Received response: ${response}`);
          if (response) {
            this.portData[index].status = 'Flashed'; // Directly update the array
            // Response format: "Device Name: <name>"
            const deviceName = response.split(': ')[1];
            this.portData[index].name = deviceName
          } else {
            this.portData[index].status = 'Not Flashed'; // Directly update the array
          }
        }
      } catch (error) {
        console.error(`Error checking if device on port ${index} is flashed:`, error);
        this.portData[index].status = 'Unknown'; // Directly update the array
      }
    },
    uploadToECUs() {
      console.log('Uploading to ECUs');
      const lines = this.fileContent.split('\n');
      this.ports.forEach((port, index) => {
        console.log(this.portData[index]);
        if (this.portData[index].status === 'Flashed') {
          console.log(`Uploading to port ${index}`);
          let selectedIds = this.selectedIds[index];
          let filteredData = {};
          selectedIds.forEach((id) => {
            if (this.canData[id]) {
              filteredData[id] = this.canData[id]; // Add entries for this CAN ID if it exists in canData
            }
          });
          this.uploadToECU(port, filteredData);
        }
      });
    },
    async uploadToECU(port, data) {
      try {
        const writer = port.writable.getWriter();
        const dataString = JSON.stringify(data);
        const dataEncoded = new TextEncoder().encode(`upload:${dataString}:end\n`);

        // Send the data to the Arduino
        await writer.write(dataEncoded);
        writer.releaseLock();
        console.log('Uploaded data to ECU');

        // Check for a response from the Arduino
        const reader = port.readable.getReader();
        const decoder = new TextDecoder();

        // Timeout mechanism to prevent waiting indefinitely
        const timeout = 5000; // 5 seconds timeout
        const timeoutPromise = new Promise((_, reject) => setTimeout(() => reject(new Error('Timeout waiting for ECU response')), timeout));

        const responsePromise = reader.read().then(({ value, done }) => {
          if (done || !value) {
            throw new Error('No response from ECU');
          }
          return decoder.decode(value);
        });

        // Await either the response or the timeout
        const response = await Promise.race([responsePromise, timeoutPromise]);

        reader.releaseLock();

        // Check the response from the Arduino
        if (response.includes('JSON upload processed successfully')) {
          console.log('Upload successful:', response);
        } else if (response.includes('Failed to parse JSON')) {
          console.error('ECU reported a JSON parsing error:', response);
        } else {
          console.error('Unexpected response from ECU:', response);
        }
      } catch (error) {
        // Handle errors from the upload or response check
        console.error('Failed to upload data to ECU:', error);
        alert(`Error uploading data to ECU: ${error.message}`);
      }
    },
    selectRemainingIds(index) {
      this.selectedIds[index] = this.uniqueIds;
      this.uniqueIds = [];
      this.saveSelectedIds();
    },
    saveSelectedIds() {
      localStorage.setItem('selectedIds', JSON.stringify(this.selectedIds));
    },
    loadSelectedIds(length) {
      const savedIds = localStorage.getItem('selectedIds');
      return savedIds ? JSON.parse(savedIds) : Array(length).fill([]);
    }
  },
  mounted() {
    this.getPreviouslyConnectedPorts();
  },
};
</script>