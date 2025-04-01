<template>
  <div class="center-container">
    <!-- Loading Overlay -->
    <div v-if="loading" class="loading-overlay">
      <div class="loading-spinner"></div>
      <p>Loading...</p>
    </div>

    <div v-if="error">
      <p v-if="error === 'No file selected'">No file selected</p>
      <p v-else>{{ error }}</p>
    </div>

    <!-- Reusable Modal -->
    <Modal v-model:visible="showModal">
      <template #default>
        <div v-if="modalType === 'attackConfig'" class="attack-config">
          <h3>Message Injection Configuration</h3>
          <div class="form-group">
            <div class="input-container">
              <label for="message">
                Message (ID#DATA):
                <span class="help-icon" @mouseenter="showHelp.message = true" @mouseleave="showHelp.message = false">
                  (?)
                </span>
              </label>
              <input id="message" type="text" placeholder="Enter the message" v-model="replayConfig.message" />
              <div v-if="showHelp.message" class="help-text">
                <p>Enter the CAN message in the format: ID#DATA (e.g., 0x123#112233).</p>
              </div>
            </div>
          </div>

          <div class="form-group">
            <div class="input-container">
              <label for="repeat">
                Repeat:
                <span class="help-icon" @mouseenter="showHelp.repeat = true" @mouseleave="showHelp.repeat = false">
                  (?)
                </span>
              </label>
              <input id="repeat" type="number" min="1" placeholder="Enter the number of times"
                v-model.number="replayConfig.repeat" />
              <div v-if="showHelp.repeat" class="help-text">
                <p>Defines how many times the message should be sent. Minimum is 1.</p>
              </div>
            </div>
          </div>

          <div class="form-group">
            <div class="input-container">
              <label for="interval">
                Interval (ms):
                <span class="help-icon" @mouseenter="showHelp.interval = true" @mouseleave="showHelp.interval = false">
                  (?)
                </span>
              </label>
              <input id="interval" type="number" min="0" placeholder="Enter the interval"
                v-model.number="replayConfig.interval" />
              <div v-if="showHelp.interval" class="help-text">
                <p>Time delay (in milliseconds) between repeated messages.</p>
              </div>
            </div>
          </div>

          <div class="form-group">
            <div class="input-container">
              <label for="startTime">
                Start Time (ms):
                <span class="help-icon" @mouseenter="showHelp.startTime = true"
                  @mouseleave="showHelp.startTime = false">
                  (?)
                </span>
              </label>
              <input id="startTime" type="number" min="0" placeholder="Enter the start time"
                v-model.number="replayConfig.startTime" />
              <div v-if="showHelp.startTime" class="help-text">
                <p>Delay before the first message is sent (in milliseconds).</p>
              </div>
            </div>
          </div>

          <button type="submit" class="configure-button" @click="configureReplay">
            Configure
          </button>
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
      </template>
    </Modal>

    <!-- File Upload & Global Actions -->
    <h1>Upload CANdump File</h1>
    <div class="file-upload-container">
      <form>
        <input @change="uploadFile" type="file" />
      </form>
    </div>

    <div class="button-group">
      <button @click="ignoreIDs">Ignore IDs</button>
      <button @click="uploadToECUs">Upload IDs to ECUs</button>
      <button @click="beginSimulation">
        {{ simulationStartTime ? 'Restart' : 'Start' }} Simulation
      </button>
    </div>

    <div class="toggle-container">
      <label class="switch">
        <input type="checkbox" v-model="showMessageStatus" />
        <span class="slider round"></span>
      </label>
      <span>Show Message Status</span>
    </div>

    <h1>Serial Consoles</h1>
    <div v-if="serialPorts.length === 0">
      <p>No serial ports connected.</p>
    </div>

    <div class="grid-container">
      <div v-for="(port, index) in serialPorts" :key="port.info" class="console-section">
        <h2>
          Console {{ index + 1 }} - {{ port.deviceName || 'Unknown Device' }}
        </h2>

        <div class="button-group">
          <button @click="sendIdentify(port)">Identify</button>
          <button @click="toggleConsole(port)">
            {{ port.showConsole ? 'Hide Console' : 'Show Console' }}
          </button>
        </div>

        <!-- (2) Hide console output when showConsole is false -->
        <div v-if="port.showConsole" class="console-container">
          <textarea class="console-output" v-model="port.consoleOutput" readonly></textarea>

          <input v-model="port.inputData" type="text" placeholder="Type a message..." />

          <div class="button-group">
            <button @click="sendData(port, index)">Send</button>
            <button @click="closePort(port)">Close Port</button>
          </div>
        </div>

        <!-- CAN ID Selection -->
        <div>
          <label for="selectIds">Select up to 6 IDs:</label>
          <Multiselect @select="handleSelect" @deselect="handleDeselect" v-model="selectedIds[index]" mode="tags"
            :searchable="true" :options="uniqueIds" placeholder="Select CAN IDs" label="ID" track-by="ID"
            :show-labels="false" :max="6" />
        </div>

        <div class="button-group">
        <button @click="configureMask(port, index)">Configure Mask</button>
        <button @click="configureAttack(port)">Configure Injection</button>
      </div>

      <div class="messages-header">
        <h3>
          Messages
          <span class="help-icon" @mouseenter="showMessageHelp = index" @mouseleave="showMessageHelp = null">
            (?)
          </span>
        </h3>
      </div>

      <div v-if="showMessageHelp === index" class="help-text">
        <p>
          This section displays CAN messages received from the connected board.<br />
          If there are no messages, this section will be hidden.
        </p>
      </div>

        <div v-if="port.messages.length > 0 && showMessageStatus" class="scroll-container">
          <MessageStatus ref="messageStatus" :messages="port.messages" />
        </div>
        <div v-else-if="port.buffer.length > 0 && !showMessageStatus && !simulationRunning && simulationStartTime"
          class="scroll-container">
          <RawMessageList :buffer="port.buffer.split('\n')" :simulationOffset="simulationStartTime - port.syncSendTime"
            :millisAtSync="port.millisAtSync" />
        </div>
      </div>
    </div>
  </div>
</template>

<script>
import Multiselect from '@vueform/multiselect';
import MessageStatus from '@/components/MessageStatus.vue';
import RawMessageList from '@/components/RawMessageList.vue';
import Modal from '@/components/Modal.vue';
import '@vueform/multiselect/themes/default.css';
import axios from 'axios';
import io from 'socket.io-client';
import { useWebWorkerFn } from '@vueuse/core'

export default {
  components: {
    Multiselect,
    MessageStatus,
    RawMessageList,
    Modal,
  },
  data() {
    return {
      serialPorts: [], // List of connected serial ports
      showMessageHelp: null,
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
      showHelp: {
        message: false,
        repeat: false,
        interval: false,
        startTime: false
      },
      showMessageStatus: true,
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
      if (this.showMessageStatus) {
        await this.processReceivedMessages();
      }
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

    async sendIdentify(portData) {
      try {
        const writer = portData.port.writable.getWriter();
        const data = new TextEncoder().encode("identify\n");
        await writer.write(data);
        console.log('Sent "identify" command to port');
        writer.releaseLock();
      } catch (error) {
        console.error('Failed to send "identify" command:', error);
      }
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
          showConsole: false,
          inputData: "",
          deviceName: null, // Initially, device name is unknown
          buffer: "",
          messages: [],
          messageMap: null,
          worker: null,
          mask: '0x7FF',
          syncSendTime: null,
          syncReceiveTime: null,
          millisAtSync: null
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

      else if (value.includes("sync:")) {
        portData.writeToBuffer = true;
        const receiveTime = Date.now();
        portData.syncReceiveTime = receiveTime;
      }

      if (this.simulationRunning || portData.writeToBuffer) {
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
                const simulationOffset = portData.simulationStartTime - portData.syncSendTime;
                const adjustedTimestamp = (timestamp - portData.millisAtSync) + simulationOffset;

                // Get the indices of the message content in the message map (for faster lookup)
                let messageIndices = messageMap.get(messageContent) || [];
                let nextMessageIndex = messageIndices.shift();

                // If the message content is not found, add it as an unexpected message
                if (nextMessageIndex === undefined) {
                  messages.push({ text: messageContent, status: "unexpected", timestamp: adjustedTimestamp });
                }
                // If the message content is found, update the status and timestamp
                else {
                  messages[nextMessageIndex].status = "received";
                  messages[nextMessageIndex].timestamp = adjustedTimestamp;

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
                consoleOutput += `Unknown message format: ${message}\n`;
              }
            }
            return { messages: messages, consoleOutput: consoleOutput };
          });
          portData.worker = workerFn;
          portData.terminateWorker = terminate;
          const messagesJson = this.showMessageStatus ? JSON.stringify(portData.messages) : "[]";
          const messageMapJson = JSON.stringify(portData.messageMap ? Array.from(portData.messageMap) : null);
          const result = await portData.worker(
            {
              buffer: portData.buffer,
              messages: messagesJson,
              messageMap: messageMapJson,
              syncSendTime: portData.syncSendTime,
              millisAtSync: portData.millisAtSync,
              simulationStartTime: this.simulationStartTime,
              showMessageStatus: this.showMessageStatus
            });
          return result;
        })
      );
      this.serialPorts.forEach((portData, index) => {
        portData.messages = results[index].messages;
        portData.consoleOutput += results[index].consoleOutput;
        portData.buffer = "";
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

    async syncPortTime(portData) {
      try {
        const writer = portData.port.writable.getWriter();
        const data = new TextEncoder().encode("sync\n");
        const sendTime = Date.now();
        portData.syncSendTime = sendTime;
        await writer.write(data);
        console.log('Sent "sync" command to port');
        writer.releaseLock();
        // Loop until the buffer contains a line that starts with 'sync:' and ends with ':endsync'
        while (!portData.buffer.match(/^sync:.*:endsync$/m)) {
          if (Date.now() - sendTime > 5000) { // 5-second timeout
            console.error('Timeout: No sync message with :endsync received from port');
            return;  // Exit the function to prevent infinite loop
          }
          await new Promise(resolve => setTimeout(resolve, 10)); // Wait 10ms before checking again
        }

        // Extract the sync message
        const syncMessage = portData.buffer.match(/^sync:.*:endsync$/m);

        portData.writeToBuffer = false;
        portData.buffer = ""; // Clear the buffer after receiving the sync message

        const receiveTime = portData.syncReceiveTime;
        const roundTripTime = receiveTime - sendTime;
        const boardMillis = parseInt(syncMessage[0].split(':')[1]) - roundTripTime / 2;
        portData.millisAtSync = boardMillis;
      } catch (error) {
        console.error('Failed to send "sync" command:', error);
      }
    },

    async beginSimulation() {
      for (const portData of this.serialPorts) {
        if (this.simulationStartTime) {
          // Reset messages to unreceived state and delete unexpected messages
          portData.messages = portData.messages.filter((message) => message.status !== "unexpected");
          portData.messages.forEach((message) => {
            message.status = "unreceived";
            message.timestamp = null;
          });

          portData.syncSendTime = null;
          portData.syncReceiveTime = null;
          portData.millisAtSync = null;
        }

        await this.syncPortTime(portData);
      }

      try {
        const payload = {
          interface: 'can0',      // Default CAN interface
          bitrate: 500000         // Default bitrate
        };

        this.simulationRunning = true;
        const response = await axios.post('http://localhost:5000/start_simulation', payload);
        this.simulationStartTime = Date.now();
        this.simulationStatus = response.data.message;
        console.log('Simulation started successfully:', response.data);
      } catch (error) {
        console.error('Error starting simulation:', error.response?.data || error.message);
        this.simulationStatus = error.response?.data?.message || 'Failed to start simulation.';
        this.simulationRunning = false;
      }
    },
  },
};
</script>

