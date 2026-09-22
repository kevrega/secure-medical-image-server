import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'


export default defineConfig({
  plugins: [
    react()
  ],

  server: {
    proxy: {
      // Forward frontend API calls to the C++ server during development
      '/api': {
        target: 'http://localhost:1337',
        changeOrigin: true,

        // The C++ server uses /patients instead of /api/patients
        rewrite: path =>
          path.replace(/^\/api/, '')
      }
    }
  }
})