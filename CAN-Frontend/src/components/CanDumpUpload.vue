<template>
    <div>
      <h1>Upload CANdump File</h1>
      <input type="file" @change="onFileChange" />
      <button @click="uploadFile">Upload</button>
      <p v-if="uploadStatus">{{ uploadStatus }}</p>
      <p v-if="uniqueIdCount !== null">Unique CAN IDs: {{ uniqueIdCount }}</p>
    </div>
  </template>
  
  <script>
  export default {
    data() {
      return {
        file: null,
        uploadStatus: '',
        uniqueIdCount: null
      };
    },
    methods: {
      onFileChange(event) {
        this.file = event.target.files[0];
      },
      async uploadFile() {
        if (!this.file) {
          this.uploadStatus = 'Please select a file first.';
          return;
        }
  
        const formData = new FormData();
        formData.append('file', this.file);
  
        try {
          const response = await fetch('http://localhost:5000/upload', {
            method: 'POST',
            body: formData
          });
          
          const data = await response.json();
          if (response.ok) {
            this.uploadStatus = 'File uploaded successfully!';
            this.uniqueIdCount = data.unique_id_count;
          } else {
            this.uploadStatus = 'File upload failed.';
          }
        } catch (error) {
          console.error('Error uploading file:', error);
          this.uploadStatus = 'An error occurred during the upload.';
        }
      }
    }
  };
  </script>
  
  <style scoped>
  h1 {
    margin-bottom: 20px;
  }
  input[type="file"] {
    display: block;
    margin-bottom: 10px;
  }
  button {
    display: block;
    margin-bottom: 10px;
  }
  p {
    color: green;
  }
  </style>
  