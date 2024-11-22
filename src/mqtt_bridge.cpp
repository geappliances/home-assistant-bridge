/*!
 * @file
 * @brief
 */

extern "C" {
#include "mqtt_bridge.h"
#include "tiny_utils.h"
}

#include <set>

using namespace std;

enum {
  erd_host_address = 0xC0,
  resubscribe_delay = 1000,
  subscription_retention_period = 30 * 1000
};

enum {
  signal_start = tiny_hsm_signal_user_start,
  signal_timer_expired,
  signal_subscription_failed,
  signal_subscription_added_or_retained,
  signal_subscription_host_came_online,
  signal_subscription_publication_received,
  signal_mqtt_disconnected,
  signal_write_requested
};

static void arm_timer(mqtt_bridge_t* self, tiny_timer_ticks_t ticks)
{
  tiny_timer_start(
    self->timer_group, &self->timer, ticks, self, +[](void* context) {
      tiny_hsm_send_signal(&reinterpret_cast<mqtt_bridge_t*>(context)->hsm, signal_timer_expired, nullptr);
    });
}

static void arm_periodic_timer(mqtt_bridge_t* self, tiny_timer_ticks_t ticks)
{
  tiny_timer_start_periodic(
    self->timer_group, &self->timer, ticks, self, +[](void* context) {
      tiny_hsm_send_signal(&reinterpret_cast<mqtt_bridge_t*>(context)->hsm, signal_timer_expired, nullptr);
    });
}

static void disarm_timer(mqtt_bridge_t* self)
{
  tiny_timer_stop(self->timer_group, &self->timer);
}

static set<tiny_erd_t>& erd_set(mqtt_bridge_t* self)
{
  return *reinterpret_cast<set<tiny_erd_t>*>(self->erd_set);
}

static tiny_hsm_result_t state_top(tiny_hsm_t* hsm, tiny_hsm_signal_t signal, const void* data);
static tiny_hsm_result_t state_subscribing(tiny_hsm_t* hsm, tiny_hsm_signal_t signal, const void* data);
static tiny_hsm_result_t state_subscribed(tiny_hsm_t* hsm, tiny_hsm_signal_t signal, const void* data);

static tiny_hsm_result_t state_top(tiny_hsm_t* hsm, tiny_hsm_signal_t signal, const void* data)
{
  mqtt_bridge_t* self = container_of(mqtt_bridge_t, hsm, hsm);
  (void)self;

  switch(signal) {
    case signal_subscription_publication_received: {
      auto args = reinterpret_cast<const tiny_erd_client_on_activity_args_t*>(data);
      auto erd = args->subscription_publication_received.erd;

      if(erd_set(self).find(erd) == erd_set(self).end()) {
        mqtt_client_register_erd(self->mqtt_client, erd);
        erd_set(self).insert(erd);
      }

      mqtt_client_update_erd(
        self->mqtt_client,
        erd,
        args->subscription_publication_received.data,
        args->subscription_publication_received.data_size);
    } break;

    case signal_write_requested: {
      auto args = reinterpret_cast<const mqtt_client_on_write_request_args_t*>(data);
      tiny_erd_client_request_id_t request_id;
      tiny_erd_client_write(self->erd_client, &request_id, erd_host_address, args->erd, args->value, args->size);
    } break;

    default:
      return tiny_hsm_result_signal_deferred;
  }

  return tiny_hsm_result_signal_consumed;
}

static tiny_hsm_result_t state_subscribing(tiny_hsm_t* hsm, tiny_hsm_signal_t signal, const void* data)
{
  mqtt_bridge_t* self = container_of(mqtt_bridge_t, hsm, hsm);
  (void)data;

  switch(signal) {
    case tiny_hsm_signal_entry:
    case signal_subscription_failed:
    case signal_timer_expired:
      if(!tiny_erd_client_subscribe(self->erd_client, erd_host_address)) {
        arm_timer(self, resubscribe_delay);
      }
      break;

    case signal_subscription_added_or_retained:
      tiny_hsm_transition(hsm, state_subscribed);
      break;

    case tiny_hsm_signal_exit:
      disarm_timer(self);
      break;

    default:
      return tiny_hsm_result_signal_deferred;
  }

  return tiny_hsm_result_signal_consumed;
}

