#ifndef __MQTT_STREAM
#define __MQTT_STREAM

class MQTTStream {
public:
	MQTTStream(){}
	virtual ~MQTTStream(){}
    virtual bool connect(const char *protocol, const char *addr, uint32_t port) = 0;
    virtual bool disconnect(void) = 0;
    virtual bool write(const uint8_t *buffer, uint32_t len) = 0;
    virtual uint32_t read(uint8_t *buffer, uint32_t buffer_size, uint32_t timeout) = 0;
};
#endif