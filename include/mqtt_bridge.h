/*!
 * @file
 * @brief Pushes published ERDs to and fulfills write requests from an MQTT server.
 */

#ifndef mqtt_bridge_h
#define mqtt_bridge_h

#include "i_mqtt_client.h"
#include "i_tiny_gea3_erd_client.h"
#include "tiny_hsm.h"
#include "tiny_timer.h"

typedef struct {
  tiny_timer_group_t* timer_group;
  i_tiny_gea3_erd_client_t* erd_client;
  i_mqtt_client_t* mqtt_client;
  tiny_timer_t timer;
  tiny_event_subscription_t mqtt_write_request_subscription;
  tiny_event_subscription_t mqtt_disconnect_subscription;
  tiny_event_subscription_t erd_client_activity_subscription;
  void* erd_set;
  tiny_hsm_t hsm;
} mqtt_bridge_t;

/*!
 * Initialize the MQTT bridge.
 */
void mqtt_bridge_init(
  mqtt_bridge_t* self,
  tiny_timer_group_t* timer_group,
  i_tiny_gea3_erd_client_t* erd_client,
  i_mqtt_client_t* mqtt_client);

/*!
 * Destroy the MQTT bridge.
 */
void mqtt_bridge_destroy(
  mqtt_bridge_t* self);

#endif
