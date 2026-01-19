import { join } from "node:path";
import nodeGypBuild from "node-gyp-build";

type Eui64 = `0x${string}`;
type SLStatus = number;
type SLZigbeeVersion = {
    build: number;
    major: number;
    minor: number;
    patch: number;
    special: number;
    type: number;
};
type SLZigbeeKeyData = { contents: Buffer };

type SLZigbeeNetworkParameters = {
    extendedPanId: number[]; // 16-size
    panId: number;
    radioTxPower: number;
    radioChannel: number;
    joinMethod: number;
    nwkManagerId: number;
    nwkUpdateId: number;
    channels: number;
};

type SLZigbeeInitialSecurityState = {
    bitmask: number;
    preconfiguredKey: SLZigbeeKeyData;
    networkKey: SLZigbeeKeyData;
    networkKeySequenceNumber: number;
    preconfiguredTrustCenterEui64: Eui64;
};

type SLZigbeeSecManNetworkKeyInfo = {
    networkKeySet: boolean;
    alternateNetworkKeySet: boolean;
    networkKeySequenceNumber: number;
    altNetworkKeySequenceNumber: number;
    networkKeyFrameCounter: number;
};

type SLZigbeeSecManContext = {
    coreKeyType: number;
    keyIndex: number;
    derivedType: number;
    eui64: Eui64;
    multiNetworkIndex: number;
    flags: number;
    psaKeyAlgPermission: number;
};

type SLZigbeeSecManApsKeyMetadata = {
    bitmask: number;
    outgoingFrameCounter: number;
    incomingFrameCounter: number;
    ttlInSeconds: number;
};

type SLZigbeeApsFrame = {
    profileId: number;
    clusterId: number;
    sourceEndpoint: number;
    destinationEndpoint: number;
    options: number;
    groupId: number;
    sequence: number;
    radius?: number;
};

type SLZigbeeMulticastTableEntry = {
    multicastId: number;
    endpoint: number;
    networkIndex: number;
};

/**
 * Typing for EZSP events emitted from native callbacks.
 */
export type EzspNativeEvent =
    | {
          name: "ncpNeedsResetAndInit";
          /** sl_zigbee_ezsp_status_t */
          status: number;
      }
    | {
          name: "stackStatus";
          status: SLStatus;
      }
    | {
          name: "messageSent";
          status: SLStatus;
          type: number;
          indexOrDestination: number;
          apsFrame: SLZigbeeApsFrame;
          messageTag: number;
          messageContents: Buffer;
      }
    | {
          name: "zdoResponse";
          apsFrame: SLZigbeeApsFrame;
          sender: number;
          messageContents: Buffer;
      }
    | {
          name: "incomingMessage";
          type: number;
          apsFrame: SLZigbeeApsFrame;
          lastHopLqi: number;
          sender: number;
          messageContents: Buffer;
      }
    | {
          name: "touchlinkMessage";
          sourcePanId: number;
          sourceAddress: Eui64;
          groupId: number;
          lastHopLqi: number;
          messageContents: Buffer;
      }
    | {
          name: "trustCenterJoin";
          newNodeId: number;
          newNodeEui64: Eui64;
          status: number;
          policyDecision: number;
          parentOfNewNodeId: number;
      };

export type EzspEventCallback = (event: EzspNativeEvent) => void;

