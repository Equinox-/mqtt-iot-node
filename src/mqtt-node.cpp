#include <mqtt-node.h>
#include <stopwatch.h>
#include <logging.h>
#include <cstring>

static void message_callback_static(struct umqtt_connection *uc, char *topic, uint8_t *data, int len);

MQTTNode::MQTTNode(MQTTStream *stream) {
	this->stream = stream;
  	this->u_conn.kalive = 30;
	this->u_conn.txbuff.start = u_txbuff;
	this->u_conn.txbuff.length = sizeof(u_txbuff);
	this->u_conn.rxbuff.start = u_rxbuff;
	this->u_conn.rxbuff.length = sizeof(u_rxbuff);
	this->u_conn.message_callback = message_callback_static;
	this->u_conn.data = this;
}

MQTTNode::~MQTTNode() {
	struct MQTTNodeSub *nd = first;
	struct MQTTNodeSub *tmp;
	while (nd != NULL) {
		tmp = nd->next;
		delete nd;
		nd = tmp;
	}
}


#define RECONNECT_STOPWATCH lastConnect
#define RECONNECT_RATE (2000) /* 2 sec */
STOPWATCH_INIT(RECONNECT_STOPWATCH);
void MQTTNode::reconnect() {
	if (!STOPWATCH_HAS_BEEN(RECONNECT_STOPWATCH, RECONNECT_RATE))
		return;
	LOG_F("Attempting to connect TCP to %s:%d\n", mqttHost, mqttPort);
	
	STOPWATCH_BEGIN(lastConnect);
	socketValid = false;
	this->stream->disconnect();
  
	if (!this->stream->connect("TCP", mqttHost, mqttPort)) {
		LOG_F("Failed to connect TCP to %s:%d\n", mqttHost, mqttPort);
		return;
	}

	socketValid = true;
	LOG_F("Connecting UMQTT to %s:%d with client id \"%s\"\n", mqttHost, mqttPort, u_conn.clientid);
	umqtt_connect(&u_conn);
}

void MQTTNode::attach(char *recvTopic, char *sendTopic, unsigned long interval, MQTTNodeSub_RECV recv, MQTTNodeSub_CHANGED changed, MQTTNodeSub_COMPOSE compose) {
	struct MQTTNodeSub *node = new struct MQTTNodeSub();

	node->recvTopic = recvTopic;
	node->sendTopic = sendTopic;
	node->lastSent = -1;
	node->interval = interval;
	node->recv = recv;
	node->changed = changed;
	node->compose = compose;


	node->prev = NULL;
	node->next = first;
	if (first != NULL)
		first->prev = NULL;
	this->first = node;
}

void MQTTNode::clientID(const char *clientid) {
	this->u_conn.clientid = clientid;
}

void MQTTNode::server(const char *host, unsigned int port) {
	this->mqttHost = host;
	this->mqttPort = port;
}

void MQTTNode::comesOnline() {
	struct MQTTNodeSub *sub = this->first;
	while (sub != NULL) {
		if (sub->recvTopic != NULL && sub->recv != NULL) {
			umqtt_subscribe(&u_conn, sub->recvTopic);
			LOG_F_VERB("Subscribing to %s\n", sub->recvTopic);
		}
		sub = sub->next;
	}
}

void MQTTNode::loopOnline() {
	struct MQTTNodeSub *sub = this->first;
	while (sub != NULL) {
		if (sub->sendTopic != NULL && sub->compose != NULL && (
			(sub->interval > 0 && STOPWATCH_HAS_BEEN(sub->lastSent, sub->interval)) || 
			(sub->changed != NULL && sub->changed()))) {
			int len = sub->compose(buff, sizeof(buff));
			umqtt_publish(&u_conn, sub->sendTopic, buff, len);
			LOG_F_VERB("Publishing \"%s\" on %s\n", buff, sub->sendTopic);
			STOPWATCH_BEGIN(sub->lastSent);
		}
		sub = sub->next;
	}
}

void MQTTNode::loop() {
	if (!socketValid) {
		reconnect();
	} else {
	    if (!umqtt_circ_is_empty(&u_conn.txbuff)) {
	      ret = umqtt_circ_pop(&u_conn.txbuff, buff, 1024);
	      if (!stream->write(buff, ret)) {
	      	LOG_F("Reset MQTT connection\n");
	      	socketValid = false;
	      }
	      LOG_F_VERB("Write %d bytes\n", ret);
	    }

	    ret = stream->read(buff, sizeof(buff), u_conn.kalive);
	    if (ret < 0) {
	      umqtt_ping(&u_conn);
	      LOG_F("UMQTT PING\n");
	    } else if (ret > 0) {
	      LOG_F_VERB("Read %d bytes\n", ret);
	    }
	    umqtt_circ_push(&u_conn.rxbuff, buff, ret);
	    umqtt_process(&u_conn);
	    if (u_conn.state == UMQTT_STATE_CONNECTED) {
	      if (!hasBeenOnline) {
	          LOG_F("Connected UMQTT\n");
	          comesOnline();
	          hasBeenOnline = true;
	      }
	      loopOnline();
	    } else {
	      	LOG_F("Reset MQTT connection\n");
	    	socketValid = false;
	    }
	}
}

void MQTTNode::message_callback_internal(char *topic, uint8_t *data, int len) {
	LOG_F_VERB("Recieved \"%s\" (%d) on %s\n", data, len, topic);
	struct MQTTNodeSub *sub = this->first;
	while (sub != NULL) {
		if (sub->recvTopic != NULL && sub->recv != NULL && strcmp(sub->recvTopic, topic) == 0) {
			sub->recv(data, len);
		}
		sub = sub->next;
	}
}

static void message_callback_static(struct umqtt_connection *uc, char *topic, uint8_t *data, int len) {
	if (uc != NULL && uc->data != NULL) {
		MQTTNode *node = (MQTTNode*) uc->data;
		node->message_callback_internal(topic, data, len);
	}
}