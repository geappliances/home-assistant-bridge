/*!
 * @file
 * @brief
 */

#include "tiny_uart_adapter.hpp"

static void poll(void* context)
{
  auto self = static_cast<tiny_uart_adapter_t*>(context);

  int rxBytes = self->uart->available();

  while(rxBytes--) {
    int byte = self->uart->read();
    tiny_uart_on_receive_args_t args = { (uint8_t)byte };
    tiny_event_publish(&self->receive_event, &args);
  }

  if(self->sent) {
    self->sent = false;
    tiny_event_publish(&self->send_complete_event, NULL);
  }
}

static void send(i_tiny_uart_t* _self, uint8_t byte)
{
  auto self = reinterpret_cast<tiny_uart_adapter_t*>(_self);
  self->sent = true;
  self->uart->write(byte);
}

static i_tiny_event_t* on_send_complete(i_tiny_uart_t* _self)
{
  auto self = reinterpret_cast<tiny_uart_adapter_t*>(_self);
  return &self->send_complete_event.interface;
}

static i_tiny_event_t* on_receive(i_tiny_uart_t* _self)
{
  auto self = reinterpret_cast<tiny_uart_adapter_t*>(_self);
  return &self->receive_event.interface;
}

static const i_tiny_uart_api_t api = { send, on_send_complete, on_receive };

void tiny_uart_adapter_init(tiny_uart_adapter_t* self, tiny_timer_group_t* timer_group, Stream& uart)
{
  self->interface.api = &api;

  self->uart = &uart;
  self->timer_group = timer_group;

  tiny_event_init(&self->send_complete_event);
  tiny_event_init(&self->receive_event);

  tiny_timer_start_periodic(timer_group, &self->timer, 0, self, poll);
}
