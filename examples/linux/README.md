# ESP Socket.IO Client - Host Example

This example demonstrates the ESP Socket.IO client using the `linux` target. It can be compiled and executed in a Linux Environment.

### Test Server

To start test server, go to the folder [test_server](../test_server/), install all node.js dependency modules, then execute:

```
node server.js
```

This starts a non-secure Socket.IO server. Default port is 3300.

## Compilation and Execution

To compile and execute this example on Linux, set target to `linux`:

```
idf.py --preview set-target linux
Change the value of CONFIG_WEBSOCKET_URI
idf.py build
./build/socketio.elf
```

## Example Output

```
I (259762728) socketio: [APP] Startup..
I (259762728) socketio: [APP] Free memory: 4294967295 bytes
I (259762728) socketio: [APP] IDF version: v5.4-dev-610-g003f3bb5dc-dirty
I (259762733) socketio: Connecting to ws://192.168.1.10:3300/socket.io/?EIO=4&transport=websocket...
W (259762733) websocket_client: `reconnect_timeout_ms` is not set, or it is less than or equal to zero, using default time out 10000 (milliseconds)
W (259762733) websocket_client: `network_timeout_ms` is not set, or it is less than or equal to zero, using default time out 10000 (milliseconds)
D (259762734) transport_ws: Write upgrade request
GET /socket.io/?EIO=4&transport=websocket HTTP/1.1
Connection: Upgrade
Host: 192.168.1.10:3300
User-Agent: ESP32 Websocket Client
Upgrade: websocket
Sec-WebSocket-Version: 13
Sec-WebSocket-Key: dIiAyziELIELvPXVcYHBuQ==


D (259762737) transport_ws: Read header chunk 129, current header size: 129
D (259762737) transport_ws: HTTP status code is 101
D (259762737) transport_ws: server key=aHixn4RIPWGi5JKy+Sr6Gr9jlgk=, send_key=dIiAyziELIELvPXVcYHBuQ==, expected_server_key=aHixn4RIPWGi5JKy+Sr6Gr9jlgk=
D (259762737) websocket_client: Transport connected to ws://192.168.1.10:3300
D (259762738) transport_ws: Opcode: 1, mask: 0, len: 107
I (259762738) socketio_client: WEBSOCKET_EVENT_DATA
I (259762738) socketio_client: Received opcode=1
W (259762738) socketio_client: Received=0{"sid":"n0R1LpS603yKasMCAAAD","upgrades":[],"pingInterval":25000,"pingTimeout":20000,"maxPayload":1000000}
I (259762738) socketio_client: Total payload length=107, data_len=107, current payload offset=0

I (259762738) socketio_packet: Received Engine.IO OPEN packet: sid = n0R1LpS603yKasMCAAAD, pingInterval = 25000, pingTimeout = 20000, maxPayload = 1000000.

I (259762738) socketio: WEBSOCKET_EVENT_DATA
I (259762738) socketio: Received Socket.IO OPEN packet.
D (259762738) transport_ws: Sending raw ws message with opcode 129
I (259762738) socketio_client: Send connect (size: 2) to / successfully.
D (259762780) transport_ws: Opcode: 1, mask: 0, len: 32
I (259762780) socketio_client: WEBSOCKET_EVENT_DATA
I (259762780) socketio_client: Received opcode=1
W (259762780) socketio_client: Received=40{"sid":"a_17XxYzLXWgtLkDAAAE"}
I (259762780) socketio_client: Total payload length=32, data_len=32, current payload offset=0

I (259762780) socketio_client: Add namespace: /, sid: a_17XxYzLXWgtLkDAAAE
I (259762780) socketio: WEBSOCKET_EVENT_DATA
I (259762780) socketio: Socket.IO connected to default namespace "/"
D (259762780) transport_ws: Sending raw ws message with opcode 129
I (259762780) socketio_client: Send connect (size: 8) to /chat successfully.
D (259762823) transport_ws: Opcode: 1, mask: 0, len: 38
I (259762823) socketio_client: WEBSOCKET_EVENT_DATA
I (259762823) socketio_client: Received opcode=1
W (259762823) socketio_client: Received=40/chat,{"sid":"nsqMHa-xLRuJ6-0xAAAF"}
I (259762823) socketio_client: Total payload length=38, data_len=38, current payload offset=0

I (259762823) socketio_packet: Custom namespace: /chat
I (259762823) socketio_client: Add namespace: /chat, sid: nsqMHa-xLRuJ6-0xAAAF
I (259762823) socketio: WEBSOCKET_EVENT_DATA
I (259762823) socketio: Socket.IO connected to namespace: "/chat"
I (259762823) socketio: address of bin_object_1: 0x7f7948000c50, size: 8
I (259762823) socketio: address of bin_object_2: 0x7f7948001260, size: 8
D (259762823) transport_ws: Sending raw ws message with opcode 129
D (259762823) transport_ws: Sending raw ws message with opcode 130
D (259762823) transport_ws: Sending raw ws message with opcode 130
D (259762867) transport_ws: Opcode: 1, mask: 0, len: 26
I (259762867) socketio_client: WEBSOCKET_EVENT_DATA
I (259762867) socketio_client: Received opcode=1
W (259762867) socketio_client: Received=43/chat,0[{"status":"ok"}]
I (259762867) socketio_client: Total payload length=26, data_len=26, current payload offset=0

I (259762867) socketio_packet: Custom namespace: /chat
I (259762867) socketio_packet: Parsing event ID.
I (259762867) socketio: WEBSOCKET_EVENT_DATA
I (259762867) socketio: Received data from server:
I (259762867) socketio: EIO: 4
I (259762867) socketio: SIO: 3
I (259762867) socketio: Namespace: /chat
I (259762867) socketio: Event ID: 0
I (259762867) socketio: [{
		"status":	"ok"
	}]
D (259762867) transport_ws: Opcode: 1, mask: 0, len: 103
I (259762867) socketio_client: WEBSOCKET_EVENT_DATA
I (259762867) socketio_client: Received opcode=1
W (259762867) socketio_client: Received=452-/chat,["hello",1,"2",{"3":"4","5":{"_placeholder":true,"num":0},"6":{"_placeholder":true,"num":1}}]
I (259762867) socketio_client: Total payload length=103, data_len=103, current payload offset=0

I (259762867) socketio_packet: Custom namespace: /chat
D (259762867) transport_ws: Opcode: 2, mask: 0, len: 2
I (259762867) socketio_client: WEBSOCKET_EVENT_DATA
I (259762867) socketio_client: Received opcode=2
W (259762867) socketio_client: Received=
I (259762867) socketio_client: Total payload length=2, data_len=2, current payload offset=0

I (259762867) socketio_client: Received binary
D (259762868) transport_ws: Opcode: 2, mask: 0, len: 4
I (259762868) socketio_client: WEBSOCKET_EVENT_DATA
I (259762868) socketio_client: Received opcode=2
W (259762868) socketio_client: Received=0123
I (259762868) socketio_client: Total payload length=4, data_len=4, current payload offset=0

I (259762868) socketio_client: Received binary
I (259762868) socketio: WEBSOCKET_EVENT_DATA
I (259762868) socketio: Received data from server:
I (259762868) socketio: EIO: 4
I (259762868) socketio: SIO: 5
I (259762868) socketio: Namespace: /chat
I (259762868) socketio: Event ID: -1
I (259762868) socketio: ["hello", 1, "2", {
		"3":	"4",
		"5":	{
			"_placeholder":	true,
			"num":	0
		},
		"6":	{
			"_placeholder":	true,
			"num":	1
		}
	}]
+ <Buffer 0 <06 05>>
+ <Buffer 1 <30 31 32 33>>
D (259772879) websocket_client: Sending PING...
D (259772879) transport_ws: Sending raw ws message with opcode 137
D (259772880) transport_ws: Opcode: 10, mask: 0, len: 0
I (259772880) socketio_client: WEBSOCKET_EVENT_DATA
I (259772880) socketio_client: Received opcode=10
W (259772880) socketio_client: Received=
W (259772880) socketio_client: Received WS PONG
I (259772880) socketio_client: Total payload length=0, data_len=0, current payload offset=0

D (259782892) websocket_client: Sending PING...
D (259782892) transport_ws: Sending raw ws message with opcode 137
D (259782893) transport_ws: Opcode: 10, mask: 0, len: 0
I (259782893) socketio_client: WEBSOCKET_EVENT_DATA
I (259782893) socketio_client: Received opcode=10
W (259782893) socketio_client: Received=
W (259782893) socketio_client: Received WS PONG
I (259782893) socketio_client: Total payload length=0, data_len=0, current payload offset=0

D (259787743) transport_ws: Opcode: 1, mask: 0, len: 1
I (259787743) socketio_client: WEBSOCKET_EVENT_DATA
I (259787743) socketio_client: Received opcode=1
W (259787743) socketio_client: Received=2
I (259787743) socketio_client: Total payload length=1, data_len=1, current payload offset=0

D (259787743) socketio_client: Receive Engine.IO PING, sending PONG
D (259787744) socketio_client: timer stopped
D (259787744) transport_ws: Sending raw ws message with opcode 129
```
