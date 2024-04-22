/*!
 * @file
 * @brief
 */

#include "CppUTestExt/MockSupport.h"
#include "double/mqtt_client_double.hpp"

static void register_erd(i_mqtt_client_t* self, tiny_erd_t erd)
{
  mock()
    .actualCall("register_erd")
    .onObject(self)
    .withParameter("erd", erd);
}

static void update_erd(i_mqtt_client_t* self, tiny_erd_t erd, const void* value, uint8_t size)
{
  mock()
    .actualCall("update_erd")
    .onObject(self)
    .withParameter("erd", erd)
    .withMemoryBufferParameter("value", reinterpret_cast<const uint8_t*>(value), size);
}

static i_tiny_event_t* on_write_request(i_mqtt_client_t* _self)
{
  auto self = reinterpret_cast<mqtt_client_double_t*>(_self);
  return &self->on_write_request.interface;
}

static i_tiny_event_t* on_mqtt_disconnect(i_mqtt_client_t* _self)
{
  auto self = reinterpret_cast<mqtt_client_double_t*>(_self);
  return &self->on_mqtt_disconnect.interface;
}

static const i_mqtt_client_api_t api = { register_erd, update_erd, on_write_request, on_mqtt_disconnect };

void mqtt_client_double_init(mqtt_client_double_t* self)
{
  self->interface.api = &api;
  tiny_event_init(&self->on_write_request);
  tiny_event_init(&self->on_mqtt_disconnect);
}

void mqtt_client_double_trigger_write_request(
  mqtt_client_double_t* self,
  tiny_erd_t erd,
  uint8_t size,
  const void* value)
{
  mqtt_client_on_write_request_args_t args = { erd, size, value };
  tiny_event_publish(&self->on_write_request, &args);
}

void mqtt_client_double_trigger_mqtt_disconnect(
  mqtt_client_double_t* self)
{
  tiny_event_publish(&self->on_mqtt_disconnect, nullptr);
}
