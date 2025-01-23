<template>
  <div>
    <div>
      <Modal v-model:visible="showModal">
        <div v-if="modalType == 'attackConfig'" class="attack-config">
          <h2>Select an Attack Type</h2>
          <div class="select-container">
            <label for="attackType">Attack Type:</label>
            <select id="attackType" v-model="attack.type" class="styled-select">
              <option value="replay">Replay</option>
            </select>
          </div>
          <div v-if="attack.type === 'replay'" class="replay-config">
            <h3>Replay Attack Configuration</h3>
            <form @submit.prevent="configureReplay">
              <div class="form-group">
                <label for="message">Message (ID#DATA):</label>
                <input id="message" type="text" placeholder="Enter the message" v-model="replayConfig.message" />
              </div>
              <div class="form-group">
                <label for="repeat">Repeat:</label>
                <input id="repeat" type="number" min="1" placeholder="Enter the number of times"
                  v-model.number="replayConfig.repeat" />
              </div>
              <div class="form-group">
                <label for="interval">Interval (ms):</label>
                <input id="interval" type="number" min="0" placeholder="Enter the interval"
                  v-model.number="replayConfig.interval" />
              </div>
              <div class="form-group">
                <label for="startTime">Start Time (ms):</label>
                <input id="startTime" type="number" min="0" placeholder="Enter the start time"
                  v-model.number="replayConfig.startTime" />
              </div>
              <button type="submit" class="configure-button">Configure</button>
            </form>
          </div>
        </div>
        <div v-else-if="modalType == 'ignoreIds'">
          <h2>Ignore CAN IDs</h2>
          <p>Enter the CAN IDs you want to ignore (in hexadecimal format):</p>
          <Multiselect v-model="ignoredIDs" mode="tags" :searchable=true placeholder="Enter CAN IDs" label="ID" track-by="ID"
            :show-labels="false" :createTag="true" :addTagOn="['enter', 'space', 'tab', ';', ',']"
            :showOptions="false" />
          <button @click="uploadIgnoredIDs">Save</button>
        </div>
        <div v-else-if="modalType == 'maskConfig'">
          <h2>Configure Mask</h2>
          <p>Enter the mask value (in hexadecimal format):</p>
          <input type="text" v-model="mask.value" placeholder="Enter the mask value" />
          <p>Matching IDs:</p>
          <div class="scroll-container">
            <ul>
              <li v-for="id in findMatchingIDs(selectedIds[mask.index], parseInt(mask.value, 16))" :key="id">{{ id }}</li>
            </ul>
          </div>
          
          <button @click="saveMaskValue">Save</button>
        </div>
      </Modal>
    </div>
    <h1>Upload CANdump File</h1>
    <form>
      <input @change="uploadFile" type="file" />
    </form>
    <div>
      <button @click="ignoreIDs">Ignore IDs</button>
      <button @click="uploadToECUs">Upload IDs to ECUs</button> <br>
      <button @click="beginSimulation">{{ simulationStartTime ? 'Restart' : 'Start' }} Simulation</button>
    </div>
    <h1>Serial Consoles</h1>
    <div v-if="serialPorts.length === 0">
      <p>No serial ports connected.</p>
    </div>
    <div v-for="(port, index) in serialPorts" :key="port.info">
      <h2>Console {{ index + 1 }} - {{ port.deviceName || 'Unknown Device' }}</h2>
      <div>
        <textarea v-model="port.consoleOutput" rows="10" cols="50" readonly></textarea>
      </div>
      <input v-model="port.inputData" type="text" placeholder="Type a message..." />
      <div>
        <label for="selectIds">Select up to 6 IDs:</label>
        <Multiselect @select="handleSelect" @deselect="handleDeselect" v-model="selectedIds[index]" mode="tags"
          :searchable=true :options="uniqueIds" placeholder="Select CAN IDs" label="ID" track-by="ID"
          :show-labels="false" :max="6" />
      </div>
      <div>
        <MessageStatus ref="messageStatus" :messages="port.messages" />
      </div>
      <button @click="sendData(port, index)">Send</button>
      <button @click="closePort(port)">Close Port</button>
      <button @click="configureMask(port, index)">Configure Mask</button>
      <button @click="configureAttack(port)">Configure Attack</button>
    </div>
  </div>
</template>


<script>
import Multiselect from '@vueform/multiselect';
import MessageStatus from '@/components/MessageStatus.vue';
import Modal from '@/components/Modal.vue';
import '@vueform/multiselect/themes/default.css';
import axios from 'axios';

export default {
  components: {
    Multiselect,
    MessageStatus,
    Modal,
  },
  data() {
    return {
      serialPorts: [], // List of connected serial ports
      fileContent: "", // Content of the uploaded file
      canData: null, // Parsed CAN data from the uploaded file
      canMessages: [], // Array to store the CAN messages
      uniqueIds: [], // Unique CAN IDs from the uploaded file
      error: "", // Error message for file processing
      showModal: false,
      modalType: '',
      ignoredIDs: [],
      attack: {
        type: null,
        port: null,
      },
      replayConfig: {
        message: "",
        repeat: 1,
        interval: 0,
        startTime: 0,
      },
      simulationStartTime: null,
      mask: {
        value: '0x7FF',
        port: null,
        index: null,
      }
    };
  },
  async mounted() {
    await this.loadPreviouslyConnectedPorts();
  },
  async beforeUnmount() {
    console.log("Closing all ports...");
    this.serialPorts.forEach((portData) => {
      this.closePort(portData);
    });
  },
  methods: {
    // Load previously connected ports and establish connections
    async loadPreviouslyConnectedPorts() {
      if ("serial" in navigator) {
        try {
          const ports = await navigator.serial.getPorts();
          this.selectedIds = this.loadSelectedIds(ports.length);
          for (const port of ports) {
            this.connectPort(port);
          }
        } catch (error) {
          console.error("Error loading previously connected ports:", error);
        }
      } else {
        alert("Web Serial API is not supported in this browser.");
      }
    },

    // Connect to a specific port and retrieve its name
    async connectPort(port) {
      try {
        await port.open({ baudRate: 115200 });
        const textDecoder = new TextDecoderStream();
        const readableStreamClosed = port.readable.pipeTo(textDecoder.writable);
        const reader = textDecoder.readable.getReader();

        // Add port to the serialPorts array with console and input data
        const serialPortData = {
          port,
          reader,
          consoleOutput: "",
          inputData: "",
          deviceName: null, // Initially, device name is unknown
          buffer: "",
          messages: [],
          mask: '0x7FF',
        };
        this.serialPorts.push(serialPortData);

        // Send "get_name" to retrieve the device name
        this.sendGetNameCommand(port, serialPortData);

        // Continuously read data from the port
        while (true) {
          const { value, done } = await reader.read();
          if (done) {
            break;
          }
          if (value) {
            this.processIncomingData(port, value);
          }
        }
      } catch (error) {
        console.error("Error connecting to port:", error);
      }
    },

    // Send "get_name" command to get the device name
    async sendGetNameCommand(port, portData) {
      try {
        const textEncoder = new TextEncoder();
        const writer = port.writable.getWriter();
        await writer.write(textEncoder.encode("get_name\n"));
        writer.releaseLock();
      } catch (error) {
        console.error("Error sending get_name command:", error);
      }
    },

    // Upload the ignored IDs to all connected ECU ports
    async uploadIgnoredIDs() {
      this.serialPorts.forEach((portData, index) => {
        this.uploadIgnoredIDsToECU(index);
      });
      this.showModal = false;
    },

    // Upload the ignored IDs to the ECU
    async uploadIgnoredIDsToECU(index) {
      const ignored = this.ignoredIDs;
      // Format the message as required: "ignore:["XXX","XXX","XXX"]:end"
      const formattedMessage = `ignore:[${ignored.map(id => `"${id}"`).join(',')}]:end`;

      // Get the port for the corresponding index
      const portData = this.serialPorts[index];

      try {
        // Send the formatted message to the ECU via the serial port
        const textEncoder = new TextEncoder();
        const writer = portData.port.writable.getWriter();
        await writer.write(textEncoder.encode(formattedMessage + "\n"));
        writer.releaseLock();
        this.serialPorts[index].consoleOutput += `\nYou: ${formattedMessage}\n`;
      } catch (error) {
        console.error("Error uploading ignored IDs to ECU:", error);
      }
    },

    configureMask(port, index) {
      this.modalType = 'maskConfig';
      this.showModal = true;
      this.mask.value = port.mask;
      this.mask.port = port;
      this.mask.index = index
    },

    saveMaskValue() {
      this.serialPorts[this.mask.index].mask = this.mask.value;
      this.showModal = false;
      this.mask = {
        value: '0x7FF',
        port: null,
        index: null,
      };
    },

    // Upload the selected CAN IDs to all connected ECU ports that have selected IDs
    async uploadToECUs() {
      this.serialPorts.forEach((portData, index) => {
        if (this.selectedIds[index].length > 0) {
          this.uploadToECU(index);
        }
      });
    },

    // Method to upload the selected IDs to the ECU
    async uploadToECU(index) {
      const selected = this.selectedIds[index];
      const mask = parseInt(this.serialPorts[index].mask, 16);

      // Use the findMatchingIDs function to get the matching IDs based on the mask
      const matchingIDs = this.findMatchingIDs(selected, mask);
      
      if (matchingIDs.length === 0) {
        alert('No matching CAN IDs found.');
        return;
      }

      let ecuMessages = this.canMessages
        .filter((message) => matchingIDs.includes(message.id.toString(16).toUpperCase())) // Compare as hex strings
        .map((message) => ({ text: `${message.id}#${message.data}`, status: "unreceived", timestamp: null }));

      this.serialPorts[index].messages = ecuMessages;

      // Format the message as required: "upload:["XXX","XXX","XXX"]:end"
      const formattedMessage = `upload:{"ids":[${selected.map(id => `"${id}"`).join(',')}],"mask":${mask}}:end`;

      // Get the port for the corresponding index
      const portData = this.serialPorts[index];

      try {
        // Send the formatted message to the ECU via the serial port
        const textEncoder = new TextEncoder();
        const writer = portData.port.writable.getWriter();
        await writer.write(textEncoder.encode(formattedMessage + "\n"));
        writer.releaseLock();
        this.serialPorts[index].consoleOutput += `\nYou: ${formattedMessage}\n`;
      } catch (error) {
        console.error("Error uploading to ECU:", error);
      }
    },

    // Process incoming data and check if it contains the device name
    processIncomingData(port, value) {
      const portData = this.serialPorts.find((p) => p.port === port);
      if (value.includes("Device name: ")) {
        const deviceName = value.split("Device name: ")[1].trim();
        portData.deviceName = deviceName;
        return;
      }
      portData.consoleOutput += value;
      portData.buffer += value;

      // Check for and process complete messages
      let startIndex = portData.buffer.indexOf("recv:");
      let endIndex = portData.buffer.indexOf(":endrecv", startIndex);

      while (startIndex !== -1 && endIndex !== -1) {
        // Extract the complete message
        const completeMessage = portData.buffer.slice(startIndex, endIndex + 8); // Include ':endrecv'

        // Process the message
        this.processReceivedMessage(portData, completeMessage);

        // Remove the processed message from the buffer
        portData.buffer = portData.buffer.slice(endIndex + 8);

        // Look for the next complete message
        startIndex = portData.buffer.indexOf("recv:");
        endIndex = portData.buffer.indexOf(":endrecv", startIndex);
      }
    },

    // Process the received message and update the status
    processReceivedMessage(portData, message) {
      console.log("Received message:", message);

      // Remove 'recv:' and ':endrecv' from the message
      const cleanMessage = message.replace(/^recv:/, '').replace(/:endrecv$/, '');

      // Split the message into parts based on '#'
      const messageParts = cleanMessage.split('#');

      // Assuming the format is ID#data#timestamp, extract relevant data
      const messageContent = `${messageParts[0]}#${messageParts[1]}`;
      const timestamp = parseInt(messageParts[2], 10); // 'timestamp' part, convert to integer

      // Find the message in the portData that matches the content and is unreceived
      const messageIndex = portData.messages.findIndex((m) => m.text === messageContent && m.status === "unreceived");

      if (messageIndex !== -1) {
        // Update status and timestamp if message was found
        portData.messages[messageIndex].status = "received";
        portData.messages[messageIndex].timestamp = timestamp;
      } else {
        // Add new message with 'unexpected' status if not found
        portData.messages.push({ text: messageContent, status: "unexpected", timestamp });
      }
    },


    saveSelectedIds() {
      localStorage.setItem('selectedIds', JSON.stringify(this.selectedIds));
    },
    loadSelectedIds(length) {
      const savedIds = localStorage.getItem('selectedIds');
      return savedIds ? JSON.parse(savedIds) : Array(length).fill([]);
    },

    findMatchingIDs(filteredIDs, mask) {
      filteredIDs = filteredIDs.map(id => parseInt(id, 16)); // Convert to decimal
      console.log(filteredIDs);
      const maxID = 0x7FF; // Maximum 11-bit CAN ID (2047 in decimal)
      const matchingIDs = [];

      for (let id = 0; id <= maxID; id++) {
        for (const filteredID of filteredIDs) {
          // Apply the mask to the current ID and the filtered ID
          if ((id & mask) === (filteredID & mask)) {
            matchingIDs.push(id.toString(16).toUpperCase()); // Convert to hexadecimal
            break; // No need to check other filtered IDs for this ID
          }
        }
      }

      return matchingIDs;
    },

    ignoreIDs() {
      this.modalType = 'ignoreIds';
      this.showModal = true;
    },

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

    // Send data to the serial port
    async sendData(portData, index) {
      const { port, inputData } = portData;
      if (inputData.trim() === "") return;
      try {
        const textEncoder = new TextEncoder();
        const writer = port.writable.getWriter();
        await writer.write(textEncoder.encode(inputData + "\n"));
        writer.releaseLock();
        this.serialPorts[index].consoleOutput += "You: " + inputData;
        this.serialPorts[index].inputData = ""; // Clear input field
      } catch (error) {
        console.error("Error sending data:", error);
      }
    },

    async closePort(portData) {
      const { port, reader } = portData;

      try {
        // Cancel the reader if it exists
        if (reader) {
          try {
            console.log("Cancelling reader...");
            await reader.cancel(); // This will stop any pending read operations
            reader.releaseLock();  // Release the lock on the reader
          } catch (error) {
            console.warn("Error cancelling reader:", error);
          }
        }

        // Close the writable stream
        if (port.writable) {
          try {
            console.log("Closing writable stream...");
            const writer = port.writable.getWriter();
            writer.releaseLock();
          } catch (error) {
            console.warn("Error releasing writer lock:", error);
          }
        }

        // Finally, close the port
        await port.close();
        console.log("Port successfully closed.");
      } catch (error) {
        console.error("Error closing port:", error);
      }

      // Remove the port from the serialPorts array
      this.serialPorts = this.serialPorts.filter(p => p.port !== port);
    },

    // Upload a CANdump file
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

    // Process the uploaded file
    processFile(fileContent) {
      try {
        this.canData = this.parseCandumpFile(fileContent);
        this.sendMessagesToBackend()
        this.uniqueIds = this.canData ? Object.keys(this.canData) : [];
        console.log("CAN Data:", this.canData);
        this.error = '';
      } catch (err) {
        this.error = 'Error processing file';
        console.error(err);
      }
    },

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

          // this.canMessages.push(`${canId}#${data}`);
          this.canMessages.push({ timestamp: relativeTimestamp, id: canId, data: data });
        }
      });

      return canData; // Return the transformed object with relative timestamps
    },

    async sendMessagesToBackend() {
      const response = await axios.post('http://localhost:5000/load_can_messages', this.canMessages);
      let status = response.data.message;
      console.log('CAN messages loaded successfully:', response.data);
    },

    // Configure the attack
    configureAttack(port) {
      this.attack.port = port;
      this.modalType = 'attackConfig';
      this.showModal = true;
    },

    configureReplay() {
      // Send the replay configuration to the attack port
      const { message, repeat, interval, startTime } = this.replayConfig;
      const { port } = this.attack;
      if (!port) {
        console.error('No attack port selected.');
        return;
      }

      const textEncoder = new TextEncoder();
      const writer = port.port.writable.getWriter();

      try {
        const replayMessage = `replay:${message}:${repeat}:${interval}:${startTime}:endreplay\n`;
        writer.write(textEncoder.encode(replayMessage));
        writer.releaseLock();
        console.log(`Replay configuration sent to ${port.deviceName}`);
        this.showModal = false;
      } catch (error) {
        console.error('Error configuring replay attack:', error);
      }
    },

    async uploadToSender() {
      console.log('Uploading to Sender...');
      const senderPort = this.serialPorts.find((port) => port.deviceName === "Sender");

      if (!senderPort) {
        console.error('No port named "Sender" found.');
        return;
      }

      const textEncoder = new TextEncoder();
      const writer = senderPort.port.writable.getWriter();

      try {
        // Send data in smaller chunks
        const chunkSize = 1024; // Adjust chunk size as needed
        const jsonString = "upload:" + JSON.stringify(this.canMessages) + ":end\n";
        let offset = 0;

        while (offset < jsonString.length) {
          const chunk = jsonString.slice(offset, offset + chunkSize);
          console.log(chunk)
          await writer.write(textEncoder.encode(chunk + "\n"));
          offset += chunkSize;
        }

        console.log(`Sent messages to Sender`);
        writer.releaseLock();
      } catch (error) {
        console.error('Error getting writer for Sender:', error);
      }
    },

    // Begin the simulation by sending the CAN messages to the "Sender" port
    async beginSimulation() {
      if (this.simulationStartTime) {
        // Reset messages to unreceived state and delete unexpected messages
        this.serialPorts.forEach((port) => {
          port.messages = port.messages.filter((message) => message.status !== "unexpected");
          port.messages.forEach((message) => {
            message.status = "unreceived";
            message.timestamp = null;
          });
        });
      }

      try {
        const payload = {
          interface: 'can0',      // Default CAN interface
          bitrate: 500000         // Default bitrate
        };

        const response = await axios.post('http://localhost:5000/start_simulation', payload);
        this.simulationStatus = response.data.message;
        console.log('Simulation started successfully:', response.data);
      } catch (error) {
        console.error('Error starting simulation:', error.response?.data || error.message);
        this.simulationStatus = error.response?.data?.message || 'Failed to start simulation.';
      }

      this.simulationStartTime = Date.now();
    },
  },
};
</script>

