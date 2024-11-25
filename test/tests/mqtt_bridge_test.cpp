/*!
 * @file
 * @brief
 */

extern "C" {
#include "mqtt_bridge.h"
}

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "double/mqtt_client_double.hpp"
#include "double/tiny_erd_client_double.hpp"
#include "double/tiny_timer_group_double.hpp"

TEST_GROUP(mqtt_bridge)
{
  enum {
    resubscribe_delay = 1000,
    subscription_retention_period = 30 * 1000
  };

  mqtt_bridge_t self;

  tiny_timer_group_double_t timer_group;
  tiny_erd_client_double_t erd_client;
  mqtt_client_double_t mqtt_client;

  void setup()
  {
    mock().strictOrder();

    tiny_timer_group_double_init(&timer_group);
    tiny_erd_client_double_init(&erd_client);
    mqtt_client_double_init(&mqtt_client);
  }

  void teardown()
  {
    mqtt_bridge_destroy(&self);
  }

  void when_the_bridge_is_initialized()
  {
    mqtt_bridge_init(
      &self,
      &timer_group.timer_group,
      &erd_client.interface,
      &mqtt_client.interface);
  }

  void given_that_the_bridge_has_been_initialized()
  {
    mock().disable();
    when_the_bridge_is_initialized();
    mock().enable();
  }

  void after_a_subscription_is_added_or_retained_for(uint8_t address)
  {
    tiny_erd_client_on_activity_args_t args;
    args.type = tiny_erd_client_activity_type_subscription_added_or_retained;
    args.address = address;

    tiny_erd_client_double_trigger_activity_event(
      &erd_client,
      &args);
  }

  void given_that_a_subscription_has_been_added_or_retained_successfully_for(uint8_t address)
  {
    mock().disable();
    after_a_subscription_is_added_or_retained_for(address);
    mock().enable();
  }

  void given_that_the_bridge_has_been_initialized_and_a_subscription_is_active_for(uint8_t address)
  {
    given_that_the_bridge_has_been_initialized();
    given_that_a_subscription_has_been_added_or_retained_successfully_for(address);
  }

  void a_subscription_to_should_be_requested_for(uint8_t address)
  {
    mock()
      .expectOneCall("subscribe")
      .onObject(&erd_client)
      .withParameter("address", address)
      .andReturnValue(true);
  }

  void a_subscription_should_be_requested_and_will_fail_to_queue_for(uint8_t address)
  {
    mock()
      .expectOneCall("subscribe")
      .onObject(&erd_client)
      .withParameter("address", address)
      .andReturnValue(false);
  }

  void a_subscription_retention_should_be_requested_for(uint8_t address)
  {
    mock()
      .expectOneCall("retain_subscription")
      .onObject(&erd_client)
      .withParameter("address", address)
      .andReturnValue(true);
  }

  void should_register_erd(tiny_erd_t erd)
  {
    mock()
      .expectOneCall("register_erd")
      .onObject(&mqtt_client)
      .withParameter("erd", erd);
  }

  template <typename T>
  void should_update_erd(tiny_erd_t erd, T value)
  {
    static T _value;
    _value = value;

    mock()
      .expectOneCall("update_erd")
      .onObject(&mqtt_client)
      .withParameter("erd", erd)
      .withMemoryBufferParameter("value", reinterpret_cast<const uint8_t*>(&_value), sizeof(_value));
  }

  template <typename T>
  void when_an_erd_publication_is_received(uint8_t publisher_address, tiny_erd_t erd, T data)
  {
    tiny_erd_client_on_activity_args_t args;
    args.type = tiny_erd_client_activity_type_subscription_publication_received;
    args.address = publisher_address;
    args.subscription_publication_received.erd = erd;
    args.subscription_publication_received.data = &data;
    args.subscription_publication_received.data_size = sizeof(data);

    tiny_erd_client_double_trigger_activity_event(
      &erd_client,
      &args);
  }

  template <typename T>
  void given_that_an_erd_publication_has_been_received(uint8_t publisher_address, tiny_erd_t erd, T data)
  {
    mock().disable();
    when_an_erd_publication_is_received(publisher_address, erd, data);
    mock().enable();
  }

  void when_a_subscription_host_came_online_is_received_for(uint8_t address)
  {
    tiny_erd_client_on_activity_args_t args;
    args.type = tiny_erd_client_activity_type_subscription_host_came_online;
    args.address = address;

    tiny_erd_client_double_trigger_activity_event(
      &erd_client,
      &args);
  }

  void when_a_subscribe_failure_is_received_for(uint8_t address)
  {
    tiny_erd_client_on_activity_args_t args;
    args.type = tiny_erd_client_activity_type_subscribe_failed;
    args.address = address;

    tiny_erd_client_double_trigger_activity_event(
      &erd_client,
      &args);
  }

  template <typename T>
  void should_request_erd_write(uint8_t address, tiny_erd_t erd, T value)
  {
    static T _value;
    _value = value;

    mock()
      .expectOneCall("write")
      .onObject(&erd_client)
      .withParameter("address", address)
      .withParameter("erd", erd)
      .withMemoryBufferParameter("data", reinterpret_cast<const uint8_t*>(&_value), sizeof(_value))
      .ignoreOtherParameters()
      .andReturnValue(true);
  }

  template <typename T>
  void when_a_write_request_is_received(tiny_erd_t erd, T value)
  {
    mqtt_client_double_trigger_write_request(&mqtt_client, erd, sizeof(value), &value);
  }

  void should_update_erd_write_result(tiny_erd_t erd, bool success, tiny_erd_client_write_failure_reason_t reason)
  {
    mock()
      .expectOneCall("update_erd_write_result")
      .onObject(&mqtt_client)
      .withParameter("erd", erd)
      .withParameter("success", success)
      .withParameter("failure_reason", reason);
  }

  template <typename T>
  void when_a_write_request_completes_successfully(uint8_t address, tiny_erd_t erd, T value)
  {
    tiny_erd_client_on_activity_args_t args;
    args.type = tiny_erd_client_activity_type_write_completed;
    args.address = address;
    args.write_completed.erd = erd;
    args.write_completed.data = &value;
    args.write_completed.data_size = sizeof(value);
    tiny_erd_client_double_trigger_activity_event(&erd_client, &args);
  }

  template <typename T>
  void when_a_write_request_completes_unsuccessfully(uint8_t address, tiny_erd_t erd, T value, tiny_erd_client_write_failure_reason_t reason)
  {
    tiny_erd_client_on_activity_args_t args;
    args.type = tiny_erd_client_activity_type_write_failed;
    args.address = address;
    args.write_failed.erd = erd;
    args.write_failed.data = &value;
    args.write_failed.data_size = sizeof(value);
    args.write_failed.reason = reason;
    tiny_erd_client_double_trigger_activity_event(&erd_client, &args);
  }

  void after_mqtt_disconnects()
  {
    mqtt_client_double_trigger_mqtt_disconnect(&mqtt_client);
  }

  void given_that_mqtt_has_disconnected()
  {
    mock().disable();
    after_mqtt_disconnects();
    mock().enable();
  }

  void after(tiny_timer_ticks_t ticks)
  {
    tiny_timer_group_double_elapse_time(&timer_group, ticks);
  }

  void nothing_should_happen()
  {
  }
};

