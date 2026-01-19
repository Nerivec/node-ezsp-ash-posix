{
    "targets": [
        {
            "target_name": "ezsp_ash_posix",
            # "dependencies": [
            #     "<!(node -p \"require('node-addon-api').targets\"):node_addon_api",
            # ],
            "sources": [
                "src/native/binding.cpp",
                # SDK EZSP sources
                "simplicity_sdk/protocol/zigbee/app/util/ezsp/ezsp.c",
                "simplicity_sdk/protocol/zigbee/app/util/ezsp/ezsp-callbacks.c",
                "simplicity_sdk/protocol/zigbee/app/util/ezsp/ezsp-enum-decode.c",
                "simplicity_sdk/protocol/zigbee/app/util/ezsp/ezsp-frame-utilities.c",
                "simplicity_sdk/protocol/zigbee/app/util/ezsp/serial-interface-uart.c",
                # SDK EZSP-host sources
                "simplicity_sdk/protocol/zigbee/app/ezsp-host/ezsp-host-io.c",
                "simplicity_sdk/protocol/zigbee/app/ezsp-host/ezsp-host-queues.c",
                "simplicity_sdk/protocol/zigbee/app/ezsp-host/ezsp-host-ui.c",
                # SDK ASH-host sources
                "simplicity_sdk/protocol/zigbee/app/ezsp-host/ash/ash-host.c",
                "simplicity_sdk/protocol/zigbee/app/ezsp-host/ash/ash-host-ui.c",
                # ASH common implementation
                "simplicity_sdk/platform/service/legacy_common_ash/src/ash-common.c",
                # HAL implementations
                "simplicity_sdk/platform/service/legacy_hal/src/system-timer.c",
                "simplicity_sdk/platform/service/legacy_hal/src/crc.c",
            ],
            "include_dirs": [
                "<!@(node -p \"require('node-addon-api').include\")",
                # Stub headers
                "src/native/stubs",
                # SDK includes - add roots for relative includes
                "simplicity_sdk",
                "simplicity_sdk/protocol/zigbee",
                "simplicity_sdk/protocol/zigbee/app/util/ezsp",
                "simplicity_sdk/protocol/zigbee/app/ezsp-host",
                "simplicity_sdk/protocol/zigbee/app/ezsp-host/ash",
                "simplicity_sdk/protocol/zigbee/stack/include",
                "simplicity_sdk/protocol/zigbee/stack/config",
                "simplicity_sdk/protocol/zigbee/stack/platform/host",
                "simplicity_sdk/platform/service/legacy_common_ash/inc",
                "simplicity_sdk/platform/service/legacy_hal/inc",
                "simplicity_sdk/platform/common/inc",
                "simplicity_sdk/platform/radio/mac",
                "simplicity_sdk/util/silicon_labs/silabs_core",
            ],
            "defines": [
                "NAPI_VERSION=9",
                "NAPI_DISABLE_CPP_EXCEPTIONS",
                # SDK-specific defines
                "EZSP_HOST",
                "EZSP_ASH", # Use ASH protocol (not SPI or CPC)
                "EZSP_APPLICATION_HAS_ZIGBEE_KEY_ESTABLISHMENT_HANDLER",
                "EZSP_APPLICATION_HAS_ID_CONFLICT_HANDLER",
                "EZSP_APPLICATION_HAS_INCOMING_NETWORK_STATUS_HANDLER",
                "UNIX_HOST",
                # Define PLATFORM_HEADER to the actual header file
                "PLATFORM_HEADER=<platform-header.h>",
            ],
            "cflags": [
                "-std=c17",
                "-Wall",
                "-Wextra",
                "-Os",
                "-Wno-unused-parameter",
                "-Wno-missing-field-initializers",
                "-Wno-missing-braces",
            ],
            "cflags_cc": [
                "-std=c++17",
                "-Wall",
                "-Wextra",
                "-Os",
                "-Wno-unused-parameter",
                "-Wno-missing-field-initializers",
                "-Wno-missing-braces",
            ],
            "conditions": [
                [
                    "OS=='linux'",
                    {
                        "defines": ["_POSIX_C_SOURCE=200809L"],
                        "libraries": []
                    }
                ],
                [
                    "OS=='mac'",
                    {
                        "cflags+": ["-fvisibility=hidden"],
                        "xcode_settings": {
                            "GCC_SYMBOLS_PRIVATE_EXTERN": "YES", # -fvisibility=hidden
                        }
                    }
                ]
            ]
        }
    ]
}