export interface EzspNative {
    init(
        ashHostConfig: {
            /** serial port name | char[40] */
            serialPort: string;
            /** baud rate (bits/second) | uint32_t | 57600,115200,230400,460800,921600 */
            baudRate: number;
            /** uint8_t */
            stopBits: 1;
            /** true enables RTS/CTS flow control, false XON/XOFF */
            rtsCts: boolean;
            /** max bytes to buffer before writing to serial port | uint16_t */
            outBlockLen: number;
            /** max bytes to read ahead from serial port | uint16_t */
            inBlockLen: number;
            /**
             * trace output control bit flags
             * - 1: frames sent and received (TRACE_FRAMES_BASIC)
             * - 2: basic frames + internal variables (TRACE_FRAMES_VERBOSE)
             * - 4: events (TRACE_EVENTS)
             * - 8: EZSP commands, responses and callbacks (TRACE_EZSP)
             * - 16: additional EZSP information (TRACE_EZSP_VERBOSE)
             */
            traceFlags: number;
            /** max frames sent without being ACKed (1-7) | uint8_t */
            txK: number;
            /** enables randomizing DATA frame payloads */
            randomize: boolean;
            /** adaptive rec'd ACK timeout initial value | uint16_t */
            ackTimeInit: number;
            /** adaptive rec'd ACK timeout minimum value | uint16_t */
            ackTimeMin: number;
            /** adaptive rec'd ACK timeout maximum value | uint16_t */
            ackTimeMax: number;
            /** time allowed to receive RSTACK after ncp is reset | uint16_t */
            timeRst: number;
            /** if free buffers < limit, host receiver isn't ready | uint8_t */
            nrLowLimit: number;
            /** if free buffers > limit, host receiver is ready | uint8_t */
            nrHighLimit: number;
            /** time until a set nFlag must be resent (max 2032) | uint16_t */
            nrTime: number;
            /**
             * method used to reset ncp
             * - 0: send RST frame (ASH_RESET_METHOD_RST)
             * - 1: reset using DTR (ASH_RESET_METHOD_DTR)
             * - 2: hook for user-defined reset (ASH_RESET_METHOD_CUSTOM)
             * - 3: no reset - for testing (ASH_RESET_METHOD_NONE)
             */
            resetMethod: 0 | 1 | 2 | 3;
        },
        callback?: EzspEventCallback,
    ): undefined;
    start(): number;
    stop(): undefined;

    // Base
    ezspVersion(desiredProtocolVersion: number): [protocolVersion: number, stackType: number, stackVersion: number];
    ezspGetEui64(): Eui64;

    // Network management
    ezspGetNetworkParameters(): [status: SLStatus, nodeType: number, parameters: SLZigbeeNetworkParameters];
    ezspNetworkInit(networkInitStruct: { bitmask: number }): SLStatus;
    ezspNetworkState(): SLStatus;
    ezspFormNetwork(parameters: SLZigbeeNetworkParameters): SLStatus;
    ezspLeaveNetwork(options?: number): SLStatus;
    ezspPermitJoining(duration: number): SLStatus;

    // Configuration
    ezspGetConfigurationValue(configId: number): [status: SLStatus, value: number];
    ezspSetConfigurationValue(configId: number, value: number): SLStatus;
    // `valueLength` not needed, but kept in typing to match Node.js implementation
    ezspGetValue(valueId: number, valueLength?: number): [status: SLStatus, outValueLength: number, outValue: number[]];
    ezspSetValue(valueId: number, valueLength: number, value: number[]): SLStatus;
    // `valueLength` not needed, but kept in typing to match Node.js implementation
    ezspGetExtendedValue(
        valueId: number,
        characteristics: number,
        valueLength?: number,
    ): [status: SLStatus, outValueLength: number, outValue: number[]];
    ezspSetPolicy(policyId: number, decisionId: number): SLStatus;
    ezspTokenFactoryReset(excludeOutgoingFC: boolean, excludeBootCounter: boolean): undefined;

