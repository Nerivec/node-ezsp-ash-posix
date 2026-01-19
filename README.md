# Node EZSP and ASH protocols for POSIX

Node.js N-API bindings for Silicon Labs EmberZNet Serial Protocol (EZSP) and Asynchronous Serial Host (ASH) protocols of [Simplicity SDK](https://github.com/SiliconLabs/simplicity_sdk).

[SDK sources](https://github.com/SiliconLabs/simplicity_sdk/blob/HEAD/protocol/zigbee/app/ezsp-host/ezsp-host-io.c) are POSIX-compatible regarding serial operations.

> [!IMPORTANT]
> Experimental: the entire protocol is not yet supported.

Mainly intended as a (mostly) drop-in replacement for [zigbee-herdsman](https://github.com/Koenkk/zigbee-herdsman) pure Node.js Ember driver.
