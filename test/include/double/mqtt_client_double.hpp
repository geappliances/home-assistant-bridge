/*!
 * @file
 * @brief
 */

#ifndef mqtt_client_double_hpp
#define mqtt_client_double_hpp

extern "C" {
#include "i_mqtt_client.h"
#include "tiny_event.h"
}

typedef struct {
  i_mqtt_client_t interface;

  tiny_event_t on_write_request;
  tiny_event_t on_mqtt_disconnect;
} mqtt_client_double_t;

/*!
 * Initialize an MQTT client test double.
 */
void mqtt_client_double_init(mqtt_client_double_t* self);

/*!
 * Trigger publication via the on_write_request event.
 */
void mqtt_client_double_trigger_write_request(
  mqtt_client_double_t* self,
  tiny_erd_t erd,
  uint8_t size,
  const void* value);

/*!
 * Trigger publication via the on_mqtt_disconnect event.
 */
void mqtt_client_double_trigger_mqtt_disconnect(
  mqtt_client_double_t* self);

#endif
