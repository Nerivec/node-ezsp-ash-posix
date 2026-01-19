import { beforeAll, describe, expect, it, vi } from "vitest";
import type { EzspNative } from "../src/index.js";

const TEST_ASH_CONFIG = {
    serialPort: "/dev/ttyMock",
    baudRate: 115200,
    stopBits: 1 as const,
    rtsCts: false,
    outBlockLen: 256,
    inBlockLen: 256,
    traceFlags: /*1 | 2 |*/ 4 | 8 | 16, // full debug
    txK: 3,
    randomize: true,
    ackTimeInit: 800,
    ackTimeMin: 400,
    ackTimeMax: 2400,
    timeRst: 5000,
    nrLowLimit: 8,
    nrHighLimit: 12,
    nrTime: 480,
    resetMethod: 0 as const,
};

describe("EZSP Native Binding", () => {
    let binding: EzspNative;

    beforeAll(async () => {
        binding = (await import("../src/index.js")).default;
    });

    it("loads the native module without error", () => {
        expect(binding).toBeDefined();
        expect(binding).not.toBeNull();
    });

    it("has all functions from types", () => {
        expect(typeof binding.init).toStrictEqual("function");
        expect(typeof binding.start).toStrictEqual("function");
        expect(typeof binding.stop).toStrictEqual("function");
        expect(typeof binding.ezspVersion).toStrictEqual("function");
        expect(typeof binding.ezspGetEui64).toStrictEqual("function");
        expect(typeof binding.ezspGetNetworkParameters).toStrictEqual("function");
        expect(typeof binding.ezspNetworkInit).toStrictEqual("function");
        expect(typeof binding.ezspNetworkState).toStrictEqual("function");
        expect(typeof binding.ezspFormNetwork).toStrictEqual("function");
        expect(typeof binding.ezspLeaveNetwork).toStrictEqual("function");
        expect(typeof binding.ezspPermitJoining).toStrictEqual("function");
        expect(typeof binding.ezspGetConfigurationValue).toStrictEqual("function");
        expect(typeof binding.ezspSetConfigurationValue).toStrictEqual("function");
        expect(typeof binding.ezspGetValue).toStrictEqual("function");
        expect(typeof binding.ezspSetValue).toStrictEqual("function");
        expect(typeof binding.ezspGetExtendedValue).toStrictEqual("function");
        expect(typeof binding.ezspSetPolicy).toStrictEqual("function");
        expect(typeof binding.ezspTokenFactoryReset).toStrictEqual("function");
        expect(typeof binding.ezspSetInitialSecurityState).toStrictEqual("function");
        expect(typeof binding.ezspGetNetworkKeyInfo).toStrictEqual("function");
        expect(typeof binding.ezspGetApsKeyInfo).toStrictEqual("function");
        expect(typeof binding.ezspExportKey).toStrictEqual("function");
        expect(typeof binding.ezspExportLinkKeyByIndex).toStrictEqual("function");
        expect(typeof binding.ezspImportLinkKey).toStrictEqual("function");
        expect(typeof binding.ezspImportTransientKey).toStrictEqual("function");
        expect(typeof binding.ezspEraseKeyTableEntry).toStrictEqual("function");
        expect(typeof binding.ezspClearKeyTable).toStrictEqual("function");
        expect(typeof binding.ezspClearTransientLinkKeys).toStrictEqual("function");
        expect(typeof binding.ezspBroadcastNextNetworkKey).toStrictEqual("function");
        expect(typeof binding.ezspBroadcastNetworkKeySwitch).toStrictEqual("function");
        expect(typeof binding.ezspSendUnicast).toStrictEqual("function");
        expect(typeof binding.ezspSendMulticast).toStrictEqual("function");
        expect(typeof binding.ezspSendBroadcast).toStrictEqual("function");
        expect(typeof binding.ezspSendRawMessage).toStrictEqual("function");
        expect(typeof binding.ezspSetRadioPower).toStrictEqual("function");
        expect(typeof binding.ezspSetRadioIeee802154CcaMode).toStrictEqual("function");
        expect(typeof binding.ezspSetLogicalAndRadioChannel).toStrictEqual("function");
        expect(typeof binding.ezspSetManufacturerCode).toStrictEqual("function");
        expect(typeof binding.ezspSetConcentrator).toStrictEqual("function");
        expect(typeof binding.ezspSetSourceRouteDiscoveryMode).toStrictEqual("function");
        expect(typeof binding.ezspSetMulticastTableEntry).toStrictEqual("function");
        expect(typeof binding.ezspAddEndpoint).toStrictEqual("function");
        expect(typeof binding.ezspReadAndClearCounters).toStrictEqual("function");
        expect(typeof binding.ezspSetNWKFrameCounter).toStrictEqual("function");
        expect(typeof binding.ezspSetAPSFrameCounter).toStrictEqual("function");
        expect(typeof binding.ezspStartWritingStackTokens).toStrictEqual("function");
        expect(typeof binding.ezspSetExtendedSecurityBitmask).toStrictEqual("function");
        expect(typeof binding.ezspGetEndpointFlags).toStrictEqual("function");
        expect(typeof binding.ezspGetVersionStruct).toStrictEqual("function");
        expect(typeof binding.send).toStrictEqual("function");
    });

    describe("init", () => {
        it("accepts a callback function in init", () => {
            const mockCallback = vi.fn();

            expect(() => {
                binding.init(TEST_ASH_CONFIG, mockCallback);
            }).not.toThrow();
        });

        it("works without a callback in init", () => {
            expect(() => {
                binding.init(TEST_ASH_CONFIG);
            }).not.toThrow();
        });

        it("rejects invalid config types", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.init(null as any);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.init(undefined as any);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.init("string" as any);
            }).toThrow();
        });

        it("validates required config properties", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.init({} as any);
            }).toThrow();

            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.init({ serialPort: "/dev/ttyUSB0" } as any);
            }).toThrow();

            expect(() => {
                binding.init({
                    ...TEST_ASH_CONFIG,
                    serialPort: "a".repeat(50), // Max is 39
                });
            }).toThrow();
        });

        it("throws if callback is not a function", () => {
            expect(() => {
                // biome-ignore lint/suspicious/noExplicitAny: test invalid input
                binding.init(TEST_ASH_CONFIG, "not a function" as any);
            }).toThrow();
        });
    });
});
