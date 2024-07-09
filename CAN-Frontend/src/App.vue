<template>
  <div id="app">
    <CanDumpUpload />
  </div>
</template>

<script>
import CanDumpUpload from './components/CanDumpUpload.vue';

export default {
  name: 'App',
  components: {
    CanDumpUpload
  }
};
</script>

<style>
#app {
  font-family: Avenir, Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;
  color: #2c3e50;
  margin-top: 60px;
}
</style>
