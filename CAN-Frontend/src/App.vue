<!-- <template>
  <div id="app">
    <CanDumpUpload />
  </div>
</template> -->
<template>
  <div>
    <nav>
      <router-link to="/">Home</router-link>
      <router-link to="/connect">Connect Device</router-link>
    </nav>
    <router-view></router-view> <!-- This is where your components will be rendered -->
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
