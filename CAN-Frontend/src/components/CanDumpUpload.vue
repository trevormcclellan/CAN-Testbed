<template>
  <div>
    <h1>Upload CANdump File</h1>
    <input type="file" @change="onFileChange" />
    <button @click="uploadFile">Upload</button>
    <p v-if="uploadStatus">{{ uploadStatus }}</p>
    <p v-if="uniqueIdCount !== null">Unique CAN IDs: {{ uniqueIdCount }}</p>
    <div v-if="nodes.length > 0">
      <h2>Nodes</h2>
      <ul>
        <li v-for="node in nodes" :key="node.node_id">
          <strong>{{ node.name }}:</strong> {{ node.description }}
          <div>
            <label for="selectIds">Select IDs:</label>
            <Multiselect
              v-model="selectedIds[node.node_id]"
              mode="tags"
              :searchable=true
              :options="uniqueIds"
              placeholder="Select CAN IDs"
              label="ID"
              track-by="ID"
              :show-labels="false"
            />
          </div>
          <button @click="selectIdsForNode(node.node_id)">Submit IDs for Node {{ node.node_id }}</button>
        </li>
      </ul>
    </div>
  </div>
</template>

<script>
import { ref, reactive, onMounted } from 'vue';
import Multiselect from '@vueform/multiselect';
import '@vueform/multiselect/themes/default.css';

export default {
  components: {
    Multiselect
  },
  setup() {
    const file = ref(null);
    const uploadStatus = ref('');
    const uniqueIds = ref([]);
    const uniqueIdCount = ref(null);
    const nodes = ref([]);
    const selectedIds = reactive({});

    const onFileChange = (event) => {
      file.value = event.target.files[0];
    };

    const uploadFile = async () => {
      if (!file.value) {
        uploadStatus.value = 'Please select a file first.';
        return;
      }

      const formData = new FormData();
      formData.append('file', file.value);

      try {
        const response = await fetch('http://localhost:5000/upload', {
          method: 'POST',
          body: formData
        });

        const data = await response.json();
        if (response.ok) {
          uploadStatus.value = 'File uploaded successfully!';
          uniqueIds.value = data.unique_ids;
          uniqueIdCount.value = uniqueIds.value.length;
          await fetchNodes();
        } else {
          uploadStatus.value = 'File upload failed.';
        }
      } catch (error) {
        console.error('Error uploading file:', error);
        uploadStatus.value = 'An error occurred during the upload.';
      }
    };

    const fetchNodes = async () => {
      try {
        const response = await fetch('http://localhost:5000/nodes');
        const data = await response.json();
        if (response.ok) {
          nodes.value = data.nodes;
          nodes.value.forEach(node => {
            selectedIds[node.node_id] = [];
          });
        }
      } catch (error) {
        console.error('Error fetching nodes:', error);
      }
    };

    const selectIdsForNode = async (nodeId) => {
      try {
        const response = await fetch(`http://localhost:5000/node/${nodeId}/select_ids`, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json'
          },
          body: JSON.stringify({ selected_ids: selectedIds[nodeId] })
        });

        const data = await response.json();
        if (response.ok) {
          console.log(`IDs selected for Node ${nodeId}:`, data.selected_ids);
        } else {
          console.error('Error selecting IDs:', data);
        }
      } catch (error) {
        console.error('Error selecting IDs for node:', error);
      }
    };

    onMounted(fetchNodes);

    return {
      file,
      uploadStatus,
      uniqueIds,
      uniqueIdCount,
      nodes,
      selectedIds,
      onFileChange,
      uploadFile,
      selectIdsForNode
    };
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
select {
  display: block;
  margin-bottom: 10px;
}
</style>