static tiny_hsm_result_t state_subscribed(tiny_hsm_t* hsm, tiny_hsm_signal_t signal, const void* data)
{
  mqtt_bridge_t* self = container_of(mqtt_bridge_t, hsm, hsm);
  (void)data;
  (void)self;

  switch(signal) {
    case tiny_hsm_signal_entry:
      arm_periodic_timer(self, subscription_retention_period);
      break;

    case signal_timer_expired:
      tiny_erd_client_retain_subscription(self->erd_client, erd_host_address);
      break;

    case signal_subscription_host_came_online:
    case signal_mqtt_disconnected:
      tiny_hsm_transition(hsm, state_subscribing);
      break;

    case tiny_hsm_signal_exit:
      disarm_timer(self);
      break;

    default:
      return tiny_hsm_result_signal_deferred;
  }

  return tiny_hsm_result_signal_consumed;
}

static const tiny_hsm_state_descriptor_t hsm_state_descriptors[] = {
  { .state = state_top, .parent = nullptr },
  { .state = state_subscribing, .parent = state_top },
  { .state = state_subscribed, .parent = state_top }
};
static const tiny_hsm_configuration_t hsm_configuration = {
  .states = hsm_state_descriptors,
  .state_count = element_count(hsm_state_descriptors)
};

void mqtt_bridge_init(
  mqtt_bridge_t* self,
  tiny_timer_group_t* timer_group,
  i_tiny_erd_client_t* erd_client,
  i_mqtt_client_t* mqtt_client)
{
  self->timer_group = timer_group;
  self->erd_client = erd_client;
  self->mqtt_client = mqtt_client;
  self->erd_set = reinterpret_cast<void*>(new set<tiny_erd_t>());

  tiny_event_subscription_init(
    &self->erd_client_activity_subscription, self, +[](void* context, const void* _args) {
      auto self = reinterpret_cast<mqtt_bridge_t*>(context);
      auto args = reinterpret_cast<const tiny_erd_client_on_activity_args_t*>(_args);

      if(args->address != erd_host_address) {
        return;
      }

      switch(args->type) {
        case tiny_erd_client_activity_type_subscription_added_or_retained:
          tiny_hsm_send_signal(&self->hsm, signal_subscription_added_or_retained, nullptr);
          break;

        case tiny_erd_client_activity_type_subscription_publication_received:
          tiny_hsm_send_signal(&self->hsm, signal_subscription_publication_received, args);
          break;

        case tiny_erd_client_activity_type_subscription_host_came_online:
          tiny_hsm_send_signal(&self->hsm, signal_subscription_host_came_online, nullptr);
          break;

        case tiny_erd_client_activity_type_subscribe_failed:
          tiny_hsm_send_signal(&self->hsm, signal_subscription_failed, nullptr);
          break;

        case tiny_erd_client_activity_type_write_completed:
          mqtt_client_update_erd_write_result(self->mqtt_client, args->write_completed.erd, true, 0);
          break;

        case tiny_erd_client_activity_type_write_failed:
          mqtt_client_update_erd_write_result(self->mqtt_client, args->write_failed.erd, false, args->write_failed.reason);
          break;
      }
    });
  tiny_event_subscribe(tiny_erd_client_on_activity(erd_client), &self->erd_client_activity_subscription);

  tiny_event_subscription_init(
    &self->mqtt_write_request_subscription, self, +[](void* context, const void* _args) {
      auto self = reinterpret_cast<mqtt_bridge_t*>(context);
      auto args = reinterpret_cast<const mqtt_client_on_write_request_args_t*>(_args);
      tiny_hsm_send_signal(&self->hsm, signal_write_requested, args);
    });
  tiny_event_subscribe(mqtt_client_on_write_request(mqtt_client), &self->mqtt_write_request_subscription);

  tiny_event_subscription_init(
    &self->mqtt_disconnect_subscription, self, +[](void* context, const void*) {
      auto self = reinterpret_cast<mqtt_bridge_t*>(context);
      reinterpret_cast<set<tiny_erd_t>*>(self->erd_set)->clear();
      tiny_hsm_send_signal(&self->hsm, signal_mqtt_disconnected, nullptr);
    });
  tiny_event_subscribe(mqtt_client_on_mqtt_disconnect(mqtt_client), &self->mqtt_disconnect_subscription);

  tiny_hsm_init(&self->hsm, &hsm_configuration, state_subscribing);
}

void mqtt_bridge_destroy(mqtt_bridge_t* self)
{
  delete reinterpret_cast<set<tiny_erd_t>*>(self->erd_set);
}
