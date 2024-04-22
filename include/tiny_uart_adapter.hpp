/*!
 * @file
 * @brief
 */

#ifndef tiny_uart_adapter_hpp
#define tiny_uart_adapter_hpp

#include <Arduino.h>

extern "C" {
#include "hal/i_tiny_uart.h"
#include "tiny_event.h"
#include "tiny_timer.h"
}

typedef struct {
  i_tiny_uart_t interface;

  Stream* uart;
  i_tiny_uart_t self;
  tiny_event_t send_complete_event;
  tiny_event_t receive_event;
  tiny_timer_group_t* timer_group;
  tiny_timer_t timer;
  bool sent;
} tiny_uart_adapter_t;

/*!
 * Initialize an adapter to expose an Arduino UART as an i_tiny_uart_t.
 */
void tiny_uart_adapter_init(tiny_uart_adapter_t* self, tiny_timer_group_t* timer_group, Stream& uart);

#endif
