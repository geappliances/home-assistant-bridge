/*!
 * @file
 * @brief
 *
 * Copyright GE Appliances - Confidential - All rights reserved.
 */

#include "HomeAssistantBridge.h"

extern "C" {
#include "tiny_time_source.h"
}

static const tiny_erd_client_configuration_t client_configuration = {
  .request_timeout = 250,
  .request_retries = 10
};

void HomeAssistantBridge::begin(PubSubClient& pubSubClient, Stream& uart, const char* deviceId, uint8_t clientAddress)
{
  this->pubSubClient = &pubSubClient;

  tiny_timer_group_init(&timer_group, tiny_time_source_init());

  tiny_uart_adapter_init(&uart_adapter, &timer_group, uart);

  mqtt_client_adapter_init(&client_adapter, &pubSubClient, deviceId);

  tiny_gea3_interface_init(
    &gea3_interface,
    &uart_adapter.interface,
    clientAddress,
    send_buffer,
    sizeof(send_buffer),
    receive_buffer,
    sizeof(receive_buffer),
    send_queue_buffer,
    sizeof(send_queue_buffer));

  tiny_erd_client_init(
    &erd_client,
    &timer_group,
    &gea3_interface.interface,
    client_queue_buffer,
    sizeof(client_queue_buffer),
    &client_configuration);

  mqtt_bridge_init(
    &mqtt_bridge,
    &timer_group,
    &erd_client.interface,
    &client_adapter.interface);
}

void HomeAssistantBridge::loop()
{
  pubSubClient->loop();
  tiny_timer_group_run(&timer_group);
  tiny_gea3_interface_run(&gea3_interface);
}

void HomeAssistantBridge::notifyMqttDisconnected()
{
  mqtt_client_adapter_notify_mqtt_disconnected(&client_adapter);
}
