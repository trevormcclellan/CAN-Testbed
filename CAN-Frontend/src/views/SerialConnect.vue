<template>
  <div>
    <div class="button-group">
      <h3>Add New Device</h3>
      <input v-model="deviceName" placeholder="Enter device name" />
      <button @click="addDevice">Add Device</button>
    </div>

    <!-- Section for previously connected ports -->
    <div>
      <h2>Previously Connected Ports</h2>
      <div v-if="ports.length">
        <ul>
          <li v-for="(port, index) in ports" :key="index">
            <span>Port {{ index + 1 }}:
              {{ portData[index].name }} - {{ portStatuses[index] }}
            </span>
            <button @click="sendIdentify(port)" :disabled="portStatuses[index] !== 'Flashed'">Identify</button>
          </li>
        </ul>
      </div>
      <div v-else>
        <p>No previously connected ports found.</p>
      </div>
    </div>

    <!-- Output for serial data -->
    <div v-if="serialData">
      <h2>Serial Data</h2>
      <p>{{ serialData }}</p>
    </div>
  </div>
</template>

<script>
export default {
  data() {
    return {
      ports: [],
      portStatuses: [],
      portData: [],
      serialData: '',
      deviceName: '',
    };
  },
  async beforeUnmount() {
    console.log("Closing all ports...");
    this.ports.forEach((portData) => {
      this.closePort(portData);
    });
  },
  methods: {
    async addDevice() {
      if (!this.deviceName) {
        alert('Please enter a device name');
        return;
      }

      try {
        const port = await navigator.serial.requestPort();
        if (!port) {
          alert('No port selected');
          return;
        }

        if (!port.readable || !port.writable) {
          console.log(`Opening port for device name: ${this.deviceName}`);
          await port.open({ baudRate: 115200 });
        }

        const writer = port.writable.getWriter();
        const data = new TextEncoder().encode(`set_name ${this.deviceName}\n`);
        await writer.write(data);
        console.log('Sent "set_name" command to port');
        writer.releaseLock();

        this.ports.push(port);
        this.portStatuses.push('Checking...');
        this.portData.push({ name: '' });
        await this.checkIfFlashed(port, this.ports.length - 1);
      } catch (error) {
        console.error('Failed to send "set_name" command:', error);
      }
    },
    async connectToDevice() {
      try {
        const port = await navigator.serial.requestPort();
        await port.open({ baudRate: 115200 });
        console.log('Connected to device:', port);
        this.readFromPort(port);
      } catch (error) {
        console.error('Failed to connect to device:', error);
      }
    },
    async getPreviouslyConnectedPorts() {
      try {
        this.ports = await navigator.serial.getPorts();
        this.portStatuses = Array(this.ports.length).fill('Checking...');
        this.portData = Array(this.ports.length).fill({name: ''});
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
          this.portStatuses[index] = 'Not Flashed'; // Directly update the array
        } else {
          const response = new TextDecoder().decode(value).trim();
          console.log(`Port ${index} - Received response: ${response}`);
          if (response) {
            this.portStatuses[index] = 'Flashed'; // Directly update the array
            // Response format: "Device Name: <name>"
            const deviceName = response.split(': ')[1];
            this.portData[index] = { name: deviceName };
          } else {
            this.portStatuses[index] = 'Not Flashed'; // Directly update the array
          }
        }
      } catch (error) {
        console.error(`Error checking if device on port ${index} is flashed:`, error);
        this.portStatuses[index] = 'Unknown'; // Directly update the array
      }
    },
    async sendIdentify(port) {
      try {
        const writer = port.writable.getWriter();
        const data = new TextEncoder().encode("identify\n");
        await writer.write(data);
        console.log('Sent "identify" command to port');
        writer.releaseLock();
      } catch (error) {
        console.error('Failed to send "identify" command:', error);
      }
    },
    async readFromPort(port) {
      const reader = port.readable.getReader();

      try {
        while (true) {
          const { value, done } = await reader.read();
          if (done) {
            break;
          }
          const decodedData = new TextDecoder().decode(value);
          this.serialData += decodedData;
        }
      } catch (error) {
        console.error('Error reading from port:', error);
      } finally {
        reader.releaseLock();
      }
    },
    portInfo(port) {
      const info = port.getInfo();
      const vendorId = info.usbVendorId ? `Vendor ID: ${info.usbVendorId}` : 'Unknown Vendor';
      const productId = info.usbProductId ? `Product ID: ${info.usbProductId}` : 'Unknown Product';
      return `${vendorId} - ${productId}`;
    },
    async closePort(port) {
      try {
        await port.close();
        console.log('Port closed:', port);
      } catch (error) {
        console.error('Failed to close port:', error);
      }
    },
  },
  mounted() {
    this.getPreviouslyConnectedPorts();
  },
};
</script>

<style scoped>
.button-group {
  display: flex;
  flex-wrap: wrap;
  justify-content: center;
  gap: 10px;
  margin: 20px 0;
}

.button {
  padding: 8px 12px;
  font-size: 14px;
  border: 1px solid #ccc;
  border-radius: 5px;
  background-color: #f8f9fa;
  cursor: pointer;
  transition: background-color 0.2s ease;
  color: #333;
}

.button:hover {
  background-color: #e0e0e0;
}



button {
  margin-top: 10px;
  margin-bottom: 10px;
  margin-right: 10px;
}
</style>