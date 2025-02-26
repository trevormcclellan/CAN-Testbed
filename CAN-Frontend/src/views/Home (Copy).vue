<template>
  <div class="center-container">
    <!-- Loading Overlay -->
    <div v-if="loading" class="loading-overlay">
      <div class="loading-spinner"></div>
      <p>Loading...</p>
    </div>

    <!-- Reusable Modal -->
    <Modal v-model:visible="showModal">
      <template #default>
        <!-- Attack Config Modal -->
        <div v-if="modalType === 'attackConfig'" class="attack-config">
          <h3>Configure Injection</h3>
          <div class="form-group">
            <label for="message">
              Message (ID#DATA):
              <span class="help-icon"
                    @mouseenter="showHelp.message = true"
                    @mouseleave="showHelp.message = false">
                (?)
              </span>
            </label>
            <input
              id="message"
              type="text"
              placeholder="Enter the message"
              v-model="replayConfig.message"
            />
            <div v-if="showHelp.message" class="help-text">
              <p>Enter the CAN message in the format: ID#DATA (e.g., 0x123#112233).</p>
            </div>
          </div>

          <div class="form-group">
            <label for="repeat">
              Repeat:
              <span class="help-icon"
                    @mouseenter="showHelp.repeat = true"
                    @mouseleave="showHelp.repeat = false">
                (?)
              </span>
            </label>
            <input
              id="repeat"
              type="number"
              min="1"
              placeholder="Enter the number of times"
              v-model.number="replayConfig.repeat"
            />
            <div v-if="showHelp.repeat" class="help-text">
              <p>Defines how many times the message should be sent. Minimum is 1.</p>
            </div>
          </div>

          <div class="form-group">
            <label for="interval">
              Interval (ms):
              <span class="help-icon"
                    @mouseenter="showHelp.interval = true"
                    @mouseleave="showHelp.interval = false">
                (?)
              </span>
            </label>
            <input
              id="interval"
              type="number"
              min="0"
              placeholder="Enter the interval"
              v-model.number="replayConfig.interval"
            />
            <div v-if="showHelp.interval" class="help-text">
              <p>Time delay (in milliseconds) between repeated messages.</p>
            </div>
          </div>

          <div class="form-group">
            <label for="startTime">
              Start Time (ms):
              <span class="help-icon"
                    @mouseenter="showHelp.startTime = true"
                    @mouseleave="showHelp.startTime = false">
                (?)
              </span>
            </label>
            <input
              id="startTime"
              type="number"
              min="0"
              placeholder="Enter the start time"
              v-model.number="replayConfig.startTime"
            />
            <div v-if="showHelp.startTime" class="help-text">
              <p>Delay before the first message is sent (in milliseconds).</p>
            </div>
          </div>

          <button type="submit" class="configure-button" @click="configureReplay">
            Configure
          </button>
        </div>

        <!-- Mask Configuration Modal -->
        <div v-else-if="modalType === 'maskConfig'">
          <h3>Configure Mask</h3>
          <input
            type="text"
            v-model="mask.value"
            placeholder="e.g. 0x7FF"
          />
          <button class="configure-button" @click="saveMaskValue">
            Save
          </button>
        </div>

        <!-- Ignore IDs Modal -->
        <div v-else-if="modalType === 'ignoreIds'">
          <h3>Ignore IDs</h3>
          <!-- Add or select existing sets -->
          <div class="form-group">
            <label>Name this set of ignored IDs:</label>
            <input v-model="arrayName" type="text" placeholder="Ignored set name"/>
          </div>

          <div class="form-group">
            <label>Enter IDs to ignore (Hex, e.g. 0x123):</label>
            <Multiselect
              v-model="ignoredIDs"
              tag-placeholder="Add ID..."
              mode="tags"
              placeholder="Ignored IDs"
              :searchable="true"
              :options="[]"
              @blur="handleBlur"
            />
          </div>

          <!-- Previously saved sets -->
          <div class="form-group">
            <label>Load saved sets:</label>
            <select @change="selectIgnoredId($event.target.value)">
              <option disabled value="">Select a saved set</option>
              <option v-for="(set, i) in savedIgnoredIDs" :key="i" :value="set.name">
                {{ set.name }}
              </option>
            </select>
          </div>

          <button class="configure-button" @click="saveIgnoredIDs">
            Save IDs
          </button>
          <button class="configure-button" @click="uploadIgnoredIDs">
            Upload IDs to All ECUs
          </button>
        </div>
      </template>
    </Modal>

    <!-- File Upload and Global Actions -->
    <h1>Upload CANdump File</h1>
    <form>
      <input @change="uploadFile" type="file" />
    </form>

    <div class="button-group">
      <button @click="ignoreIDs">Ignore IDs</button>
      <button @click="uploadToECUs">Upload IDs to ECUs</button>
      <button @click="beginSimulation">
        {{ simulationStartTime ? 'Restart' : 'Start' }} Simulation
      </button>
    </div>

    <h1>Serial Consoles</h1>
    <div v-if="serialPorts.length === 0">
      <p>No serial ports connected.</p>
    </div>

    <!-- Console Sections -->
    <div
      v-for="(port, index) in serialPorts"
      :key="port.info"
      class="console-section"
    >
      <h2>
        Console {{ index + 1 }} - {{ port.deviceName || 'Unknown Device' }}
      </h2>

      <div class="button-group">
        <button @click="sendIdentify(port)">Identify</button>
        <button @click="toggleConsole(port)">
          {{ port.showConsole ? 'Hide Console' : 'Show Console' }}
        </button>
      </div>

      <div class="console-container">
        <textarea
          class="console-output"
          v-model="port.consoleOutput"
          readonly
        ></textarea>

        <input
          v-if="port.showConsole"
          v-model="port.inputData"
          type="text"
          placeholder="Type a message..."
        />

        <div class="console-buttons">
          <button @click="sendData(port, index)">Send</button>
          <button @click="closePort(port)">Close Port</button>
        </div>
      </div>

      <!-- CAN ID Selection -->
      <div>
        <label for="selectIds">Select up to 6 IDs:</label>
        <Multiselect
          @select="handleSelect"
          @deselect="handleDeselect"
          v-model="selectedIds[index]"
          mode="tags"
          :searchable="true"
          :options="uniqueIds"
          placeholder="Select CAN IDs"
          label="ID"
          track-by="ID"
          :show-labels="false"
          :max="6"
        />
      </div>

      <div class="button-group">
        <button @click="configureMask(port, index)">Configure Mask</button>
        <button @click="configureAttack(port)">Configure Injection</button>
      </div>

      <div class="messages-header">
        <h3>
          Messages
          <span
            class="help-icon"
            @mouseenter="showMessageHelp = index"
            @mouseleave="showMessageHelp = null"
          >
            (?)
          </span>
        </h3>
      </div>

      <!-- Help Text for messages -->
      <div v-if="showMessageHelp === index" class="help-text">
        <p>
          This section displays CAN messages received from the connected board.<br />
          If there are no messages, this section will be hidden.
        </p>
      </div>

      <!-- Messages Box -->
      <div v-if="port.messages.length > 0" class="scroll-container">
        <ul>
          <li v-for="message in port.messages" :key="message.id + message.timestamp">
            <strong>{{ message.id }}:</strong> {{ message.data }}
          </li>
        </ul>
      </div>
    </div>
  </div>
</template>

<script>
/* External imports */
import Multiselect from "@vueform/multiselect";
import "@vueform/multiselect/themes/default.css";
import axios from "axios";
import io from "socket.io-client";
import { useWebWorkerFn } from "@vueuse/core";

/* Local imports */
import MessageStatus from "@/components/MessageStatus.vue";
import Modal from "@/components/Modal.vue";

export default {
  components: {
    Multiselect,
    MessageStatus,
    Modal,
  },
  data() {
    return {
      /* ----- Primary State ----- */
      serialPorts: [
        // Example Simulated Serial Port
        {
          info: "Simulated Port 1",
          buffer: "",
          deviceName: "Virtual COM1",
          consoleOutput: "Simulated console output...",
          inputData: "",
          messages: [],
          showConsole: false,
        },
      ],
      fileContent: "",
      canData: null,
      canMessages: [],
      uniqueIds: [],

      /* ----- UI / Modal State ----- */
      showModal: false,
      loading: false,
      modalType: "",
      showMessageHelp: null,
      showHelp: {
        message: false,
        repeat: false,
        interval: false,
        startTime: false,
      },

      /* ----- IDs and Ignored/Mask Data ----- */
      ignoredIDs: [],
      arrayName: "",
      savedIgnoredIDs: [],
      selectedIds: [], // Array of arrays, one for each port
      mask: {
        value: "0x7FF",
        port: null,
        index: null,
      },

      /* ----- Attack / Simulation Data ----- */
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

      /* ----- Error Handling ----- */
      error: "",

      /* ----- Socket ----- */
      socket: null,
    };
  },
  async mounted() {
    await this.loadPreviouslyConnectedPorts();
    this.loadSavedIgnoredIDs();
  },
  created() {
    this.socket = io("http://localhost:5000");
    this.socket.on("simulation_complete", async () => {
      console.log("Simulation complete.");
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
    /* -------------------------------------------------------------------------- */
    /*                               Port Management                               */
    /* -------------------------------------------------------------------------- */

    /**
     * Attempt to load previously connected ports
     * and reconnect them automatically (if user has granted permission).
     */
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

    /**
     * Connect to a SerialPort object, open it, and set up its reader and writer.
     * @param {SerialPort} port
     */
    async connectPort(port) {
      try {
        await port.open({ baudRate: 115200 });
        const textDecoder = new TextDecoderStream();
        const readableStreamClosed = port.readable.pipeTo(textDecoder.writable);
        const reader = textDecoder.readable.getReader();

        // Add the port to state
        const serialPortData = {
          port,
          reader,
          consoleOutput: "",
          showConsole: false,
          inputData: "",
          buffer: "",
          messages: [],
          messageMap: null,
          worker: null,
          mask: "0x7FF",
          deviceName: null,
        };
        this.serialPorts.push(serialPortData);

        // Attempt to query the device name
        this.sendGetNameCommand(port, serialPortData);

        // Continuously read data
        while (true) {
          const { value, done } = await reader.read();
          if (done) break;
          if (value) {
            this.processIncomingData(port, value);
          }
        }
      } catch (error) {
        console.error("Error connecting to port:", error);
      }
    },

    /**
     * Close a port and remove it from serialPorts.
     * @param {Object} portData
     */
    async closePort(portData) {
      const { port, reader } = portData;
      try {
        if (reader) {
          try {
            await reader.cancel();
            reader.releaseLock();
          } catch (error) {
            console.warn("Error cancelling reader:", error);
          }
        }
        if (port.writable) {
          try {
            const writer = port.writable.getWriter();
            writer.releaseLock();
          } catch (error) {
            console.warn("Error releasing writer lock:", error);
          }
        }
        await port.close();
        console.log("Port successfully closed.");
      } catch (error) {
        console.error("Error closing port:", error);
      }
      this.serialPorts = this.serialPorts.filter((p) => p.port !== port);
    },

    /**
     * Process incoming string data from the serial port.
     * @param {SerialPort} port
     * @param {string} value
     */
    processIncomingData(port, value) {
      const portData = this.serialPorts.find((p) => p.port === port);
      // If name was requested, parse it
      if (value.includes("Device name: ")) {
        const deviceName = value.split("Device name: ")[1].trim();
        portData.deviceName = deviceName;
        return;
      }

      // During simulation, accumulate data in the buffer
      if (this.simulationRunning) {
        portData.buffer += value;
      } else {
        // Otherwise just append to console
        portData.consoleOutput += value;
      }
    },

    /**
     * Issue a "get_name" command to retrieve the board's device name.
     */
    async sendGetNameCommand(port, portData) {
      try {
        await this.sendCommandToPort(portData, "get_name");
      } catch (error) {
        console.error("Error sending get_name command:", error);
      }
    },

    /**
     * Toggle console visibility for a given port object.
     */
    toggleConsole(port) {
      if (!port.hasOwnProperty("showConsole")) {
        this.$set(port, "showConsole", false);
      }
      port.showConsole = !port.showConsole;
    },

    /* -------------------------------------------------------------------------- */
    /*                              Console Utilities                              */
    /* -------------------------------------------------------------------------- */

    /**
     * Send a plain string command to a port.
     * @param {Object} portData
     * @param {string} command
     */
    async sendCommandToPort(portData, command) {
      if (!portData?.port?.writable) return;
      try {
        const textEncoder = new TextEncoder();
        const writer = portData.port.writable.getWriter();
        await writer.write(textEncoder.encode(command + "\n"));
        writer.releaseLock();
        this.appendConsoleOutput(portData, `You: ${command}\n`);
      } catch (error) {
        console.error(`Failed to send command "${command}":`, error);
      }
    },

    /**
     * Safely append console text to a port's consoleOutput.
     */
    appendConsoleOutput(portData, text) {
      if (!portData.consoleOutput.endsWith("\n")) {
        portData.consoleOutput += "\n";
      }
      portData.consoleOutput += text;
    },

    /**
     * Send arbitrary user data from input field to the serial port.
     */
    async sendData(portData, index) {
      const { port, inputData } = portData;
      if (inputData.trim() === "") return;
      try {
        await this.sendCommandToPort(portData, inputData);
        this.serialPorts[index].inputData = "";
      } catch (error) {
        console.error("Error sending data:", error);
      }
    },

    /**
     * Send "identify" command to the given port.
     */
    async sendIdentify(portData) {
      await this.sendCommandToPort(portData, "identify");
    },

    /* -------------------------------------------------------------------------- */
    /*                                 Mask Config                                 */
    /* -------------------------------------------------------------------------- */

    configureMask(port, index) {
      this.modalType = "maskConfig";
      this.showModal = true;
      this.mask.value = port.mask;
      this.mask.port = port;
      this.mask.index = index;
    },

    /**
     * Save the mask value from the modal to the specified port
     */
    saveMaskValue() {
      if (this.mask.port) {
        const { port, index, value } = this.mask;
        this.serialPorts[index].mask = value;
      }
      this.resetMaskModal();
    },

    /**
     * Reset mask object and close modal
     */
    resetMaskModal() {
      this.showModal = false;
      this.mask = {
        value: "0x7FF",
        port: null,
        index: null,
      };
    },

    /* -------------------------------------------------------------------------- */
    /*                                Ignore IDs                                   */
    /* -------------------------------------------------------------------------- */

    ignoreIDs() {
      this.modalType = "ignoreIds";
      this.showModal = true;
    },

    /**
     * Handle blur on the Multiselect input to add a new tag if needed.
     */
    handleBlur() {
      // Attempt to read the text in the multiselect input
      const multiselect = this.$refs.ignoredIDsMultiselect;
      if (!multiselect) return;
      const inputEl = multiselect.$el.querySelector("input");
      if (inputEl && inputEl.value.trim() !== "") {
        const newTag = inputEl.value.trim();
        this.ignoredIDs.push(newTag);
        inputEl.value = "";
      }
    },

    /**
     * Upload ignored IDs to all ports.
     */
    uploadIgnoredIDs() {
      this.serialPorts.forEach((_, index) => {
        this.uploadIgnoredIDsToECU(index);
      });
      this.showModal = false;
    },

    /**
     * Format and send the "ignore" message to the ECU for a given port index.
     */
    async uploadIgnoredIDsToECU(index) {
      const portData = this.serialPorts[index];
      const formattedMessage = this.formatIgnoredIDsMessage(this.ignoredIDs);
      await this.sendCommandToPort(portData, formattedMessage);
    },

    /**
     * Format the "ignore" command with an array of IDs.
     */
    formatIgnoredIDsMessage(ids) {
      // Example: ignore:["0x123","0x300"]:end
      const payload = ids.map((id) => `"${id}"`).join(",");
      return `ignore:[${payload}]:end`;
    },

    /**
     * Save the current ignored IDs to local storage with the user-specified name.
     */
    saveIgnoredIDs() {
      if (!this.arrayName.trim()) {
        alert("Please enter a name for the array.");
        return;
      }
      if (!this.ignoredIDs.length) {
        alert("Please add at least one CAN ID.");
        return;
      }
      const newSet = {
        name: this.arrayName.trim(),
        ids: this.ignoredIDs.slice(),
      };
      let savedArrays = this.loadIgnoredIDsFromStorage();
      savedArrays.push(newSet);
      this.saveIgnoredIDsToStorage(savedArrays);

      this.savedIgnoredIDs = savedArrays;
      this.arrayName = "";
      this.ignoredIDs = [];
    },

    /**
     * Handle selection of a saved set from the dropdown.
     */
    selectIgnoredId(selectedName) {
      const foundSet = this.savedIgnoredIDs.find((set) => set.name === selectedName);
      if (foundSet) {
        this.ignoredIDs = foundSet.ids;
        this.arrayName = foundSet.name;
      }
    },

    /**
     * Save an array of ignored ID sets to local storage.
     */
    saveIgnoredIDsToStorage(arrays) {
      localStorage.setItem("savedArrays", JSON.stringify(arrays));
    },

    /**
     * Load any previously saved ignored ID sets from local storage.
     */
    loadSavedIgnoredIDs() {
      this.savedIgnoredIDs = this.loadIgnoredIDsFromStorage();
    },

    /**
     * Helper to load from local storage (returns an array).
     */
    loadIgnoredIDsFromStorage() {
      let savedArrays = JSON.parse(localStorage.getItem("savedArrays") || "[]");
      if (!Array.isArray(savedArrays)) savedArrays = [];
      return savedArrays;
    },

    /* -------------------------------------------------------------------------- */
    /*                            Uploading Selected IDs                           */
    /* -------------------------------------------------------------------------- */

    /**
     * Upload the selected CAN IDs to each ECU port (where user has selected IDs).
     */
    async uploadToECUs() {
      this.loading = true;
      // Let the UI update
      await this.$nextTick();

      const tasks = this.serialPorts.map((portData, index) => {
        if (this.selectedIds[index]?.length > 0) {
          return this.uploadToECU(index);
        }
        return Promise.resolve();
      });
      await Promise.all(tasks);
      this.loading = false;
    },

    /**
     * For a given port index, find matching IDs, create messageMap, send them to the ECU.
     */
    async uploadToECU(index) {
      const selected = this.selectedIds[index];
      const maskHex = this.serialPorts[index].mask;
      const mask = parseInt(maskHex, 16);

      // Filter down to matching IDs according to the mask
      const matchingIDs = this.findMatchingIDs(selected, mask);
      if (!matchingIDs.length) {
        alert("No matching CAN IDs found.");
        return;
      }

      // Build local messages array
      const { ecuMessages, messageMap } = this.buildPortMessages(matchingIDs);

      // Apply to the port
      this.serialPorts[index].messages = ecuMessages;
      this.serialPorts[index].messageMap = messageMap;

      // Format the message and send
      const payload = {
        ids: selected.map((id) => `"${id}"`).join(","),
        mask: mask,
      };
      const command = `upload:{"ids":[${payload.ids}],"mask":${payload.mask}}:end`;

      const portData = this.serialPorts[index];
      await this.sendCommandToPort(portData, command);
    },

    /**
     * Build the messages array and messageMap for a given set of matching IDs.
     * Returns { ecuMessages, messageMap }.
     */
    buildPortMessages(matchingIDs) {
      const ecuMessages = [];
      const messageMap = new Map();

      // Compare as uppercase hex, no "0x" prefix
      this.canMessages
        .filter((m) => matchingIDs.includes(m.id.toString(16).toUpperCase()))
        .forEach((message, idx) => {
          const text = `${message.id}#${message.data}`;
          const entry = { text, status: "unreceived", timestamp: null };

          ecuMessages.push(entry);

          if (!messageMap.has(text)) {
            messageMap.set(text, []);
          }
          messageMap.get(text).push(idx);
        });

      return { ecuMessages, messageMap };
    },

    /**
     * Find all possible IDs (0 to 0x7FF) that match the masked versions of user-selected IDs.
     */
    findMatchingIDs(filteredIDs, mask) {
      const numericFiltered = filteredIDs.map((id) => parseInt(id, 16));
      const maxID = 0x7ff;
      const matching = [];

      for (let id = 0; id <= maxID; id++) {
        for (const fid of numericFiltered) {
          if ((id & mask) === (fid & mask)) {
            matching.push(id.toString(16).toUpperCase().padStart(3, "0"));
            break;
          }
        }
      }
      return matching;
    },

    /**
     * Capture newly selected ID from multiselect, remove from uniqueIds if needed.
     */
    handleSelect(value) {
      const index = this.uniqueIds.indexOf(value);
      if (index !== -1) {
        this.uniqueIds.splice(index, 1);
      }
      this.saveSelectedIds();
    },

    /**
     * On deselect, re-add to uniqueIds to keep them in the pool.
     */
    handleDeselect(value) {
      this.uniqueIds.push(value);
      this.saveSelectedIds();
    },

    /**
     * Save selected IDs to local storage for each port (keyed by deviceName).
     */
    saveSelectedIds() {
      this.serialPorts.forEach((portData, index) => {
        if (!portData.deviceName) return;
        localStorage.setItem(
          `selectedIds-${portData.deviceName}`,
          JSON.stringify(this.selectedIds[index])
        );
      });
    },

    /**
     * Load previously saved IDs for each port from local storage.
     */
    loadSelectedIds(length) {
      return Array.from({ length }, (_, idx) => {
        const deviceName = this.serialPorts[idx]?.deviceName;
        if (!deviceName) return [];
        const saved = localStorage.getItem(`selectedIds-${deviceName}`);
        return saved ? JSON.parse(saved) : [];
      });
    },

    /* -------------------------------------------------------------------------- */
    /*                              File Upload Logic                              */
    /* -------------------------------------------------------------------------- */

    /**
     * Handle file upload event and read the text content.
     */
    uploadFile(event) {
      const file = event.target.files[0];
      if (!file) {
        this.error = "No file selected";
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

    /**
     * Process the file content (candump format).
     */
    processFile(fileContent) {
      try {
        this.canData = this.parseCandumpFile(fileContent);
        this.sendMessagesToBackend();
        this.uniqueIds = this.canData ? Object.keys(this.canData) : [];
        this.error = "";
      } catch (err) {
        this.loading = false;
        this.error = "Error processing file";
        console.error(err);
      }
    },

    /**
     * Parse candump file content into an object, also fill `this.canMessages`.
     */
    parseCandumpFile(content) {
      const canData = {};
      let firstTimestamp = null;

      const lines = content.split("\n");
      lines.forEach((line) => {
        const parts = line.trim().split(/\s+/);
        if (parts.length > 2) {
          const timestamp = parseFloat(parts[0].replace(/[()]/g, ""));
          const [canId, data] = parts[2].split("#");

          if (firstTimestamp === null) {
            firstTimestamp = timestamp;
          }
          const relativeTimestamp = timestamp - firstTimestamp;

          if (!canData[canId]) {
            canData[canId] = [];
          }
          canData[canId].push({
            timestamp: relativeTimestamp,
            data: data || "",
          });

          this.canMessages.push({
            timestamp: relativeTimestamp,
            id: canId,
            data: data,
          });
        }
      });
      return canData;
    },

    /**
     * Send the loaded canMessages to a backend endpoint.
     */
    async sendMessagesToBackend() {
      try {
        const response = await axios.post(
          "http://localhost:5000/load_can_messages",
          this.canMessages
        );
        console.log("CAN messages loaded successfully:", response.data);
      } catch (error) {
        console.error("Error sending messages to backend:", error);
      }
    },

    /* -------------------------------------------------------------------------- */
    /*                                Simulation                                   */
    /* -------------------------------------------------------------------------- */

    /**
     * Begin or restart the simulation on the backend server.
     * Resets relevant message states if restarting.
     */
    async beginSimulation() {
      if (this.simulationStartTime) {
        // Reset "unreceived" states
        this.serialPorts.forEach((port) => {
          // Remove "unexpected" messages
          port.messages = port.messages.filter((msg) => msg.status !== "unexpected");
          port.messages.forEach((msg) => {
            msg.status = "unreceived";
            msg.timestamp = null;
          });
        });
      }

      try {
        this.simulationRunning = true;
        const payload = { interface: "can0", bitrate: 500000 };
        const response = await axios.post(
          "http://localhost:5000/start_simulation",
          payload
        );
        console.log("Simulation started successfully:", response.data);
      } catch (error) {
        console.error("Error starting simulation:", error.response?.data || error.message);
        this.simulationRunning = false;
      }
      this.simulationStartTime = Date.now();
    },

    /**
     * Once the simulation completes, gather final messages from each port's buffer.
     */
    async processReceivedMessages() {
      const results = await Promise.all(
        this.serialPorts.map(async (portData) => {
          const { workerFn, terminate } = useWebWorkerFn((pd) => {
            // Recreate data structures
            const lines = pd.buffer.split("\n");
            const messages = JSON.parse(pd.messages);
            const rawMap = pd.messageMap ? JSON.parse(pd.messageMap) : null;
            const messageMap = rawMap ? new Map(rawMap) : new Map();
            let consoleOutput = "";

            for (const line of lines) {
              const message = line.trim();
              if (message.startsWith("recv:") && message.endsWith(":endrecv")) {
                if (!message.includes("#")) {
                  consoleOutput += `Malformed message: ${message}\n`;
                  continue;
                }
                const cleanMessage = message
                  .replace(/^recv:/, "")
                  .replace(/:endrecv$/, "");
                const [id, data, timestampStr] = cleanMessage.split("#");

                if (!id || !data || isNaN(timestampStr)) {
                  consoleOutput += `Invalid message format: ${message}\n`;
                  continue;
                }
                const messageContent = `${id}#${data}`;
                const timestamp = parseInt(timestampStr, 10);

                let messageIndices = messageMap.get(messageContent) || [];
                let nextMessageIndex = messageIndices.shift();
                if (nextMessageIndex === undefined) {
                  messages.push({
                    text: messageContent,
                    status: "unexpected",
                    timestamp,
                  });
                } else {
                  messages[nextMessageIndex].status = "received";
                  messages[nextMessageIndex].timestamp = timestamp;
                  if (messageIndices.length > 0) {
                    messageMap.set(messageContent, messageIndices);
                  } else {
                    messageMap.delete(messageContent);
                  }
                }
              } else {
                consoleOutput += `${message}\n`;
              }
            }

            pd.buffer = "";
            return { messages, consoleOutput };
          });
          portData.worker = workerFn;
          portData.terminateWorker = terminate;

          const messagesJson = JSON.stringify(portData.messages);
          const messageMapJson = JSON.stringify(
            portData.messageMap ? Array.from(portData.messageMap) : null
          );

          const result = await portData.worker({
            buffer: portData.buffer,
            messages: messagesJson,
            messageMap: messageMapJson,
          });
          return result;
        })
      );

      // Apply results back to each port
      this.serialPorts.forEach((portData, index) => {
        portData.messages = results[index].messages;
        portData.consoleOutput += results[index].consoleOutput;
      });
      console.log("Received messages processed.");
    },

    /* -------------------------------------------------------------------------- */
    /*                              Attack / Injection                             */
    /* -------------------------------------------------------------------------- */

    /**
     * Open the modal to configure an injection attack.
     */
    configureAttack(port) {
      this.attack.port = port;
      this.modalType = "attackConfig";
      this.showModal = true;
    },

    /**
     * Send the replay injection configuration to the chosen port.
     */
    configureReplay() {
      const { message, repeat, interval, startTime } = this.replayConfig;
      const { port } = this.attack;

      if (!port) {
        console.error("No attack port selected.");
        return;
      }
      try {
        const replayMessage = `replay:${message}:${repeat}:${interval}:${startTime}:endreplay`;
        this.sendCommandToPort(port, replayMessage);
        this.showModal = false;
      } catch (error) {
        console.error("Error configuring replay attack:", error);
      }
    },
  },
};
</script>

<style>
/* Shared / Global styles */
textarea {
  width: 100%;
  margin-bottom: 10px;
}

input {
  width: calc(100% - 100px);
  margin-right: 10px;
}

.center-container {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background: white;
  padding: 20px;
  border-radius: 8px;
  max-width: 800px;
  margin: 0 auto;
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
  font-weight: bold;
  vertical-align: middle;
  transition: color 0.2s ease-in-out;
}
.help-icon:hover {
  color: #007bff;
}

.help-text {
  background: #fff9c4;
  padding: 8px 12px;
  margin-top: 5px;
  border: 1px solid #ffd54f;
  border-radius: 4px;
  max-width: 800px;
  font-size: 14px;
  color: #333;
  position: absolute;
  z-index: 10;
  box-shadow: 2px 2px 5px rgba(0, 0, 0, 0.3);
  text-align: left;
  line-height: 1.4;
}

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

.console-buttons {
  display: flex;
  justify-content: center;
  gap: 10px;
}

.messages-header {
  display: flex;
  justify-content: center;
  align-items: center;
  gap: 6px;
  margin-top: 8px;
}

.scroll-container {
  height: 300px;
  overflow-y: auto;
  border: 1px solid #ccc;
  padding: 10px;
  background-color: #f9f9f9;
}

/* Attack Config / Mask Config Modal */
.attack-config {
  margin: 20px;
}
.configure-button {
  background-color: #007bff;
  color: white;
  padding: 10px 15px;
  border-radius: 4px;
  cursor: pointer;
  font-size: 16px;
  border: none;
}
.configure-button:hover {
  background-color: #0056b3;
}
</style>