TEST(mqtt_bridge, should_subscribe_when_initialized)
{
  a_subscription_to_should_be_requested_for(0xC0);
  when_the_bridge_is_initialized();
}

TEST(mqtt_bridge, should_retry_subscribe_after_a_delay_if_the_subscribe_request_fails_to_queue)
{
  a_subscription_should_be_requested_and_will_fail_to_queue_for(0xC0);
  when_the_bridge_is_initialized();

  nothing_should_happen();
  after(resubscribe_delay - 1);

  a_subscription_should_be_requested_and_will_fail_to_queue_for(0xC0);
  after(1);

  a_subscription_to_should_be_requested_for(0xC0);
  after(resubscribe_delay);
}

TEST(mqtt_bridge, should_retry_subscribe_if_the_subscribe_request_fails)
{
  given_that_the_bridge_has_been_initialized();
  a_subscription_to_should_be_requested_for(0xC0);
  when_a_subscribe_failure_is_received_for(0xC0);
}

TEST(mqtt_bridge, should_not_retry_subscribe_if_the_subscribe_request_fails_for_a_different_address)
{
  given_that_the_bridge_has_been_initialized();
  nothing_should_happen();
  when_a_subscribe_failure_is_received_for(0xC1);
}

TEST(mqtt_bridge, should_resubscribe_after_receiving_a_subscription_host_came_online_from_the_erd_host)
{
  given_that_the_bridge_has_been_initialized_and_a_subscription_is_active_for(0xC0);
  a_subscription_to_should_be_requested_for(0xC0);
  when_a_subscription_host_came_online_is_received_for(0xC0);
}

