# home-assistant-bridge
[![Tests](https://github.com/geappliances/home-assistant-bridge/actions/workflows/test.yml/badge.svg)](https://github.com/geappliances/home-assistant-bridge/actions/workflows/test.yml)

Subscribes to data hosted by a GE Appliances product using GEA3 and publishes it to an MQTT server under `/geappliances/<device ID>`. ERDs are identified by 16-bit identifiers and the raw binary data is published as a hex string to `/geappliances/<device ID>/erd/<ERD ID>/value`. Data can be written to an ERD by writing a hex string of the appropriate size to `/geappliances/<device ID>/erd/<ERD ID>/write`.

This is intended to be used with the MQTT server provided by Home Assistant, but it should work with other MQTT servers.
