<template>
  <div>
    <!-- Loading Overlay -->
    <div v-if="loading" class="loading-overlay">
      <div class="loading-spinner"></div>
      <p>Loading...</p>
    </div>
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
          <!-- buttons to reapply saved sets-->
          <div id="saved-sets">
            <h3>Saved CAN ID Sets</h3>
            <button v-for="(set, index) in savedIgnoredIDs" :key="index" type="button" @click="selectIgnoredId(set)">
              {{ set.name }}
            </button>
          </div>
          <p>Enter the CAN IDs you want to ignore (in hexadecimal format):</p>
          <Multiselect ref="ignoredIDsMultiselect" v-model="ignoredIDs" :allowAbsent="true" mode="tags"
            :searchable="true" placeholder="Enter CAN IDs" :show-labels="false" :taggable="true"
            :addTagOn="['enter', 'space', ';', ',']" :showOptions="false" :createTag="true" @focusout="handleBlur" />

          <!-- space for user to add name of CAN ID set-->
          <input v-model="arrayName" placeholder="Enter a name for this set of ID's" />
          <button @click="saveIgnoredIDs">Save Set</button>
          <button @click="uploadIgnoredIDs">Upload</button>
        </div>

        <div v-else-if="modalType == 'maskConfig'">
          <h2>Configure Mask</h2>
          <p>Enter the mask value (in hexadecimal format):</p>
          <input type="text" v-model="mask.value" placeholder="Enter the mask value" />
          <p>Matching IDs:</p>
          <div class="scroll-container">
            <ul>
              <li v-for="id in findMatchingIDs(selectedIds[mask.index], parseInt(mask.value, 16))" :key="id">{{ id }}
              </li>
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
      <!-- Identify Button-->
      <button @click="sendIdentify(port)">Identify</button>
         <!-- Toggle Console Button-->
          <button @click="toggleConsole(port)">
            {{ port.showConsole ? 'Hide Console' : 'Show Console' }}
          </button>
          <!-- Console hidden by default-->
           <div v-if="port.showConsole" class="console-output">
            <textarea v-model="port.consoleOutput" rows ="10" cols="50" readonly></textarea>
           </div>

      <input v-model="port.inputData" type="text" placeholder="Type a message..." />
      <div>
        <label for="selectIds">Select up to 6 IDs:</label>
        <Multiselect @select="handleSelect" @deselect="handleDeselect" v-model="selectedIds[index]" mode="tags"
          :searchable="true" :options="uniqueIds" placeholder="Select CAN IDs" label="ID" track-by="ID"
          :show-labels="false" :max="6" />
      </div>
        <div class="help-container">
        <h3>Messages
            <span
            class="help-icon"
            @mouseenter="showMessageHelp = true"
            @mouseleave="showMessageHelp = false"
            >
            (?)
            </span>
        </h3>
        <!--Help Text-->
        <div v-if="showMessageHelp" class="help-text">
             <p>
                 This section displays CAN messages received from the connected board. <br>
                 If there are no messages, this section will be hidden.
            </p>
        </div>
        </div>

      <div v-if="port.messages.lenght > 0" class="scroll-container">
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
import SerialConnect from '@/views/SerialConnect.vue';
import Multiselect from '@vueform/multiselect';
import MessageStatus from '@/components/MessageStatus.vue';
import Modal from '@/components/Modal.vue';
import '@vueform/multiselect/themes/default.css';
import axios from 'axios';
import io from 'socket.io-client';
import { useWebWorkerFn } from '@vueuse/core'

