# Socket.IO Sample application

This example will shows how to use the Socket.IO component to set up a Socket.IO connection and communicate in Socket.IO protocol.

## How to Use Example

### Test Server

To start test server, go to the folder [test_server](../test_server/), install all node.js dependency modules, then execute:

```
node server.js
```

This starts a non-secure Socket.IO server. Default port is 3300.

```
node server.js -secure
```
This starts a secure Socket.IO server. Use this option to test Socket.IO on wss.

### Hardware Required

This example can be executed on any ESP32 board, the only required interface is WiFi and connection to internet or a local server.

### Configure the project

* Open the project configuration menu (`idf.py menuconfig`)
* Configure Wi-Fi or Ethernet under "Example Connection Configuration" menu.
* Configure the Socket.IO endpoint URI under "Example Configuration", if "SOCKETIO_URI_FROM_STDIN" is selected then the example application will connect to the URI it reads from stdin (used for testing)
* To test a Socket.IO client example over TLS, please enable one of the following configurations: `CONFIG_WS_OVER_TLS_MUTUAL_AUTH` or `CONFIG_WS_OVER_TLS_SERVER_AUTH`. Remember to use `wss` in the URI.

> **_Note:_**
> - Refer to [Server Certificate Verification](https://github.com/espressif/esp-protocols/blob/master/components/esp_websocket_client/examples/target/README.md#server-certificate-verification) in the esp_websocket_client target example for detailed description of the two authentication methods. Note that same limitations apply to this example.
> - Refer to [Generating a self signed Certificates with OpenSSL](https://github.com/espressif/esp-protocols/blob/master/components/esp_websocket_client/examples/target/README.md#generating-a-self-signed-certificates-with-openssl) in the same example for a guide on how to generate self-signed certificates with OpenSSL.

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) for full steps to configure and use ESP-IDF to build projects.

## Example Output

```
I (4512) esp_netif_handlers: example_netif_sta ip: 192.168.1.100, mask: 255.255.255.0, gw: 192.168.1.1
I (4512) example_connect: Got IPv4 event: Interface "example_netif_sta" address: 192.168.1.100
I (4612) example_connect: Got IPv6 event: Interface "example_netif_sta" address: fe80:0000:0000:0000:1297:bdff:fed4:9e6c, type: ESP_IP6_ADDR_IS_LINK_LOCAL
I (4612) example_common: Connected to example_netif_sta
I (4622) example_common: - IPv4 address: 192.168.1.100,
I (4622) example_common: - IPv6 address: fe80:0000:0000:0000:1297:bdff:fed4:9e6c, type: ESP_IP6_ADDR_IS_LINK_LOCAL
I (4632) socketio: Connecting to wss://192.168.1.10:3300/socket.io/?EIO=4&transport=websocket...
W (4642) websocket_client: `reconnect_timeout_ms` is not set, or it is less than or equal to zero, using default time out 10000 (milliseconds)
W (4662) websocket_client: `network_timeout_ms` is not set, or it is less than or equal to zero, using default time out 10000 (milliseconds)
I (4682) wifi:<ba-add>idx:1 (ifx:0, 74:11:b2:c9:5e:40), tid:3, ssn:0, winSize:64
I (5502) socketio_client: WEBSOCKET_EVENT_DATA
I (5502) socketio_client: Received opcode=1
W (5502) socketio_client: Received=0{"sid":"nEYIG8ihxvM5ak5mAAAD","upgrades":[],"pingInterval":25000,"pingTimeout":20000,"maxPayload":1000000}
I (5512) socketio_client: Total payload length=107, data_len=107, current payload offset=0

I (5522) socketio_packet: Received Engine.IO OPEN packet: sid = nEYIG8ihxvM5ak5mAAAD, pingInterval = 25000, pingTimeout = 20000, maxPayload = 1000000.

I (5542) socketio_client: Start Socket.IO ping timer: 45000 ms
I (5542) socketio: WEBSOCKET_EVENT_DATA
I (5552) socketio: Received Socket.IO OPEN packet.
I (5562) socketio_client: Send connect (size: 2) to "/" successfully.
I (5582) socketio_client: WEBSOCKET_EVENT_DATA
I (5582) socketio_client: Received opcode=1
W (5582) socketio_client: Received=40{"sid":"fJEV25toHyCTbm3VAAAE"}
I (5592) socketio_client: Total payload length=32, data_len=32, current payload offset=0

I (5602) socketio_client: Add namespace: /, sid: fJEV25toHyCTbm3VAAAE
I (5602) socketio: WEBSOCKET_EVENT_DATA
I (5612) socketio: Socket.IO connected to default namespace "/"
I (5622) socketio_client: Send connect (size: 8) to "/chat" successfully.
I (5642) socketio_client: WEBSOCKET_EVENT_DATA
I (5642) socketio_client: Received opcode=1
W (5642) socketio_client: Received=40/chat,{"sid":"cB9vg6cAGadcoA4AAAAF"}
I (5642) socketio_client: Total payload length=38, data_len=38, current payload offset=0

I (5652) socketio_packet: Custom namespace: "/chat"
I (5662) socketio_client: Add namespace: /chat, sid: cB9vg6cAGadcoA4AAAAF
I (5672) socketio: WEBSOCKET_EVENT_DATA
I (5672) socketio: Socket.IO connected to namespace: "/chat"
I (5682) socketio: address of bin_object_1: 0x3ffd21e4, size: 4
I (5682) socketio: address of bin_object_2: 0x3ffd227c, size: 4
I (5722) socketio_client: WEBSOCKET_EVENT_DATA
I (5722) socketio_client: Received opcode=1
W (5722) socketio_client: Received=43/chat,0[{"status":"ok"}]
I (5732) socketio_client: Total payload length=26, data_len=26, current payload offset=0

I (5742) socketio_packet: Custom namespace: "/chat"
I (5742) socketio_packet: Parsing event ID.
I (5752) socketio: WEBSOCKET_EVENT_DATA
I (5752) socketio: Received data from server:
I (5762) socketio: EIO: 4
I (5762) socketio: SIO: 3
I (5762) socketio: Namespace: /chat
I (5772) socketio: Event ID: 0
I (5772) socketio: [{
		"status":	"ok"
	}]
I (5782) socketio_client: WEBSOCKET_EVENT_DATA
I (5782) socketio_client: Received opcode=1
W (5792) socketio_client: Received=452-/chat,["hello",1,"2",{"3":"4","5":{"_placeholder":true,"num":0},"6":{"_placeholder":true,"num":1}}]
I (5802) socketio_client: Total payload length=103, data_len=103, current payload offset=0

I (5812) socketio_packet: Custom namespace: "/chat"
I (5822) socketio_client: WEBSOCKET_EVENT_DATA
I (5822) socketio_client: Received opcode=2
W (5822) socketio_client: Received=
I (5832) socketio_client: Total payload length=2, data_len=2, current payload offset=0

I (5842) socketio_client: Received binary
I (5842) socketio_client: WEBSOCKET_EVENT_DATA
I (5852) socketio_client: Received opcode=2
W (5852) socketio_client: Received=0123
I (5862) socketio_client: Total payload length=4, data_len=4, current payload offset=0

I (5862) socketio_client: Received binary
I (5872) socketio: WEBSOCKET_EVENT_DATA
I (5872) socketio: Received data from server:
I (5882) socketio: EIO: 4
I (5882) socketio: SIO: 5
I (5892) socketio: Namespace: /chat
I (5892) socketio: Event ID: -1
I (5892) socketio: ["hello", 1, "2", {
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
I (16052) socketio_client: WEBSOCKET_EVENT_DATA
I (16052) socketio_client: Received opcode=10
W (16052) socketio_client: Received=
W (16052) socketio_client: Received WS PONG
I (16062) socketio_client: Total payload length=0, data_len=0, current payload offset=0
```
