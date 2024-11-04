const mqtt = require('mqtt');
const fs = require('fs');

// Thông tin MQTT broker
const mqttOptions = {
  host: 'f69f6416905b4890a65c0d638608cfff.s1.eu.hivemq.cloud',
  port: 8883,
  protocol: 'mqtts',  // TLS option
  username: 'test',  // uname to connect mqtt
  password: '1',  // password to connect mqtt
  rejectUnauthorized: true,  
};

const client = mqtt.connect(mqttOptions);

// Complete connection
client.on('connect', () => {
  console.log('Connected to MQTT broker');
  client.subscribe('hello', (err) => {
    if (err) {
      console.error('Failed to subscribe:', err);
    } else {
      console.log('Subscribed to topic!');
    }
  });
});

// Message receive
client.on('message', (topic, message) => {
  console.log(`Message received on topic ${topic}: ${message.toString()}`);
});

// Error 
client.on('error', (error) => {
  console.error('Connection error:', error);
});
client.on('close', () => {
  console.log('Connection closed');
});
