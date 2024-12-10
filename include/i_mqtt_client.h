/*!
 * @file
 * @brief
 */

#ifndef i_mqtt_client_h
#define i_mqtt_client_h

#include "i_tiny_event.h"
#include "i_tiny_gea3_erd_client.h"
#include "tiny_erd.h"

typedef struct {
  tiny_erd_t erd;
  uint8_t size;
  const void* value;
} mqtt_client_on_write_request_args_t;

struct i_mqtt_client_api_t;

typedef struct {
  const struct i_mqtt_client_api_t* api;
} i_mqtt_client_t;

typedef struct i_mqtt_client_api_t {
  void (*register_erd)(i_mqtt_client_t* self, tiny_erd_t erd);

  void (*update_erd)(i_mqtt_client_t* self, tiny_erd_t erd, const void* value, uint8_t size);

  void (*update_erd_write_result)(i_mqtt_client_t* self, tiny_erd_t erd, bool success, tiny_gea3_erd_client_write_failure_reason_t failure_reason);

  i_tiny_event_t* (*on_write_request)(i_mqtt_client_t* self);

  i_tiny_event_t* (*on_mqtt_disconnect)(i_mqtt_client_t* self);
} i_mqtt_client_api_t;

/*!
 * Register a newly discovered ERD.
 */
static inline void mqtt_client_register_erd(i_mqtt_client_t* self, tiny_erd_t erd)
{
  self->api->register_erd(self, erd);
}

/*!
 * Provide an updated value for a previously registered ERD.
 */
static inline void mqtt_client_update_erd(i_mqtt_client_t* self, tiny_erd_t erd, const void* value, uint8_t size)
{
  self->api->update_erd(self, erd, value, size);
}

/*!
 * Provide the result for the most recently completed write request to an ERD.
 */
static inline void mqtt_client_update_erd_write_result(i_mqtt_client_t* self, tiny_erd_t erd, bool success, tiny_gea3_erd_client_write_failure_reason_t failure_reason)
{
  self->api->update_erd_write_result(self, erd, success, failure_reason);
}

/*!
 * Event raised when a write request is received from the MQTT broker.
 */
static inline i_tiny_event_t* mqtt_client_on_write_request(i_mqtt_client_t* self)
{
  return self->api->on_write_request(self);
}

/*!
 * Event raised when the client disconnects from the MQTT broker.
 */
static inline i_tiny_event_t* mqtt_client_on_mqtt_disconnect(i_mqtt_client_t* self)
{
  return self->api->on_mqtt_disconnect(self);
}

#endif
