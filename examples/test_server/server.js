const fs = require('fs');
const http = require('http');
const https = require('https');
const { Server } = require('socket.io');
const express = require('express');

// Read command-line argument
const args = process.argv.slice(2);
const isSecure = args.includes('--secure');

// Express application
const app = express();

// Serve static files or handle HTTP requests (optional)
app.get('/', (req, res) => {
  res.send('Socket.IO Test Server');
});

let server;

if (isSecure) {
  // Read SSL certificate files
  const options = {
    key: fs.readFileSync('server_key.pem'),
    cert: fs.readFileSync('server_cert.pem'),
    // ca: fs.readFileSync('ca.pem'), // Uncomment if you have a CA certificate
  };

  // Create HTTPS server
  server = https.createServer(options, app);
  console.log('Starting server with HTTPS');
} else {
  // Create HTTP server
  server = http.createServer(app);
  console.log('Starting server with HTTP');
}

// Create a Socket.IO server
const io = new Server(server);

// Default namespace ('/')
io.on('connection', (socket) => {
  console.log('Client connected to the default namespace:', socket.id);

  // Listen for 'message' events from the client
  socket.on('message', (msg) => {
    console.log('Received message from client:', msg);

    // Emit a 'message' event back to the client
    socket.emit('message', 'Hello from the server!');
  });

  // Demonstrate handling of various data types.
  socket.on('hello', (arg1, arg2, arg3) => {
    console.log(arg1); // 1
    console.log(arg2); // "2"
    console.log(arg3); // { 3: '4', 5: ArrayBuffer (1) [ 6 ] }
    socket.emit('hello', 'Mixed data received by server!');
  });

  // Demonstrate handling of event ID.
  socket.on("update item", (arg1, arg2, callback) => {
    console.log(arg1); // 1
    console.log(arg2); // { name: "updated" }
    callback({
      status: "ok"
    });
  });

  socket.on('disconnect', () => {
    console.log('Client disconnected from default namespace:', socket.id);
  });
});

// Custom namespace ('/chat')
const chatNamespace = io.of('/chat');
chatNamespace.on('connection', (socket) => {
  console.log('Client connected to the /chat namespace:', socket.id);

  // Demonstrate handling of various data types.
  socket.on("hello", (arg1, arg2, arg3, arg4, arg5, callback) => {
    console.log('Message from /chat namespace:');
    console.log(arg1); // 1
    console.log(arg2);
    console.log(arg3);
    console.log(arg4);
    console.log(arg5);
    callback({
      status: "ok"
    });
    socket.emit("hello", 1, "2", { 3: '4', 5: Buffer.from([6, 5]), "6": Buffer.from([48, 49, 50, 51]) });
  });

  socket.on('disconnect', () => {
    console.log('Client disconnected from /chat namespace:', socket.id);
  });
});

// Start the server
const port = 3300;
server.listen(port, () => {
  console.log(`Server is running on port ${port}`);
});