<style>
textarea {
  width: 100%;
  margin-bottom: 10px;
}

input {
  width: calc(100% - 100px);
  margin-right: 10px;
}
</style>

<style scoped>
.attack-config {
  font-family: Arial, sans-serif;
  margin: 20px;
}

.scroll-container {
  height: 300px; /* Adjust height as needed */
  overflow-y: auto; /* Enable vertical scrolling */
  border: 1px solid #ccc; /* Optional: Add a border for visibility */
  padding: 10px; /* Optional: Add some padding */
  background-color: #f9f9f9; /* Optional: Set a background color */
}

.select-container,
.replay-config .form-group {
  margin-bottom: 15px;
}

label {
  display: block;
  margin-bottom: 5px;
  font-weight: bold;
}

.styled-select,
input {
  width: 100%;
  padding: 8px;
  border: 1px solid #ccc;
  border-radius: 4px;
}

input[type="number"] {
  -moz-appearance: textfield;
}

input[type="number"]::-webkit-outer-spin-button,
input[type="number"]::-webkit-inner-spin-button {
  -webkit-appearance: none;
  margin: 0;
}

.configure-button {
  background-color: #007bff;
  color: white;
  padding: 10px 15px;
  border: none;
  border-radius: 4px;
  cursor: pointer;
  font-size: 16px;
}

.configure-button:hover {
  background-color: #0056b3;
}
</style>
