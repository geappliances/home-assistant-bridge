/*!
 * @file
 * @brief
 */

#include "mqtt_client_adapter.hpp"

static uint8_t ascii_hex_to_nybble(char ascii_hex)
{
  if('A' <= ascii_hex && ascii_hex <= 'F') {
    return ascii_hex - 'A' + 10;
  }
  else if('a' <= ascii_hex && ascii_hex <= 'f') {
    return ascii_hex - 'a' + 10;
  }
  else {
    return ascii_hex - '0';
  }
}

// Terrible hack needed because PubSubClient doesn't allow a context to be provided
static mqtt_client_adapter_t* mqtt_callback_self;

static void mqtt_callback(char* topic, byte* payload, unsigned int length)
{
  auto self = mqtt_callback_self;

  auto topic_string = String(topic);

  if(topic_string.startsWith("geappliances/") && topic_string.endsWith("/write")) {
    auto erdString = topic_string.substring(topic_string.lastIndexOf("/erd/0x") + 7, topic_string.lastIndexOf("/write"));
    tiny_erd_t erd;

    erd = ascii_hex_to_nybble(erdString[0]);
    erd = (erd << 4) + ascii_hex_to_nybble(erdString[1]);
    erd = (erd << 4) + ascii_hex_to_nybble(erdString[2]);
    erd = (erd << 4) + ascii_hex_to_nybble(erdString[3]);

    for(unsigned i = 0; i < length / 2; i++) {
      auto high = payload[2 * i];
      auto low = payload[2 * i + 1];
      payload[i] = (ascii_hex_to_nybble(high) << 4) | ascii_hex_to_nybble(low);
    }

    mqtt_client_on_write_request_args_t args = {
      .erd = erd,
      .size = static_cast<uint8_t>(length / 2),
      .value = payload,
    };
    tiny_event_publish(&self->write_request, &args);
  }
}

static void register_erd(i_mqtt_client_t* _self, tiny_erd_t erd)
{
  auto self = reinterpret_cast<mqtt_client_adapter_t*>(_self);

  auto erd_string = String(erd, HEX);
  while(erd_string.length() < 4) {
    erd_string = "0" + erd_string;
  }

  auto topic = String("geappliances/") + self->device_id + "/erd/0x" + erd_string + "/write";
  self->client->subscribe(topic.c_str());
}

static void update_erd(i_mqtt_client_t* _self, tiny_erd_t erd, const void* _value, uint8_t size)
{
  auto self = reinterpret_cast<mqtt_client_adapter_t*>(_self);

  auto payload = String();
  auto bytes = reinterpret_cast<const uint8_t*>(_value);

  for(unsigned i = 0; i < size; i++) {
    auto hex = String(bytes[i], HEX);
    if(hex.length() == 1) {
      payload += "0";
    }
    payload += hex;
  }

  auto erd_string = String(erd, HEX);
  while(erd_string.length() < 4) {
    erd_string = "0" + erd_string;
  }

  auto topic = String("geappliances/") + self->device_id + "/erd/0x" + erd_string + "/value";
  self->client->publish(topic.c_str(), payload.c_str(), true);
}

static void update_erd_write_result(i_mqtt_client_t* _self, tiny_erd_t erd, bool success, tiny_erd_client_write_failure_reason_t failure_reason)
{
  auto self = reinterpret_cast<mqtt_client_adapter_t*>(_self);

  auto erd_string = String(erd, HEX);
  while(erd_string.length() < 4) {
    erd_string = "0" + erd_string;
  }

  auto topic = String("geappliances/") + self->device_id + "/erd/0x" + erd_string + "/write_result";

  if(success) {
    self->client->publish(topic.c_str(), "success", true);
  }
  else {
    switch(failure_reason) {
      case tiny_erd_client_write_failure_reason_retries_exhausted:
        self->client->publish(topic.c_str(), "retries exhausted", true);
        break;

      case tiny_erd_client_write_failure_reason_not_supported:
        self->client->publish(topic.c_str(), "not supported", true);
        break;

      case tiny_erd_client_write_failure_reason_incorrect_size:
        self->client->publish(topic.c_str(), "incorrect size", true);
        break;

      default:
        self->client->publish(topic.c_str(), "unknown", true);
        break;
    }
  }
}

static i_tiny_event_t* on_write_request(i_mqtt_client_t* _self)
{
  auto self = reinterpret_cast<mqtt_client_adapter_t*>(_self);
  return &self->write_request.interface;
}

static i_tiny_event_t* on_mqtt_disconnect(i_mqtt_client_t* _self)
{
  auto self = reinterpret_cast<mqtt_client_adapter_t*>(_self);
  return &self->mqtt_disconnect.interface;
}

static const i_mqtt_client_api_t api = {
  register_erd,
  update_erd,
  update_erd_write_result,
  on_write_request,
  on_mqtt_disconnect
};

void mqtt_client_adapter_init(mqtt_client_adapter_t* self, PubSubClient* client, const char* deviceId)
{
  self->interface.api = &api;
  self->client = client;
  self->device_id = deviceId;

  mqtt_callback_self = self;
  client->setCallback(mqtt_callback);

  tiny_event_init(&self->write_request);
  tiny_event_init(&self->mqtt_disconnect);
}

void mqtt_client_adapter_notify_mqtt_disconnected(mqtt_client_adapter_t* self)
{
  tiny_event_publish(&self->mqtt_disconnect, nullptr);
}
