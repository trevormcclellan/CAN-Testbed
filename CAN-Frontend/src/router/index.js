import { createRouter, createWebHistory } from 'vue-router';
import Home from '../views/Home.vue';
import SerialConnect from '../views/SerialConnect.vue';

const routes = [
  { path: '/', name: 'Home', component: Home },
  { path: '/connect', name: 'SerialConnect', component: SerialConnect },
];

const router = createRouter({
  history: createWebHistory(),
  routes,
});

export default router;
