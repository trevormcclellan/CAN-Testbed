<template>
    <div>
      <ul>
        <!-- Loop through the messages and display each with its corresponding status -->
        <li v-for="(message, index) in messages" :key="index">
          <span>{{ message.text }}</span>
          <span v-if="message.timestamp">({{ formatTimestamp(message.timestamp) }}ms)</span>
          <span v-if="message.status === 'received'">✔️</span>
          <span v-if="message.status === 'unexpected'">❗</span>
        </li>
      </ul>
    </div>
  </template>
  
  <script>
  export default {
    props: {
      // Accept messages as a prop from the parent component
      messages: {
        type: Array,
        required: true
      }
    },
    methods: {
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
  