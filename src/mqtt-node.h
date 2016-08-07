#ifndef __MQTT_NODE_H
#define __MQTT_NODE_H

#ifndef NULL
#define NULL (0)
#endif

#include <umqtt.h>
#include <mqtt-stream.h>

#define RX_TX_BUFFER_LEN (2048)

typedef void (*MQTTNodeSub_RECV)(uint8_t *data, int len);
typedef bool (*MQTTNodeSub_CHANGED)();
typedef int (*MQTTNodeSub_COMPOSE)(uint8_t *data, int len);

struct MQTTNodeSub {
	struct MQTTNodeSub* next;
	struct MQTTNodeSub* prev;

	char *recvTopic;
	char *sendTopic;

	unsigned long lastSent;
	unsigned long interval;

	MQTTNodeSub_RECV recv; 
	MQTTNodeSub_CHANGED changed;
	MQTTNodeSub_COMPOSE compose;
};

class MQTTNode {
public:
	MQTTNode(MQTTStream *stream);
	virtual ~MQTTNode();

	bool connected() { return u_conn.state == UMQTT_STATE_CONNECTED; }
	void attach(char *recvTopic, char *sendTopic, unsigned long interval, MQTTNodeSub_RECV recv, MQTTNodeSub_CHANGED changed, MQTTNodeSub_COMPOSE compose);
	void clientID(const char *clientID);
	void server(const char *mqttHost, unsigned int mqttPort);

	void message_callback_internal(char *topic, uint8_t *data, int len);
	void loop();
private:
	void reconnect();
	void comesOnline();
	void loopOnline();
private:
	const char *mqttHost;
	unsigned int mqttPort;
	MQTTStream *stream;

	MQTTNodeSub *first = NULL;

	uint8_t u_rxbuff[RX_TX_BUFFER_LEN];
	uint8_t u_txbuff[RX_TX_BUFFER_LEN];
	struct umqtt_connection u_conn;
	uint8_t ret;
	uint8_t buff[1024];
	bool hasBeenOnline = false;
	bool socketValid = false;
};
#endif