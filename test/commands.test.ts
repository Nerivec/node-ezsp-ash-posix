import { beforeAll, describe, expect, it } from "vitest";
import type { EzspNative } from "../src/index.js";

const TEST_EUI64 = "0x0123456789abcdef";
const TEST_EUI64_INVALID = "0xinvalid";
const TEST_EUI64_SHORT = "0x0123";

const TEST_APS_FRAME = {
    profileId: 0x0104,
    clusterId: 0x0006,
    sourceEndpoint: 1,
    destinationEndpoint: 1,
    options: 0x0040,
    groupId: 0,
    sequence: 0,
};

const TEST_NETWORK_PARAMS = {
    extendedPanId: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15],
    panId: 0x1234,
    radioTxPower: 8,
    radioChannel: 15,
    joinMethod: 0,
    nwkManagerId: 0,
    nwkUpdateId: 0,
    channels: 0x07fff800,
};

// const TEST_SECURITY_STATE = {
//     bitmask: 0,
//     preconfiguredKey: { contents: Buffer.alloc(16, 0) },
//     networkKey: { contents: Buffer.alloc(16, 0) },
//     networkKeySequenceNumber: 0,
//     preconfiguredTrustCenterEui64: TEST_EUI64,
// };

const TEST_SEC_MAN_CONTEXT = {
    coreKeyType: 0,
    keyIndex: 0,
    derivedType: 0,
    eui64: TEST_EUI64,
    multiNetworkIndex: 0,
    flags: 0,
    psaKeyAlgPermission: 0,
};

