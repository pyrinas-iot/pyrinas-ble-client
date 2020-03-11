#include "app.h"

timer_define(m_sensor_timer);

// Catch events sent over Bluetooth
static void ble_evt(char *name, char *data)
{
  NRF_LOG_INFO("%s: %s", name, data);
}

static void sensor_timer_evt()
{
  // Check if we're connected
  if (ble_is_connected())
  {
    // Sends "ping" with the event name of "data"
    ble_publish("data", "ping");
  }
}

void setup()
{
  BLE_STACK_PERIPH_DEF(init);

  // Configuration for ble stack
  ble_stack_init(&init);

  // Setup BLE callback
  ble_subscribe("data", ble_evt);

  // Start advertising
  advertising_start();

  // Sensor sensor timer.
  timer_create(&m_sensor_timer, TIMER_REPEATED, sensor_timer_evt);

  // Start
  timer_start(&m_sensor_timer, 1000);

  // Print the address
  util_print_device_address();
}

void loop()
{
}