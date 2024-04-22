/*!
 * @file
 * @brief
 */

#ifndef mqtt_client_adapter_hpp
#define mqtt_client_adapter_hpp

#include <PubSubClient.h>
#include <cstdint>
#include <map>
#include <vector>

extern "C" {
#include "i_mqtt_client.h"
#include "tiny_event.h"
#include "tiny_timer.h"
}

typedef struct {
  i_mqtt_client_t interface;

  PubSubClient* client;
  const char* device_id;
  tiny_event_t write_request;
  tiny_event_t mqtt_disconnect;
} mqtt_client_adapter_t;

/*!
 * Initialize an adapter to expose an Arduino PubSubClient as an i_mqtt_client_t.
 */
void mqtt_client_adapter_init(mqtt_client_adapter_t* self, PubSubClient* client, const char* deviceId);

/*!
 * Notify the adapter that the MQTT client has connected. Should be called before reconnecting when connection is lost.
 */
void mqtt_client_adapter_notify_mqtt_disconnected(mqtt_client_adapter_t* self);

#endif
