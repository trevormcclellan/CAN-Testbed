<template>
    <div>
      <ul>
        <!-- Loop through the messages and display each with its corresponding status -->
          <li v-for="(message, index) in buffer" :key="index">
            {{ formatMessage(message) }}
          </li>
      </ul>
    </div>
  </template>
  
  <script>
  export default {
    props: {
      // Accept messages as a prop from the parent component
      buffer: {
        type: Array,
        default: () => []
      },
      simulationOffset: {
        type: Number,
        default: 0
      },
      millisAtSync: {
        type: Number,
        default: 0
      },
    },
    methods: {
      formatMessage(message) {
        message = message.trim();
        if (message.startsWith('recv:') && message.endsWith(':endrecv')) {
          if (!message.includes('#')) {
            return
          }

          // Remove 'recv:' and ':endrecv' from the message
          const cleanMessage = message.replace(/^recv:/, '').replace(/:endrecv$/, '');
          const [id, data, timestampStr] = cleanMessage.split('#');
          if (!id || !data || isNaN(timestampStr)) {
            return
          }

          const messageContent = `${id}#${data}`;
          const timestamp = parseInt(timestampStr, 10);
          const adjustedTimestamp = (timestamp - this.millisAtSync) + this.simulationOffset;

          return `${messageContent} (${adjustedTimestamp}ms)`;
        }
        // Format the message as needed
        return message;
      },
      // Format the timestamp as the difference from the first message's timestamp
      formatTimestamp(currentTimestamp) {
        // If messages exist, subtract the timestamp of the first message
        if (this.messages.length > 0) {
          const firstTimestamp = this.messages[0].timestamp;
          return currentTimestamp - firstTimestamp;
        }
        return currentTimestamp; // Default to showing the raw timestamp if no messages exist
      },
    }
  };
  </script>
  
  <style scoped>
  /* You can add custom styles here for your messages */
  ul {
    list-style-type: none;
    padding: 0;
  }
  
  li {
    padding: 5px;
    margin: 5px 0;
  }
  
  span {
    margin-left: 5px;
  }
  </style>
  