TEST(mqtt_bridge, should_ignore_subscription_host_came_online_from_other_addresses)
{
  given_that_the_bridge_has_been_initialized_and_a_subscription_is_active_for(0xC0);
  nothing_should_happen();
  when_a_subscription_host_came_online_is_received_for(0xC1);
}

TEST(mqtt_bridge, should_ignore_subscription_added_activity_for_other_addresses)
{
  given_that_the_bridge_has_been_initialized();
  nothing_should_happen();
  after_a_subscription_is_added_or_retained_for(0xC1);
  after(subscription_retention_period);
}

TEST(mqtt_bridge, should_periodically_retain_an_active_subscription)
{
  given_that_the_bridge_has_been_initialized_and_a_subscription_is_active_for(0xC0);

  nothing_should_happen();
  after(subscription_retention_period - 1);

  a_subscription_retention_should_be_requested_for(0xC0);
  after(1);
}

TEST(mqtt_bridge, should_register_and_update_newly_discovered_erds_when_published_by_the_erd_client)
{
  given_that_the_bridge_has_been_initialized_and_a_subscription_is_active_for(0xC0);
  should_register_erd(0xABCD);
  should_update_erd(0xABCD, uint32_t(0x12345678));
  when_an_erd_publication_is_received(0xC0, 0xABCD, uint32_t(0x12345678));
}

TEST(mqtt_bridge, should_update_known_erds_when_published_by_the_erd_client)
{
  given_that_the_bridge_has_been_initialized_and_a_subscription_is_active_for(0xC0);
  given_that_an_erd_publication_has_been_received(0xC0, 0xABCD, uint32_t(0x12345678));
  should_update_erd(0xABCD, uint32_t(0x87654321));
  when_an_erd_publication_is_received(0xC0, 0xABCD, uint32_t(0x87654321));
}

// This makes sure that if we miss the ERD subscription added message that we still handle ERD publications
// Since the ERD client acknowledges publications even if a subscription isn't known to be active, this is
// necessary to make sure that we don't miss any ERD publications
TEST(mqtt_bridge, should_handle_erd_publications_even_when_a_subscription_is_not_confirmed_active)
{
  given_that_the_bridge_has_been_initialized();
  should_register_erd(0xABCD);
  should_update_erd(0xABCD, uint32_t(0x12345678));
  when_an_erd_publication_is_received(0xC0, 0xABCD, uint32_t(0x12345678));
}

TEST(mqtt_bridge, should_ignore_erd_publications_from_other_hosts)
{
  given_that_the_bridge_has_been_initialized();
  nothing_should_happen();
  when_an_erd_publication_is_received(0xC1, 0xABCD, uint32_t(0x12345678));
}

TEST(mqtt_bridge, should_forward_write_requests_from_the_mqtt_client)
{
  given_that_the_bridge_has_been_initialized();
  should_request_erd_write(0xC0, 0xABCD, uint32_t(0x12345678));
  when_a_write_request_is_received(0xABCD, uint32_t(0x12345678));
}

TEST(mqtt_bridge, should_report_write_results_to_the_mqtt_client)
{
  given_that_the_bridge_has_been_initialized();

  should_update_erd_write_result(0xABCD, true, 0);
  when_a_write_request_completes_successfully(0xC0, 0xABCD, uint32_t(0x12345678));

  should_update_erd_write_result(0xABCD, false, tiny_erd_client_write_failure_reason_not_supported);
  when_a_write_request_completes_unsuccessfully(0xC0, 0xABCD, uint32_t(0x12345678), tiny_erd_client_write_failure_reason_not_supported);
}

TEST(mqtt_bridge, should_register_and_update_newly_discovered_erds_when_published_by_the_erd_client_after_mqtt_reconnects)
{
  given_that_the_bridge_has_been_initialized_and_a_subscription_is_active_for(0xC0);
  given_that_an_erd_publication_has_been_received(0xC0, 0xABCD, uint32_t(0x12345678));
  given_that_mqtt_has_disconnected();
  should_register_erd(0xABCD);
  should_update_erd(0xABCD, uint32_t(0x12345678));
  when_an_erd_publication_is_received(0xC0, 0xABCD, uint32_t(0x12345678));
}

TEST(mqtt_bridge, should_resubscribe_after_mqtt_disconnects)
{
  given_that_the_bridge_has_been_initialized_and_a_subscription_is_active_for(0xC0);
  a_subscription_to_should_be_requested_for(0xC0);
  after_mqtt_disconnects();
}
