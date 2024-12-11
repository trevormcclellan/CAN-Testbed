<template>
    <div>
      <ul>
        <!-- Loop through the messages and display each with its corresponding status -->
        <li v-for="(message, index) in messages" :key="index">
          <span>{{ message.text }}</span>
          <span v-if="message.timestamp">({{ message.timestamp }}ms)</span>
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
      // If you want to pass a message back to the parent to be handled there
      handleMessage(newMessage) {
        // Check if the message already exists in the list
        const existingMessage = this.messages.find(message => message.text === newMessage.text);
  
        if (existingMessage) {
          // If message exists, update its status to 'received'
          existingMessage.status = 'received';
        } else {
          // If message does not exist, add it with a 'new' status
          this.messages.push({ text: newMessage.text, status: 'new' });
        }
      }
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
  