<style>
/* General Layout */
.center-container {
  display: flex;
  flex-direction: column;
  align-items: center;
  background: white;
  padding: 20px;
  border-radius: 8px;
  max-width: 800px;
  margin: 0 auto;
}

.form-group {
  display: flex;
  flex-direction: column;
  margin-bottom: 1rem;
}

.form-group label {
  border: 1px solid #ccc;
  border-radius: 4px;
  width: 100%
}

.button-group {
  display: flex;
  flex-wrap: wrap;
  justify-content: center;
  margin-bottom: 10px;
  gap: 10px;
  margin-top: 10px;
}

.button-group button {
  padding: 8px 12px;
  font-size: 14px;
  border-radius: 5px;
  background-color: #f8f9fa;
  border: 1px solid #ccc;
  cursor: pointer;
  transition: background 0.2s ease-in-out;
}

.button-group button:hover {
  background-color: #e0e0e0;
}

/* Loading Overlay */
.loading-overlay {
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background-color: rgba(0, 0, 0, 0.5);
  display: flex;
  justify-content: center;
  align-items: center;
  color: white;
  font-size: 20px;
  z-index: 9999;
}

.center-container {
  display: flex;
  flex-direction: column;
  align-items: center;
  background: white;
  padding: 20px;
  border-radius: 8px;
  max-width: 1500px;
  margin: 0 auto;
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

@keyframes spin {
  0% {
    transform: rotate(0deg);
  }

  100% {
    transform: rotate(360deg);
  }
}

/* Help Icons and Text */
.help-icon {
  cursor: pointer;
  margin-left: 6px;
  color: #555;
  font-size: 12px;
  background: transparent;
  padding: 2px;
  border-radius: 50%;
  display: inline-block;
  text-align: center;
  font-weight: normal;
  font-size: 0.9rem;
  vertical-align: middle;
  transition: color 0.1s ease-in-out;
}

.help-icon:hover {
  color: #007bff;
}

.help-text {
  background: #fff9c4;
  padding: 8px 12px;
  border: 1px solid #ffd54f;
  border-radius: 4px;
  max-width: 800px;
  font-size: 14px;
  color: #333;
  position: absolute;
  z-index: 10;
  box-shadow: 2px 2px 5px rgba(0, 0, 0, 0.3);
  text-align: left;
  line-height: 1.4rem;
  transition: none !important;
  animation: none !important;
}

/* Console Section */
.console-section {
  background: #f9f9f9;
  padding: 15px;
  border-radius: 10px;
  width: 100%;
  margin-top: 20px;
  box-shadow: 0px 4px 8px rgba(0, 0, 0, 0.1);
}

.console-container {
  display: flex;
  flex-direction: column;
  align-items: center;
  background: black;
  color: limegreen;
  padding: 15px;
  border-radius: 8px;
  font-family: monospace;
  width: 100%;
  max-width: 600px;
  text-align: left;
  margin-top: 10px;
}

.console-output {
  width: 100%;
  background-color: #121212;
  color: #ffffff;
  padding: 10px;
  max-width: 500px;
  border-radius: 5px;
  text-align: left;
  font-family: monospace;
  resize: none;
  margin-bottom: 10px;
  height: 200px;
  border: none;
}

/* Messages List */
.messages-header {
  display: flex;
  justify-content: center;
  align-items: center;
  gap: 6px;
  margin-top: 8px;
}

.file-upload-container {
  display: flex;
  align-items: center;
  justify-content: center;
  margin: 20px 0;
  /* Adjust vertical spacing as needed */
}

.file-upload-container input[type="file"] {
  padding: 10px;
  border: 1px solid #ccc;
  border-radius: 4px;
}

.grid-container {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 20px;
  width: -webkit-fill-available;
  width: 100%;
}

.scroll-container {
  height: 300px;
  overflow-y: auto;
  border: 1px solid #ccc;
  padding: 10px;
  background-color: #f9f9f9;
}

.attack-config {
  font-family: Arial, sans-serif;
  margin: 20px;
}

/* Remove borders from labels and center them */
.attack-config .form-group label {
  border: none;
  font-weight: bold;
  text-align: center;
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

.configure-button {
  display: inline-block;
  background-color: #007bff;
  color: white;
  padding: 8px 16px;
  border-radius: 4px;
  cursor: pointer;
  font-size: 1rem;
  border: none;
}

.configure-button:hover {
  background-color: #0056b3;
}

/* Form Inputs */
textarea {
  width: 100%;
  margin-bottom: 10px;
}

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

/* Toggle Switch */
.toggle-container {
  display: flex;
  align-items: center;
  margin-bottom: 10px;
}

.switch {
  position: relative;
  display: inline-block;
  width: 34px;
  height: 20px;
  margin-right: 10px;
}

.switch input {
  opacity: 0;
  width: 0;
  height: 0;
}

.slider {
  position: absolute;
  cursor: pointer;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: #ccc;
  transition: 0.4s;
  border-radius: 20px;
}

.slider:before {
  position: absolute;
  content: "";
  height: 14px;
  width: 14px;
  left: 3px;
  bottom: 3px;
  background-color: white;
  transition: 0.4s;
  border-radius: 50%;
}

input:checked+.slider {
  background-color: hsla(160, 100%, 37%, 1);
}

input:checked+.slider:before {
  transform: translateX(14px);
}
</style>