# Changelog

## [1.0.0]

### Features

* Change to a ESP-IDF component.

## [0.0.2]

### Features

* Socket.IO CONNECT packet supports adding data payload.

### Bug Fixes

* Fix Linux target build error.

## [0.0.1]

### Initial Version

- Protocol and packet encoding according to Socket.IO protocol (https://socket.io/docs/v4/socket-io-protocol/) and Engine.IO protocol (https://github.com/socketio/engine.io-protocol), EIO=4.
- Support only websocket transport (no HTTP polling).
- Support both ws and wss (server-only authentication and mutual authentication).
- Data: both event and binary event.
- Custom namespace.
- Acknowledgement with event ID.
- Keep-alive with ping-pong packets.
  