    // Security
    ezspSetInitialSecurityState(state: SLZigbeeInitialSecurityState): SLStatus;
    ezspGetNetworkKeyInfo(): [status: SLStatus, networkKeyInfo: SLZigbeeSecManNetworkKeyInfo];
    ezspGetApsKeyInfo(context: SLZigbeeSecManContext): [status: SLStatus, keyData: SLZigbeeSecManApsKeyMetadata];
    ezspExportKey(context: SLZigbeeSecManContext): [status: SLStatus, key: SLZigbeeKeyData];
    ezspExportLinkKeyByIndex(
        index: number,
    ): [status: SLStatus, context: SLZigbeeSecManContext, plaintextKey: SLZigbeeKeyData, keyData: SLZigbeeSecManApsKeyMetadata];
    ezspImportLinkKey(index: number, address: Eui64, plaintextKey: SLZigbeeKeyData): SLStatus;
    ezspImportTransientKey(eui64: Eui64, plaintextKey: SLZigbeeKeyData): SLStatus;
    ezspEraseKeyTableEntry(index: number): SLStatus;
    ezspClearKeyTable(): SLStatus;
    ezspClearTransientLinkKeys(): undefined;
    ezspBroadcastNextNetworkKey(key: SLZigbeeKeyData): SLStatus;
    ezspBroadcastNetworkKeySwitch(): SLStatus;

    // Messaging
    ezspSendUnicast(
        type: number,
        indexOrDestination: number,
        apsFrame: SLZigbeeApsFrame,
        messageTag: number,
        messageContents: Buffer,
    ): [status: SLStatus, apsSequence: number];
    ezspSendMulticast(
        apsFrame: SLZigbeeApsFrame,
        hops: number,
        broadcastAddr: number,
        alias: number,
        nwkSequence: number,
        messageTag: number,
        messageContents: Buffer,
    ): undefined;
    ezspSendBroadcast(
        alias: number,
        destination: number,
        nwkSequence: number,
        apsFrame: SLZigbeeApsFrame,
        radius: number,
        messageTag: number,
        messageContents: Buffer,
    ): [status: SLStatus, apsSequence: number];
    ezspSendRawMessage(messageContents: Buffer, priority: number, useCca: boolean): SLStatus;

    // Radio/hardware
    ezspSetRadioPower(power: number): SLStatus;
    ezspSetRadioIeee802154CcaMode(ccaMode: number): SLStatus;
    ezspSetLogicalAndRadioChannel(radioChannel: number): SLStatus;
    ezspSetManufacturerCode(code: number): SLStatus;

    // Routing/tables
    ezspSetConcentrator(
        on: boolean,
        concentratorType: number,
        minTime: number,
        maxTime: number,
        routeErrorThreshold: number,
        deliveryFailureThreshold: number,
        maxHops: number,
    ): SLStatus;
    ezspSetSourceRouteDiscoveryMode(mode: number): number;
    ezspSetMulticastTableEntry(index: number, value: SLZigbeeMulticastTableEntry): SLStatus;
    ezspAddEndpoint(
        endpoint: number,
        profileId: number,
        deviceId: number,
        deviceVersion: number,
        inputClusterList: number[],
        outputClusterList: number[],
    ): SLStatus;

    // Monitoring
    ezspReadAndClearCounters(): number[];

    // Convenience wrappers
    ezspSetNWKFrameCounter(frameCounter: number): SLStatus;
    ezspSetAPSFrameCounter(frameCounter: number): SLStatus;
    ezspStartWritingStackTokens(): SLStatus;
    ezspSetExtendedSecurityBitmask(mask: number): SLStatus;
    ezspGetEndpointFlags(endpoint: number): [status: SLStatus, flags: number];
    ezspGetVersionStruct(): [status: SLStatus, version: SLZigbeeVersion];
    /** `apsFrame.sequence` is mutated internally based on call */
    send(
        type: number,
        indexOrDestination: number,
        apsFrame: SLZigbeeApsFrame,
        message: Buffer,
        alias: number,
        sequence: number,
    ): [status: SLStatus, messageTag: number];
}

/**
 * Native binding prebuild loaded via node-gyp-build.
 *
 * NOTE: package.json explicitly does not have `node-gyp-build` in `install` step due to SDK requirements.
 *       If a prebuild is not found for a given system, this will hard fail.
 */
const nativeModule = nodeGypBuild(join(import.meta.dirname, "../"));

export default nativeModule as EzspNative;