describe("EZSP Commands", () => {
    let binding: EzspNative;

    beforeAll(async () => {
        binding = (await import("../src/index.js")).default;
    });

    // TODO: mock hardware
    // These tests focus on argument validation and type conversion.

    describe("ezspVersion", () => {
        it("rejects non-number arguments", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspVersion("13" as any);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspVersion(null as any);
            }).toThrow();
        });

        it("rejects missing arguments", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspVersion(undefined as any);
            }).toThrow();
        });
    });

    describe("ezspNetworkInit", () => {
        it("rejects non-object arguments", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspNetworkInit(null as any);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspNetworkInit("object" as any);
            }).toThrow();
        });

        it("rejects missing bitmask property", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspNetworkInit({} as any);
            }).toThrow();
        });
    });

    describe("ezspFormNetwork", () => {
        it("rejects invalid parameters object", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspFormNetwork(null as any);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspFormNetwork({} as any);
            }).toThrow();
        });

        it("rejects invalid extendedPanId", () => {
            expect(() => {
                binding.ezspFormNetwork({
                    ...TEST_NETWORK_PARAMS,
                    extendedPanId: [1, 2, 3], // Wrong length
                });
            }).toThrow();

            expect(() => {
                binding.ezspFormNetwork({
                    ...TEST_NETWORK_PARAMS,
                    // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                    extendedPanId: "not an array" as any,
                });
            }).toThrow();
        });
    });

    describe("ezspPermitJoining", () => {
        it("rejects non-number duration", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspPermitJoining("60" as any);
            }).toThrow();
        });
    });

    describe("ezspGetConfigurationValue / ezspSetConfigurationValue", () => {
        it("rejects non-number configId", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspGetConfigurationValue("1" as any);
            }).toThrow();
        });

        it("rejects invalid setValue arguments", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSetConfigurationValue("1" as any, 10);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSetConfigurationValue(1, "10" as any);
            }).toThrow();
        });
    });

    describe("ezspGetValue / ezspSetValue", () => {
        it("rejects invalid valueId", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspGetValue("1" as any);
            }).toThrow();
        });

        it("rejects invalid value array in setValue", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSetValue(1, 4, "not array" as any);
            }).toThrow();

            expect(() => {
                binding.ezspSetValue(1, 2, [1, 2, 3, 4]); // Length mismatch
            }).toThrow();
        });
    });

    describe("ezspGetExtendedValue", () => {
        it("rejects invalid arguments", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspGetExtendedValue("1" as any, 0);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspGetExtendedValue(1, "0" as any);
            }).toThrow();
        });
    });

    describe("ezspSetPolicy", () => {
        it("rejects non-number arguments", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSetPolicy("1" as any, 0);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSetPolicy(1, "0" as any);
            }).toThrow();
        });
    });

    describe("ezspTokenFactoryReset", () => {
        it("rejects non-boolean arguments", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspTokenFactoryReset(1 as any, false);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspTokenFactoryReset(false, "false" as any);
            }).toThrow();
        });
    });

    describe("ezspSetInitialSecurityState", () => {
        it("rejects invalid security state object", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSetInitialSecurityState(null as any);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSetInitialSecurityState({} as any);
            }).toThrow();
        });

        it("rejects invalid key data", () => {
            expect(() => {
                binding.ezspSetInitialSecurityState({
                    bitmask: 0,
                    preconfiguredKey: { contents: Buffer.alloc(16, 0) },
                    // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                    networkKey: { contents: "not a buffer" as any },
                    networkKeySequenceNumber: 0,
                    preconfiguredTrustCenterEui64: TEST_EUI64,
                });
            }).toThrow();
        });

        it("rejects invalid key data length", () => {
            expect(() => {
                binding.ezspSetInitialSecurityState({
                    bitmask: 0,
                    preconfiguredKey: { contents: Buffer.alloc(16, 0) },
                    networkKey: { contents: Buffer.alloc(8) }, // Wrong size
                    networkKeySequenceNumber: 0,
                    preconfiguredTrustCenterEui64: TEST_EUI64,
                });
            }).toThrow();
        });

        it("rejects invalid EUI64", () => {
            expect(() => {
                binding.ezspSetInitialSecurityState({
                    bitmask: 0,
                    preconfiguredKey: { contents: Buffer.alloc(16, 0) },
                    networkKey: { contents: Buffer.alloc(16, 0) },
                    networkKeySequenceNumber: 0,
                    // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                    preconfiguredTrustCenterEui64: TEST_EUI64_INVALID as any,
                });
            }).toThrow();
        });
    });

    describe("ezspGetApsKeyInfo / ezspExportKey", () => {
        it("rejects invalid context object", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspGetApsKeyInfo(null as any);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspExportKey({} as any);
            }).toThrow();
        });

        it("rejects invalid EUI64 in context", () => {
            expect(() => {
                binding.ezspGetApsKeyInfo({
                    ...TEST_SEC_MAN_CONTEXT,
                    eui64: TEST_EUI64_SHORT,
                });
            }).toThrow();
        });
    });

    describe("ezspExportLinkKeyByIndex", () => {
        it("rejects non-number index", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspExportLinkKeyByIndex("0" as any);
            }).toThrow();
        });
    });

    describe("ezspImportLinkKey / ezspImportTransientKey", () => {
        it("rejects invalid arguments", () => {
            const validKey = { contents: Buffer.alloc(16, 0xff) };

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspImportLinkKey("not a number" as any, TEST_EUI64, validKey);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspImportLinkKey(0, TEST_EUI64_INVALID as any, validKey);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspImportLinkKey(0, TEST_EUI64, null as any);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspImportTransientKey(123 as any, validKey);
            }).toThrow();
        });
    });

    describe("ezspEraseKeyTableEntry / ezspClearKeyTable", () => {
        it("rejects non-number index", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspEraseKeyTableEntry("0" as any);
            }).toThrow();
        });
    });

    describe("ezspBroadcastNextNetworkKey", () => {
        it("rejects invalid key", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspBroadcastNextNetworkKey(null as any);
            }).toThrow();

            expect(() => {
                binding.ezspBroadcastNextNetworkKey({
                    contents: Buffer.alloc(8), // Wrong size
                });
            }).toThrow();
        });
    });

    describe("ezspSendUnicast", () => {
        it("rejects invalid arguments", () => {
            const validMessage = Buffer.from([0x00, 0x01, 0x02]);

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSendUnicast("0" as any, 0x1234, TEST_APS_FRAME, 1, validMessage);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSendUnicast(0, "0x1234" as any, TEST_APS_FRAME, 1, validMessage);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSendUnicast(0, 0x1234, null as any, 1, validMessage);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSendUnicast(0, 0x1234, TEST_APS_FRAME, "1" as any, validMessage);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSendUnicast(0, 0x1234, TEST_APS_FRAME, 1, "not a buffer" as any);
            }).toThrow();
        });

        it("rejects invalid APS frame", () => {
            const validMessage = Buffer.from([0x00, 0x01, 0x02]);

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSendUnicast(0, 0x1234, {} as any, 1, validMessage);
            }).toThrow();
        });
    });

    describe("ezspSendMulticast", () => {
        it("rejects invalid arguments", () => {
            const validMessage = Buffer.from([0x00, 0x01, 0x02]);

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSendMulticast(null as any, 3, 0xfffc, 0, 0, 1, validMessage);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSendMulticast(TEST_APS_FRAME, "3" as any, 0xfffc, 0, 0, 1, validMessage);
            }).toThrow();
        });
    });

    describe("ezspSendBroadcast", () => {
        it("rejects invalid arguments", () => {
            const validMessage = Buffer.from([0x00, 0x01, 0x02]);

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSendBroadcast(0, 0xfffc, "0" as any, TEST_APS_FRAME, 3, 1, validMessage);
            }).toThrow();
        });
    });

    describe("ezspSendRawMessage", () => {
        it("rejects invalid message", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSendRawMessage("not a buffer" as any, 0, true);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSendRawMessage(Buffer.from([0x00]), "0" as any, true);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSendRawMessage(Buffer.from([0x00]), 0, "true" as any);
            }).toThrow();
        });
    });

    describe("ezspSetRadioPower", () => {
        it("rejects non-number power", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSetRadioPower("8" as any);
            }).toThrow();
        });
    });

    describe("ezspSetRadioIeee802154CcaMode", () => {
        it("rejects non-number ccaMode", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSetRadioIeee802154CcaMode("1" as any);
            }).toThrow();
        });
    });

    describe("ezspSetLogicalAndRadioChannel", () => {
        it("rejects non-number channel", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSetLogicalAndRadioChannel("15" as any);
            }).toThrow();
        });
    });

    describe("ezspSetManufacturerCode", () => {
        it("rejects non-number code", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSetManufacturerCode("0x1234" as any);
            }).toThrow();
        });
    });

    describe("ezspSetConcentrator", () => {
        it("rejects invalid arguments", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSetConcentrator("true" as any, 1, 10, 60, 3, 1, 0);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSetConcentrator(true, "1" as any, 10, 60, 3, 1, 0);
            }).toThrow();
        });
    });

    describe("ezspSetSourceRouteDiscoveryMode", () => {
        it("rejects non-number mode", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSetSourceRouteDiscoveryMode("1" as any);
            }).toThrow();
        });
    });

    describe("ezspSetMulticastTableEntry", () => {
        it("rejects invalid arguments", () => {
            const validEntry = {
                multicastId: 1,
                endpoint: 1,
                networkIndex: 0,
            };

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSetMulticastTableEntry("0" as any, validEntry);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSetMulticastTableEntry(0, null as any);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSetMulticastTableEntry(0, {} as any);
            }).toThrow();
        });
    });

    describe("ezspAddEndpoint", () => {
        it("rejects invalid arguments", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspAddEndpoint("1" as any, 0x0104, 0x0001, 1, [6], [6]);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspAddEndpoint(1, "0x0104" as any, 0x0001, 1, [6], [6]);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspAddEndpoint(1, 0x0104, 0x0001, 1, "not array" as any, [6]);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspAddEndpoint(1, 0x0104, 0x0001, 1, [6], "not array" as any);
            }).toThrow();
        });
    });

    describe("ezspSetNWKFrameCounter / ezspSetAPSFrameCounter", () => {
        it("rejects invalid frame counter", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSetNWKFrameCounter("100" as any);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSetAPSFrameCounter("100" as any);
            }).toThrow();
        });
    });

    describe("ezspSetExtendedSecurityBitmask", () => {
        it("rejects invalid bitmask", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspSetExtendedSecurityBitmask("0x0001" as any);
            }).toThrow();
        });
    });
    describe("ezspGetEndpointFlags", () => {
        it("rejects invalid endpoint", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.ezspGetEndpointFlags("1" as any);
            }).toThrow();
        });
    });
    describe("send", () => {
        it("rejects invalid arguments", () => {
            const validMessage = Buffer.from([0x00, 0x01, 0x02]);

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.send("0" as any, 0x1234, TEST_APS_FRAME, validMessage, 0, 0);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.send(0, "0x1234" as any, TEST_APS_FRAME, validMessage, 0, 0);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.send(0, 0x1234, null as any, validMessage, 0, 0);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.send(0, 0x1234, TEST_APS_FRAME, "not a buffer" as any, 0, 0);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.send(0, 0x1234, TEST_APS_FRAME, validMessage, "0" as any, 0);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.send(0, 0x1234, TEST_APS_FRAME, validMessage, 0, "0" as any);
            }).toThrow();
        });
    });
});
