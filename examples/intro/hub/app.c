#include "app.h"

// Catch events sent over Bluetooth
static void ble_evt(char *name, char *data)
{
  NRF_LOG_INFO("%s: %s", name, data);

  ble_publish("data", "pong");
}

void setup()
{
  // Default config for central mode
  BLE_STACK_CENTRAL_DEF(init);

  // Add an addresses to scan for
  ble_gap_addr_t first = {
      .addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC,
      .addr = {0x81, 0x64, 0x4c, 0xad, 0x7d, 0xc0}};
  init.config.devices[0] = first;

  // Increment the device_count
  init.config.device_count = 1;

  // Configuration for ble stack
  ble_stack_init(&init);

  // Setup BLE callback
  ble_subscribe("data", ble_evt);

  // Start scanning.
  scan_start();
}

void loop()
{
}