export default {
  components: {
    Multiselect,
    MessageStatus,
    Modal,
  },
  data() {
    return {
        serialPorts: [ //Simulated Serial conneciton
            {
                info: "Simulated Port 1",
                deviceName: "Virtual COM1",
                consoleOutput: "Simulated console output...",
                inputData: "",
                messages: [],
                showConsole: false,            }
      ], // List of connected serial ports
      showMessageHelp: false,      
      fileContent: "", // Content of the uploaded file
      canData: null, // Parsed CAN data from the uploaded file
      canMessages: [], // Array to store the CAN messages
      uniqueIds: [], // Unique CAN IDs from the uploaded file
      error: "", // Error message for file processing
      selectedIds: [], // Selected CAN IDs for each serial port
      showModal: false,
      loading: false,
      modalType: '',
      ignoredIDs: [],
      arrayName: '',
      savedIgnoredIDs: [],
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
      simulationRunning: false,
      mask: {
        value: '0x7FF',
        port: null,
        index: null,
      },
    };
  },
  async mounted() {
    await this.loadPreviouslyConnectedPorts();
    // Load any previously saved ignored ID sets from local storage
    this.loadSavedIgnoredIDs();
  },
  created() {
    this.socket = io('http://localhost:5000');
    this.socket.on('simulation_complete', async () => {
      console.log('Simulation complete.');
      this.simulationRunning = false;
      this.loading = true;
      await this.processReceivedMessages();
      this.loading = false;
    });
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

    sendIdentify(port) {
      SerialConnect.methods.sendIdentify.call(this,port);
    },

    toggleConsole(port) {
        if (!port.hasOwnProperty('showConsole')) {
         this.$set(port, 'showConsole', false); // Ensure reactivity
        }
        port.showConsole = !port.showConsole;
      },

  
    handleBlur() {
      const multiselect = this.$refs.ignoredIDsMultiselect;
      if (multiselect) {
        const inputEl = multiselect.$el.querySelector('input');
        if (inputEl && inputEl.value.trim() !== '') {
          const newTag = inputEl.value.trim();
          this.ignoredIDs.push(newTag);
          inputEl.value = '';
        }
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
          messageMap: null,
          worker: null,
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

    saveIgnoredIDs() {
      if (this.arrayName.trim() === '') {
        alert('Please enter a name for the array.');
        return;
      }
      if (this.ignoredIDs.length === 0) {
        alert('Please add at least one CAN ID');
        return;
      }
      // Here, ignoredIDs is an array of objects (each like { ID: "ABC123" })
      const newSet = {
        name: this.arrayName.trim(),
        ids: this.ignoredIDs.slice()  // clone the array
      };
      let savedArrays = JSON.parse(localStorage.getItem('savedArrays') || '[]');
      savedArrays.push(newSet);
      localStorage.setItem('savedArrays', JSON.stringify(savedArrays));
      this.savedIgnoredIDs = savedArrays;
      console.log("Saved new set. Current savedIgnoredIDs:", this.savedIgnoredIDs);
      // Clear inputs
      this.arrayName = '';
      this.ignoredIDs = [];
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

    loadSavedIgnoredIDs() {
      let savedArrays = JSON.parse(localStorage.getItem('savedArrays') || '[]');
      if (!Array.isArray(savedArrays)) {
        savedArrays = [];
      }
      this.savedIgnoredIDs = savedArrays;
    },

    selectIgnoredId(savedSet) {
      // Set ignoredIDs to the saved array of strings.
      this.ignoredIDs = savedSet.ids;
      this.arrayName = savedSet.name;
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
      this.loading = true;
      await new Promise(resolve => setTimeout(resolve, 0));
      await Promise.all(
        this.serialPorts.map((portData, index) => {
          if (this.selectedIds[index].length > 0) {
            return this.uploadToECU(index); // Return the promise
          }
          return Promise.resolve(); // No-op for ports without selected IDs
        })
      );
      this.loading = false;
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

      let ecuMessages = [];
      let messageMap = new Map();

      this.canMessages
        .filter((message) => matchingIDs.includes(message.id.toString(16).toUpperCase())) // Compare as hex strings
        .forEach((message, index) => {
          const text = `${message.id}#${message.data}`;
          const entry = { text, status: "unreceived", timestamp: null };

          ecuMessages.push(entry);

          // Maintain an ordered list of indices for each message content
          if (!messageMap.has(text)) {
            messageMap.set(text, []);
          }
          messageMap.get(text).push(index); // Store index to support duplicates
        });

      this.serialPorts[index].messages = ecuMessages;
      this.serialPorts[index].messageMap = messageMap;

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

      if (this.simulationRunning) {
        portData.buffer += value;
      }
      else {
        portData.consoleOutput += value;
      }
    },

    async processReceivedMessages() {
      const results = await Promise.all(
        this.serialPorts.map(async (portData) => {
          const { workerFn, terminate } = useWebWorkerFn((portData) => {
            const messages = JSON.parse(portData.messages);
            const messageMap = portData.messageMap ? new Map(JSON.parse(portData.messageMap)) : new Map();
            let consoleOutput = "";

            const lines = portData.buffer.split('\n');

            // Process each line sequentially to maintain the order within the serial port
            for (const line of lines) {
              const message = line.trim();

              // Check if the line starts with 'recv:' and ends with ':endrecv'
              if (message.startsWith('recv:') && message.endsWith(':endrecv')) {
                if (!message.includes('#')) {
                  consoleOutput += `Malformed message: ${message}\n`;
                  continue;
                }

                // Remove 'recv:' and ':endrecv' from the message
                const cleanMessage = message.replace(/^recv:/, '').replace(/:endrecv$/, '');
                const [id, data, timestampStr] = cleanMessage.split('#');
                if (!id || !data || isNaN(timestampStr)) {
                  consoleOutput += `Invalid message format: ${message}\n`;
                  continue;
                }

                const messageContent = `${id}#${data}`;
                const timestamp = parseInt(timestampStr, 10);

                // Get the indices of the message content in the message map (for faster lookup)
                let messageIndices = messageMap.get(messageContent) || [];
                let nextMessageIndex = messageIndices.shift();

                // If the message content is not found, add it as an unexpected message
                if (nextMessageIndex === undefined) {
                  messages.push({ text: messageContent, status: "unexpected", timestamp });
                }
                // If the message content is found, update the status and timestamp
                else {
                  messages[nextMessageIndex].status = "received";
                  messages[nextMessageIndex].timestamp = timestamp;

                  // Update the message map with the remaining indices
                  if (messageIndices.length > 0) {
                    messageMap.set(messageContent, messageIndices);
                  }
                  // If there are no more indices, delete the message content from the map
                  else {
                    messageMap.delete(messageContent);
                  }
                }

              } else {
                consoleOutput += `${message}\n`;
              }
            }
            portData.buffer = '';
            return { messages: messages, consoleOutput: consoleOutput };
          });
          portData.worker = workerFn;
          portData.terminateWorker = terminate;
          const messagesJson = JSON.stringify(portData.messages);
          const messageMapJson = JSON.stringify(portData.messageMap ? Array.from(portData.messageMap) : null);
          const result = await portData.worker({ buffer: portData.buffer, messages: messagesJson, messageMap: messageMapJson });
          return result;
        })
      );
      this.serialPorts.forEach((portData, index) => {
        portData.messages = results[index].messages;
        portData.consoleOutput += results[index].consoleOutput;
      });
      console.log('Received messages processed.');
    },

    saveSelectedIds() {
      this.serialPorts.forEach((portData, index) => {
        localStorage.setItem(`selectedIds-${portData.deviceName}`, JSON.stringify(this.selectedIds[index]));
      });
    },
    loadSelectedIds(length) {
      // Load selected IDs from local storage based on the device name
      return Array.from({ length }, (_, index) => {
        const deviceName = this.serialPorts[index].deviceName;
        const savedIds = localStorage.getItem(`selectedIds-${deviceName}`);
        return savedIds ? JSON.parse(savedIds) : [];
      });
    },

    findMatchingIDs(filteredIDs, mask) {
      filteredIDs = filteredIDs.map(id => parseInt(id, 16)); // Convert to decimal
      const maxID = 0x7FF; // Maximum 11-bit CAN ID (2047 in decimal)
      const matchingIDs = [];

      for (let id = 0; id <= maxID; id++) {
        for (const filteredID of filteredIDs) {
          // Apply the mask to the current ID and the filtered ID
          if ((id & mask) === (filteredID & mask)) {
            matchingIDs.push(id.toString(16).toUpperCase().padStart(3, '0'));
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

      this.loading = true;
      this.selectedIds = this.loadSelectedIds(this.serialPorts.length);

      const reader = new FileReader();
      reader.onload = (e) => {
        this.fileContent = e.target.result;
        this.processFile(this.fileContent);
        this.loading = false;
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
        this.loading = false;
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

        this.simulationRunning = true;
        const response = await axios.post('http://localhost:5000/start_simulation', payload);
        this.simulationStatus = response.data.message;
        console.log('Simulation started successfully:', response.data);
      } catch (error) {
        console.error('Error starting simulation:', error.response?.data || error.message);
        this.simulationStatus = error.response?.data?.message || 'Failed to start simulation.';
        this.simulationRunning = false;
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
  height: 300px;
  /* Adjust height as needed */
  overflow-y: auto;
  /* Enable vertical scrolling */
  border: 1px solid #ccc;
  /* Optional: Add a border for visibility */
  padding: 10px;
  /* Optional: Add some padding */
  background-color: #f9f9f9;
  /* Optional: Set a background color */
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

/* Overlay style */
.loading-overlay {
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background-color: rgba(0, 0, 0, 0.5);
  /* Semi-transparent black background */
  display: flex;
  justify-content: center;
  align-items: center;
  color: white;
  font-size: 20px;
  z-index: 9999;
  /* Ensure it's above all other content */
}

.loading-spinner {
  border: 4px solid rgba(255, 255, 255, 0.3);
  border-top: 4px solid white;
  border-radius: 50%;
  width: 50px;
  height: 50px;
  margin-right: 10px;
  animation: spin 1s linear infinite;
}

/* Adjust spacing and alignment of (?) icon */
.help-icon {
  cursor: pointer;
  margin-left: 6px;
  color: #333;
  font-size: 0.9em;
  background: #e0e0e0; /* Light gray background */
  padding: 1px 6px;
  border-radius: 8px; 
  display: inline-block;
  text-align: center;
  font-weight: bold;
  vertical-align: middle; 
}


.help-text {
  background: #fff9c4;
  padding: 8px 12px;
  margin-top: 5px;
  border: 1px solid #ffd54f;
  border-radius: 4px;
  width: 500px;
  max-width: 1000px;
  font-size: 14px;
  color: #333;
  position: absolute;
  z-index: 10;
  box-shadow: 2px 2px 5px rgba(0, 0, 0, 0.3);
  text-align: left; 
  line-height: 1.4; 
}


.help-container {
  position: relative;
  display: inline-block;
}

  
@keyframes spin {
  0% {
    transform: rotate(0deg);
  }

  100% {
    transform: rotate(360deg);
  }
}
</style>
