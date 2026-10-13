<template>
  <div id="app">
    <el-container>
      <el-header>
        <h1>Web3.0 AI Agent IDE</h1>
      </el-header>
      <el-main>
        <el-row :gutter="20">
          <el-col :span="12">
            <el-card>
              <template #header>
                <span>Agent Info</span>
              </template>
              <div v-if="agentInfo">
                <p><strong>Address:</strong> {{ agentInfo.address }}</p>
                <p><strong>Chain ID:</strong> {{ agentInfo.chain?.chainId }}</p>
                <p><strong>Block:</strong> {{ agentInfo.chain?.blockNumber }}</p>
                <p><strong>LLM:</strong> {{ agentInfo.llm }}</p>
                <p><strong>Peers:</strong> {{ agentInfo.peers?.join(', ') || 'None' }}</p>
              </div>
              <el-button @click="loadInfo" type="primary">Refresh</el-button>
            </el-card>
          </el-col>
          <el-col :span="12">
            <el-card>
              <template #header>
                <span>Chat with Agent</span>
              </template>
              <el-input
                v-model="message"
                type="textarea"
                :rows="4"
                placeholder="Type your message..."
              />
              <el-button @click="sendMessage" type="primary" style="margin-top: 10px">Send</el-button>
              <div v-if="response" style="margin-top: 20px">
                <el-alert :title="response" type="success" :closable="false" />
              </div>
            </el-card>
          </el-col>
        </el-row>
      </el-main>
    </el-container>
  </div>
</template>

<script>
import axios from 'axios'

export default {
  name: 'App',
  data() {
    return {
      agentInfo: null,
      message: '',
      response: ''
    }
  },
  mounted() {
    this.loadInfo()
  },
  methods: {
    async loadInfo() {
      try {
        const res = await axios.get('http://localhost:8080/api/agent/info')
        this.agentInfo = res.data
      } catch (error) {
        console.error('Failed to load agent info:', error)
      }
    },
    async sendMessage() {
      if (!this.message.trim()) return
      try {
        const res = await axios.post('http://localhost:8080/api/agent/chat', {
          message: this.message
        })
        this.response = res.data.response
      } catch (error) {
        console.error('Failed to send message:', error)
        this.response = 'Error: ' + error.message
      }
    }
  }
}
</script>

<style>
#app {
  font-family: Avenir, Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  color: #2c3e50;
}
.el-header {
  background-color: #409eff;
  color: white;
  text-align: center;
  line-height: 60px;
}
</style>
