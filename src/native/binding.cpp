/**
 * Node-API bindings for Silicon Labs EmberZNet Serial Protocol (EZSP)
 * and Asynchronous Serial Host (ASH) protocols of Simplicity SDK
 *
 * https://github.com/SiliconLabs/simplicity_sdk
 *
 * https://github.com/nodejs/node-addon-api/blob/main/doc
 */

#include <napi.h>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <memory>
#include <uv.h>

// Silicon Labs SDK headers
extern "C"
{
#include "platform-header.h" // base types
#include "sl_status.h"       // comes first - defines sl_status_t
#include "sl_zigbee_types.h"
#include "ezsp-enum.h"
#include "ezsp-host-common.h"
#include "ezsp-host-io.h"
#include "ash-host.h"
#include "ezsp.h"
#include "serial-interface.h"
#include "ezsp-protocol.h"
#include "ezsp-host-priv.h"
}

#define simulatedTimePasses()

// #region InterPAN helpers

// NWK stub frame has two control bytes.
#define STUB_NWK_SIZE 2
#define STUB_NWK_FRAME_CONTROL 0x000B

// Interpan APS Unicast
//  - Frame Control   (1-byte)
//  - Cluster ID      (2-bytes)
//  - Profile ID      (2-bytes)
#define INTERPAN_APS_UNICAST_SIZE 5

// Interpan APS Multicast
//  - Frame Control   (1-byte)
//  - Group ID        (2-bytes)
//  - Cluster ID      (2-bytes)
//  - Profile ID      (2-bytes)
#define INTERPAN_APS_MULTICAST_SIZE 7

#define MIN_STUB_APS_SIZE (INTERPAN_APS_UNICAST_SIZE)

#define INTERPAN_APS_FRAME_TYPE 0x03

// The only allowed APS FC value (without the delivery mode subfield)
#define INTERPAN_APS_FRAME_CONTROL_NO_DELIVERY_MODE (INTERPAN_APS_FRAME_TYPE)

#define INTERPAN_APS_FRAME_DELIVERY_MODE_MASK 0x0C
#define INTERPAN_APS_FRAME_SECURITY 0x20

#define SL_ZIGBEE_AF_INTER_PAN_UNICAST 0x00u
#define SL_ZIGBEE_AF_INTER_PAN_BROADCAST 0x08u
#define SL_ZIGBEE_AF_INTER_PAN_MULTICAST 0x0Cu

#define MAC_ACK_REQUIRED 0x0020

#define MAC_FRAME_TYPE_DATA 0x0001

#define MAC_FRAME_SOURCE_MODE_SHORT 0x8000
#define MAC_FRAME_SOURCE_MODE_LONG 0xC000

#define MAC_FRAME_DESTINATION_MODE_SHORT 0x0800
#define MAC_FRAME_DESTINATION_MODE_LONG 0x0C00

// The two possible incoming MAC frame controls.
// Using short source address is not allowed.
#define SHORT_DEST_FRAME_CONTROL (MAC_FRAME_TYPE_DATA | MAC_FRAME_DESTINATION_MODE_SHORT | MAC_FRAME_SOURCE_MODE_LONG)
#define LONG_DEST_FRAME_CONTROL (MAC_FRAME_TYPE_DATA | MAC_FRAME_DESTINATION_MODE_LONG | MAC_FRAME_SOURCE_MODE_LONG)

// #endregion InterPAN helpers

// Forward declarations
namespace EzspNapi
{
    Napi::Value Init(const Napi::CallbackInfo &info);
    Napi::Value Start(const Napi::CallbackInfo &info);
    Napi::Value Stop(const Napi::CallbackInfo &info);

    // Base commands
    Napi::Value Version(const Napi::CallbackInfo &info);
    Napi::Value GetEui64(const Napi::CallbackInfo &info);

    // Network management commands
    Napi::Value GetNetworkParameters(const Napi::CallbackInfo &info);
    Napi::Value NetworkInit(const Napi::CallbackInfo &info);
    Napi::Value NetworkState(const Napi::CallbackInfo &info);
    Napi::Value FormNetwork(const Napi::CallbackInfo &info);
    Napi::Value LeaveNetwork(const Napi::CallbackInfo &info);
    Napi::Value PermitJoining(const Napi::CallbackInfo &info);

    // Configuration commands
    Napi::Value GetConfigurationValue(const Napi::CallbackInfo &info);
    Napi::Value SetConfigurationValue(const Napi::CallbackInfo &info);
    Napi::Value GetValue(const Napi::CallbackInfo &info);
    Napi::Value SetValue(const Napi::CallbackInfo &info);
    Napi::Value GetExtendedValue(const Napi::CallbackInfo &info);
    Napi::Value SetPolicy(const Napi::CallbackInfo &info);
    Napi::Value TokenFactoryReset(const Napi::CallbackInfo &info);

    // Security commands
    Napi::Value SetInitialSecurityState(const Napi::CallbackInfo &info);
    Napi::Value GetNetworkKeyInfo(const Napi::CallbackInfo &info);
    Napi::Value GetApsKeyInfo(const Napi::CallbackInfo &info);
    Napi::Value ExportKey(const Napi::CallbackInfo &info);
    Napi::Value ExportLinkKeyByIndex(const Napi::CallbackInfo &info);
    Napi::Value ImportLinkKey(const Napi::CallbackInfo &info);
    Napi::Value ImportTransientKey(const Napi::CallbackInfo &info);
    Napi::Value EraseKeyTableEntry(const Napi::CallbackInfo &info);
    Napi::Value ClearKeyTable(const Napi::CallbackInfo &info);
    Napi::Value ClearTransientLinkKeys(const Napi::CallbackInfo &info);
    Napi::Value BroadcastNextNetworkKey(const Napi::CallbackInfo &info);
    Napi::Value BroadcastNetworkKeySwitch(const Napi::CallbackInfo &info);

    // Messaging commands
    Napi::Value SendUnicast(const Napi::CallbackInfo &info);
    Napi::Value SendMulticast(const Napi::CallbackInfo &info);
    Napi::Value SendBroadcast(const Napi::CallbackInfo &info);
    Napi::Value SendRawMessage(const Napi::CallbackInfo &info);

    // Radio/hardware commands
    Napi::Value SetRadioPower(const Napi::CallbackInfo &info);
    Napi::Value SetRadioIeee802154CcaMode(const Napi::CallbackInfo &info);
    Napi::Value SetLogicalAndRadioChannel(const Napi::CallbackInfo &info);
    Napi::Value SetManufacturerCode(const Napi::CallbackInfo &info);

    // Routing/table commands
    Napi::Value SetConcentrator(const Napi::CallbackInfo &info);
    Napi::Value SetSourceRouteDiscoveryMode(const Napi::CallbackInfo &info);
    Napi::Value SetMulticastTableEntry(const Napi::CallbackInfo &info);
    Napi::Value AddEndpoint(const Napi::CallbackInfo &info);

    // Monitoring
    Napi::Value ReadAndClearCounters(const Napi::CallbackInfo &info);

    // Convenience wrappers
    Napi::Value SetNWKFrameCounter(const Napi::CallbackInfo &info);
    Napi::Value SetAPSFrameCounter(const Napi::CallbackInfo &info);
    Napi::Value StartWritingStackTokens(const Napi::CallbackInfo &info);
    Napi::Value SetExtendedSecurityBitmask(const Napi::CallbackInfo &info);
    Napi::Value GetEndpointFlags(const Napi::CallbackInfo &info);
    Napi::Value GetVersionStruct(const Napi::CallbackInfo &info);
}

// Global reference to callback function
static Napi::ThreadSafeFunction tsfn;
static bool initialized = false;
// Poll handle for ezsp tick
static uv_timer_t tickTimer;
static bool tickTimerActive = false;

static uint8_t ezspSequenceNumber = 0;

// Tick callback that checks for EZSP events
static void ezspTickCallback(uv_timer_t *handle)
{
    // Call EZSP tick function to process any pending callbacks/events
    extern void sl_zigbee_ezsp_tick(void);
    sl_zigbee_ezsp_tick();
}

static uint8_t ezspNextSequence(void) { return ((++ezspSequenceNumber) & 0x7F); }

// #region Helper Functions for Type Conversions

/**
 * Convert uint8_t array to JavaScript array of numbers
 * @param env Napi environment
 * @param data Pointer to uint8_t array
 * @param length Array length
 * @return JavaScript array
 */
inline Napi::Array Uint8ArrayToNumberArray(Napi::Env env, const uint8_t *data, size_t length)
{
    Napi::Array arr = Napi::Array::New(env, length);

    for (size_t i = 0; i < length; i++)
    {
        arr[i] = Napi::Number::New(env, data[i]);
    }

    return arr;
}

/**
 * Convert JavaScript array or Buffer to uint8_t array
 * @param env Napi environment
 * @param jsValue JavaScript array or buffer
 * @param output Output buffer pointer
 * @param maxLength Maximum output buffer size
 * @return true if copied, false on error
 */
inline bool Uint8ArrayFromNumberArray(Napi::Env env, const Napi::Value &jsValue, uint8_t *output, size_t maxLength)
{
    if (jsValue.IsArray())
    {
        Napi::Array arr = jsValue.As<Napi::Array>();

        if (arr.Length() != maxLength)
        {
            return false; // Error: mismatching array length and given length
        }

        for (size_t i = 0; i < maxLength; i++)
        {
            Napi::Value val = arr[i];

            if (val.IsNumber())
            {
                output[i] = val.As<Napi::Number>().Uint32Value();
            }
            else
            {
                return false; // Error: non-number in array
            }
        }

        return true;
    }

    return false; // Error: not buffer or array
}

/**
 * Convert EUI64 from 8-byte array to hex string format (0xXXXXXXXXXXXXXXXX)
 * @param eui64 Input buffer (8 bytes)
 * @param hexString Output buffer (must be at least 19 bytes: "0x" + 16 hex chars + null terminator)
 */
inline void Eui64ToHexString(const uint8_t *eui64, char *hexString)
{
    snprintf(hexString, 19, "0x%02x%02x%02x%02x%02x%02x%02x%02x", eui64[7], eui64[6], eui64[5], eui64[4], eui64[3], eui64[2], eui64[1], eui64[0]);
}

/**
 * Convert EUI64 from hex string (0xXXXXXXXXXXXXXXXX) to 8-byte array
 * @param env Napi environment
 * @param hexString JavaScript string in format "0xXXXXXXXXXXXXXXXX"
 * @param eui64 Output buffer (8 bytes)
 * @return true on success, false on error
 */
inline bool Eui64FromHexString(Napi::Env env, const Napi::String &hexString, uint8_t *eui64)
{
    std::string str = hexString.Utf8Value();

    // Expected format: "0x" + 16 hex chars
    if (str.length() != 18 || str[0] != '0' || str[1] != 'x')
    {
        return false;
    }

    // Parse in reverse order (little-endian)
    for (int i = 0; i < 8; i++)
    {
        const char *hexByte = str.c_str() + 2 + (14 - i * 2); // Start from end
        char temp[3] = {hexByte[0], hexByte[1], '\0'};
        eui64[i] = (uint8_t)strtol(temp, nullptr, 16);
    }

    return true;
}

/**
 * Convert sl_zigbee_aps_frame_t from JavaScript object to native struct
 * @param env Napi environment
 * @param apsFrameObj JavaScript object `SLZigbeeApsFrame`
 * @param apsFrame Output native struct pointer
 * @return true on success, false on invalid object
 */
inline bool ApsFrameFromObject(Napi::Env env, const Napi::Object &apsFrameObj, sl_zigbee_aps_frame_t *apsFrame)
{
    if (!apsFrameObj.Has("profileId") || !apsFrameObj.Has("clusterId") || !apsFrameObj.Has("sourceEndpoint") ||
        !apsFrameObj.Has("destinationEndpoint") || !apsFrameObj.Has("options") || !apsFrameObj.Has("groupId") || !apsFrameObj.Has("sequence"))
    {
        return false;
    }

    memset(apsFrame, 0, sizeof(sl_zigbee_aps_frame_t));
    apsFrame->profileId = apsFrameObj.Get("profileId").As<Napi::Number>().Uint32Value();
    apsFrame->clusterId = apsFrameObj.Get("clusterId").As<Napi::Number>().Uint32Value();
    apsFrame->sourceEndpoint = apsFrameObj.Get("sourceEndpoint").As<Napi::Number>().Uint32Value();
    apsFrame->destinationEndpoint = apsFrameObj.Get("destinationEndpoint").As<Napi::Number>().Uint32Value();
    apsFrame->options = apsFrameObj.Get("options").As<Napi::Number>().Uint32Value();
    apsFrame->groupId = apsFrameObj.Get("groupId").As<Napi::Number>().Uint32Value();
    apsFrame->sequence = apsFrameObj.Get("sequence").As<Napi::Number>().Uint32Value();
    apsFrame->radius = apsFrameObj.Has("radius") ? apsFrameObj.Get("radius").As<Napi::Number>().Uint32Value() : 0;

    return true;
}

/**
 * Convert sl_zigbee_aps_frame_t from native struct to JavaScript object
 * @param env Napi environment
 * @param apsFrame Native struct pointer
 * @return JavaScript object `SLZigbeeApsFrame`
 */
inline Napi::Object ApsFrameToObject(Napi::Env env, const sl_zigbee_aps_frame_t *apsFrame)
{
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("profileId", Napi::Number::New(env, apsFrame->profileId));
    obj.Set("clusterId", Napi::Number::New(env, apsFrame->clusterId));
    obj.Set("sourceEndpoint", Napi::Number::New(env, apsFrame->sourceEndpoint));
    obj.Set("destinationEndpoint", Napi::Number::New(env, apsFrame->destinationEndpoint));
    obj.Set("options", Napi::Number::New(env, apsFrame->options));
    obj.Set("groupId", Napi::Number::New(env, apsFrame->groupId));
    obj.Set("sequence", Napi::Number::New(env, apsFrame->sequence));
    obj.Set("radius", Napi::Number::New(env, apsFrame->radius));

    return obj;
}

/**
 * Convert sl_zigbee_sec_man_context_t from JavaScript object to native struct
 * @param env Napi environment
 * @param contextObj JavaScript object `SLZigbeeSecManContext`
 * @param context Output native struct pointer
 * @return true on success, false on invalid object
 */
inline bool SecManContextFromObject(Napi::Env env, const Napi::Object &contextObj, sl_zigbee_sec_man_context_t *context)
{
    if (!contextObj.Has("coreKeyType") || !contextObj.Has("keyIndex") || !contextObj.Has("derivedType") || !contextObj.Has("eui64") ||
        !contextObj.Has("multiNetworkIndex") || !contextObj.Has("flags") || !contextObj.Has("psaKeyAlgPermission"))
    {
        return false;
    }

    memset(context, 0, sizeof(sl_zigbee_sec_man_context_t));
    context->core_key_type = contextObj.Get("coreKeyType").As<Napi::Number>().Uint32Value();
    context->key_index = contextObj.Get("keyIndex").As<Napi::Number>().Uint32Value();
    context->derived_type = contextObj.Get("derivedType").As<Napi::Number>().Uint32Value();

    // EUI64 comes as "0x..." hex string
    Napi::Value eui64Val = contextObj.Get("eui64");

    if (eui64Val.IsString())
    {
        if (!Eui64FromHexString(env, eui64Val.As<Napi::String>(), context->eui64))
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    context->multi_network_index = contextObj.Get("multiNetworkIndex").As<Napi::Number>().Uint32Value();
    context->flags = contextObj.Get("flags").As<Napi::Number>().Uint32Value();
    context->psa_key_alg_permission = contextObj.Get("psaKeyAlgPermission").As<Napi::Number>().Uint32Value();

    return true;
}

/**
 * Convert sl_zigbee_sec_man_context_t from native struct to JavaScript object
 * @param env Napi environment
 * @param context Native struct pointer
 * @return JavaScript object `SLZigbeeSecManContext`
 */
inline Napi::Object SecManContextToObject(Napi::Env env, const sl_zigbee_sec_man_context_t *context)
{
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("coreKeyType", Napi::Number::New(env, context->core_key_type));
    obj.Set("keyIndex", Napi::Number::New(env, context->key_index));
    obj.Set("derivedType", Napi::Number::New(env, context->derived_type));
    char hexString[19];
    Eui64ToHexString(context->eui64, hexString);
    obj.Set("eui64", Napi::String::New(env, hexString));
    obj.Set("multiNetworkIndex", Napi::Number::New(env, context->multi_network_index));
    obj.Set("flags", Napi::Number::New(env, context->flags));
    obj.Set("psaKeyAlgPermission", Napi::Number::New(env, context->psa_key_alg_permission));

    return obj;
}

/**
 * Convert sl_zigbee_sec_man_key_t from JavaScript object to native struct
 * @param env Napi environment
 * @param keyObj JavaScript object with `SLZigbeeKeyData`
 * @param key Output native struct pointer
 * @return true on success, false on invalid object
 */
inline bool SecManKeyFromObject(Napi::Env env, const Napi::Object &keyObj, sl_zigbee_sec_man_key_t *key)
{
    if (!keyObj.Has("contents"))
    {
        return false;
    }

    Napi::Value contents = keyObj.Get("contents");

    if (!contents.IsBuffer())
    {
        return false;
    }

    Napi::Buffer<uint8_t> keyBuffer = contents.As<Napi::Buffer<uint8_t>>();

    if (keyBuffer.Length() != 16)
    {
        return false;
    }

    memcpy(key->key, keyBuffer.Data(), 16);

    return true;
}

/**
 * Convert sl_zigbee_sec_man_key_t from native struct to JavaScript object
 * @param env Napi environment
 * @param key Native struct pointer
 * @return JavaScript object with `SLZigbeeKeyData`
 */
inline Napi::Object SecManKeyToObject(Napi::Env env, const sl_zigbee_sec_man_key_t *key)
{
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("contents", Napi::Buffer<uint8_t>::Copy(env, key->key, 16));

    return obj;
}

/**
 * Convert sl_zigbee_key_data_t from JavaScript object to native struct
 * @param env Napi environment
 * @param keyObj JavaScript object with `SLZigbeeKeyData`
 * @param key Output native struct pointer
 * @return true on success, false on invalid object
 */
inline bool ZigbeeKeyDataFromObject(Napi::Env env, const Napi::Object &keyObj, sl_zigbee_key_data_t *key)
{
    if (!keyObj.Has("contents"))
    {
        return false;
    }

    Napi::Value contents = keyObj.Get("contents");

    if (!contents.IsBuffer())
    {
        return false;
    }

    Napi::Buffer<uint8_t> keyBuffer = contents.As<Napi::Buffer<uint8_t>>();

    if (keyBuffer.Length() != 16)
    {
        return false;
    }

    memcpy(key->contents, keyBuffer.Data(), 16);

    return true;
}

/**
 * Convert sl_zigbee_key_data_t from native struct to JavaScript object
 * @param env Napi environment
 * @param key Native struct pointer
 * @return JavaScript object with `SLZigbeeKeyData`
 */
inline Napi::Object ZigbeeKeyDataToObject(Napi::Env env, const sl_zigbee_key_data_t *key)
{
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("contents", Napi::Buffer<uint8_t>::Copy(env, key->contents, 16));

    return obj;
}
// #endregion Helper Functions for Type Conversions

extern "C"
{
    extern sli_ash_host_config_t ashHostConfig;

    // #region EZSP Callbacks Bindings

    void sl_zigbee_ezsp_error_handler(sl_zigbee_ezsp_status_t status)
    {
        if (status != SL_ZIGBEE_EZSP_ERROR_QUEUE_FULL)
        {
            sl_zigbee_ezsp_print_elapsed_time();
            ezspDebugPrintf("EZSP: ERROR: sl_zigbee_ezsp_error_handler 0x%02X", status);
        }

        if (status == SL_ZIGBEE_EZSP_ERROR_OVERFLOW)
        {
            sl_zigbee_ezsp_print_elapsed_time();
            ezspDebugPrintf("EZSP: WARNING: the NCP has run out of buffers, causing general malfunction. Remediate network congestion, if present.");
        }

        bool ncpNeedsResetAndInit = false;

        // Do not reset if this is a decryption failure, as we ignored the packet
        // Do not reset for a callback overflow or error queue, as we don't want the device to reboot under stress;
        // Resetting under these conditions does not solve the problem as the problem is external to the NCP.
        // Throttling the additional traffic and staggering things might make it better instead.
        // For all other errors, we reset the NCP
        if ((status != SL_ZIGBEE_EZSP_ERROR_SECURITY_PARAMETERS_INVALID) && (status != SL_ZIGBEE_EZSP_ERROR_OVERFLOW) &&
            (status != SL_ZIGBEE_EZSP_ERROR_QUEUE_FULL))
        {
            ncpNeedsResetAndInit = true;
        }

        if (ncpNeedsResetAndInit && tsfn)
        {
            tsfn.BlockingCall(
                [status](Napi::Env env, Napi::Function jsCallback)
                {
                    Napi::Object event = Napi::Object::New(env);
                    event.Set("name", Napi::String::New(env, "ncpNeedsResetAndInit"));
                    event.Set("status", Napi::Number::New(env, status));
                    jsCallback.Call({event});
                });
        }
    }

    void sl_zigbee_ezsp_stack_status_handler(sl_status_t status)
    {
        if (tsfn)
        {
            tsfn.BlockingCall(
                [status](Napi::Env env, Napi::Function jsCallback)
                {
                    Napi::Object event = Napi::Object::New(env);
                    event.Set("name", Napi::String::New(env, "stackStatus"));
                    event.Set("status", Napi::Number::New(env, status));
                    jsCallback.Call({event});
                });
        }
    }

    void sl_zigbee_ezsp_message_sent_handler(sl_status_t status, sl_zigbee_outgoing_message_type_t type, uint16_t indexOrDestination,
                                             sl_zigbee_aps_frame_t *apsFrame, uint16_t messageTag, uint8_t messageLength, uint8_t *messageContents)
    {
        if (tsfn && apsFrame && messageContents)
        {
            // Capture data before async call
            uint8_t *msgCopy = new uint8_t[messageLength];
            memcpy(msgCopy, messageContents, messageLength);
            sl_zigbee_aps_frame_t frameCopy = *apsFrame;

            tsfn.BlockingCall(
                [status, type, indexOrDestination, frameCopy, messageTag, messageLength, msgCopy](Napi::Env env, Napi::Function jsCallback)
                {
                    Napi::Object event = Napi::Object::New(env);
                    event.Set("name", Napi::String::New(env, "messageSent"));
                    event.Set("status", Napi::Number::New(env, status));
                    event.Set("type", Napi::Number::New(env, type));
                    event.Set("indexOrDestination", Napi::Number::New(env, indexOrDestination));
                    event.Set("apsFrame", ApsFrameToObject(env, &frameCopy));
                    event.Set("messageTag", Napi::Number::New(env, messageTag));
                    event.Set("messageContents", Napi::Buffer<uint8_t>::Copy(env, msgCopy, messageLength));

                    jsCallback.Call({event});
                    delete[] msgCopy;
                });
        }
    }

    void sl_zigbee_ezsp_incoming_message_handler(sl_zigbee_incoming_message_type_t type, sl_zigbee_aps_frame_t *apsFrame,
                                                 sl_zigbee_rx_packet_info_t *packetInfo, uint8_t messageLength, uint8_t *message)
    {
        if (tsfn && apsFrame && packetInfo && message)
        {
            if (type != SL_ZIGBEE_INCOMING_BROADCAST_LOOPBACK && type != SL_ZIGBEE_INCOMING_MULTICAST_LOOPBACK)
            {
                // Capture data before async call
                uint8_t *msgCopy = new uint8_t[messageLength];
                memcpy(msgCopy, message, messageLength);
                sl_zigbee_aps_frame_t frameCopy = *apsFrame;
                sl_zigbee_rx_packet_info_t packetCopy = *packetInfo;

                if (apsFrame->profileId == 0)
                {
                    // ZDO
                    tsfn.BlockingCall(
                        [type, frameCopy, packetCopy, messageLength, msgCopy](Napi::Env env, Napi::Function jsCallback)
                        {
                            Napi::Object event = Napi::Object::New(env);
                            event.Set("name", Napi::String::New(env, "zdoResponse"));
                            event.Set("apsFrame", ApsFrameToObject(env, &frameCopy));
                            event.Set("sender", Napi::Number::New(env, packetCopy.sender_short_id));
                            event.Set("messageContents", Napi::Buffer<uint8_t>::Copy(env, msgCopy, messageLength));

                            jsCallback.Call({event});
                            delete[] msgCopy;
                        });
                }
                else
                {
                    // assumed ZCL
                    tsfn.BlockingCall(
                        [type, frameCopy, packetCopy, messageLength, msgCopy](Napi::Env env, Napi::Function jsCallback)
                        {
                            Napi::Object event = Napi::Object::New(env);
                            event.Set("name", Napi::String::New(env, "incomingMessage"));
                            event.Set("type", Napi::Number::New(env, type));
                            event.Set("apsFrame", ApsFrameToObject(env, &frameCopy));
                            event.Set("lastHopLqi", Napi::Number::New(env, packetCopy.last_hop_lqi));
                            event.Set("sender", Napi::Number::New(env, packetCopy.sender_short_id));
                            event.Set("messageContents", Napi::Buffer<uint8_t>::Copy(env, msgCopy, messageLength));

                            jsCallback.Call({event});
                            delete[] msgCopy;
                        });
                }
            }
        }
    }

    void sl_zigbee_ezsp_mac_filter_match_message_handler(sl_zigbee_mac_filter_match_data_t filterValueMatch,
                                                         sl_zigbee_mac_passthrough_type_t legacyPassthroughType,
                                                         sl_zigbee_rx_packet_info_t *packetInfo, uint8_t messageLength, uint8_t *messageContents)
    {
        if (tsfn && packetInfo && messageContents)
        {
            // cf `parseInterpanMessage` in `simplicity_sdk/protocol/zigbee/app/framework/plugin/interpan/interpan.c`
            uint8_t *finger = messageContents;

            // We rely on the stack to insure that the MAC frame is formatted
            // correctly and that the length is at least long enough
            // to contain that frame.

            uint16_t macFrameControl = HIGH_LOW_TO_INT(finger[1], finger[0]) & ~(MAC_ACK_REQUIRED);

            if (macFrameControl == LONG_DEST_FRAME_CONTROL)
            {
                // control, sequence, dest PAN ID, long dest
                finger += 2 + 1 + 2 + 8;
            }
            else if (macFrameControl == SHORT_DEST_FRAME_CONTROL)
            {
                // control, sequence, dest PAN ID, short dest
                finger += 2 + 1 + 2 + 2;
            }
            else
            {
                return;
            }

            uint16_t panId = HIGH_LOW_TO_INT(finger[1], finger[0]);
            finger += 2;
            sl_802154_long_addr_t longAddress;
            memmove(longAddress, finger, 8);
            finger += 8;

            uint8_t remainingLength = messageLength - (uint8_t)(finger - messageContents);

            if (remainingLength < (STUB_NWK_SIZE + MIN_STUB_APS_SIZE))
            {
                return;
            }

            if (HIGH_LOW_TO_INT(finger[1], finger[0]) != STUB_NWK_FRAME_CONTROL)
            {
                return;
            }

            finger += 2;
            remainingLength -= 2;

            uint8_t apsFrameControl = (*finger++);

            if ((apsFrameControl & ~(INTERPAN_APS_FRAME_DELIVERY_MODE_MASK) & ~INTERPAN_APS_FRAME_SECURITY) !=
                INTERPAN_APS_FRAME_CONTROL_NO_DELIVERY_MODE)
            {
                fprintf(stderr, "ERROR: Inter-PAN Bad APS frame control 0x%02X", apsFrameControl);
                return;
            }

            if (apsFrameControl & INTERPAN_APS_FRAME_SECURITY)
            {
                // !ALLOW_APS_ENCRYPTED_MESSAGES => SL_STATUS_NOT_AVAILABLE
                return;
            }

            uint8_t messageType = (apsFrameControl & INTERPAN_APS_FRAME_DELIVERY_MODE_MASK);
            uint16_t groupId = 0;

            switch (messageType)
            {
            case SL_ZIGBEE_AF_INTER_PAN_UNICAST:
            case SL_ZIGBEE_AF_INTER_PAN_BROADCAST:
                // Broadcast and unicast have the same size messages
                if (remainingLength < INTERPAN_APS_UNICAST_SIZE)
                {
                    return;
                }

                break;
            case SL_ZIGBEE_AF_INTER_PAN_MULTICAST:
                if (remainingLength < INTERPAN_APS_MULTICAST_SIZE)
                {
                    return;
                }

                groupId = HIGH_LOW_TO_INT(finger[1], finger[0]);
                finger += 2;

                break;
            default:
                fprintf(stderr, "ERROR: Inter-PAN Bad Delivery Mode 0x%02X", messageType);
                return;
            }

            uint16_t clusterId = HIGH_LOW_TO_INT(finger[1], finger[0]);
            finger += 2;

            if (clusterId != 0x1000)
            {
                // not TOUCHLINK
                return;
            }

            uint16_t profileId = HIGH_LOW_TO_INT(finger[1], finger[0]);
            finger += 2;

            if (profileId != 0xc05e)
            {
                // not TOUCHLINK
                return;
            }

            uint8_t payloadOffset = (finger - messageContents);

            if (payloadOffset == 0)
            {
                return;
            }

            uint8_t *payload = messageContents + payloadOffset;
            uint8_t payloadLength = messageLength - payloadOffset;

            uint8_t *payloadCopy = new uint8_t[payloadLength];
            memcpy(payloadCopy, payload, payloadLength);
            sl_zigbee_rx_packet_info_t packetCopy = *packetInfo;

            char sourceAddressRaw[19];
            Eui64ToHexString(longAddress, sourceAddressRaw);
            std::string sourceAddress(sourceAddressRaw);

            tsfn.BlockingCall(
                [panId, sourceAddress, groupId, packetCopy, payloadLength, payloadCopy](Napi::Env env, Napi::Function jsCallback)
                {
                    Napi::Object event = Napi::Object::New(env);
                    event.Set("name", Napi::String::New(env, "touchlinkMessage"));
                    event.Set("sourcePanId", Napi::Number::New(env, panId));
                    event.Set("sourceAddress", Napi::String::New(env, sourceAddress));
                    event.Set("groupId", Napi::Number::New(env, groupId));
                    event.Set("lastHopLqi", Napi::Number::New(env, packetCopy.last_hop_lqi));
                    event.Set("messageContents", Napi::Buffer<uint8_t>::Copy(env, payloadCopy, payloadLength));

                    jsCallback.Call({event});
                    delete[] payloadCopy;
                });
        }
    }

    void sl_zigbee_ezsp_trust_center_post_join_handler(sl_802154_short_addr_t newNodeId, sl_802154_long_addr_t newNodeEui64,
                                                       sl_zigbee_device_update_t status, sl_zigbee_join_decision_t policyDecision,
                                                       sl_802154_short_addr_t parentOfNewNodeId)
    {
        if (tsfn)
        {
            char hexStringRaw[19];
            Eui64ToHexString(newNodeEui64, hexStringRaw);
            std::string hexString(hexStringRaw);

            tsfn.BlockingCall(
                [newNodeId, hexString, status, policyDecision, parentOfNewNodeId](Napi::Env env, Napi::Function jsCallback)
                {
                    Napi::Object event = Napi::Object::New(env);
                    event.Set("name", Napi::String::New(env, "trustCenterJoin"));
                    event.Set("newNodeId", Napi::Number::New(env, newNodeId));
                    event.Set("newNodeEui64", Napi::String::New(env, hexString));
                    event.Set("status", Napi::Number::New(env, status));
                    event.Set("policyDecision", Napi::Number::New(env, policyDecision));
                    event.Set("parentOfNewNodeId", Napi::Number::New(env, parentOfNewNodeId));
                    jsCallback.Call({event});
                });
        }
    }

    void sl_zigbee_ezsp_gpep_incoming_message_handler(sl_zigbee_gp_params_t *param)
    {
        if (tsfn && param)
        {
            // ZCL frame transformation
            // XXX: specific to zigbee-herdsman
            if (param->addr.applicationId == SL_ZIGBEE_GP_APPLICATION_IEEE_ADDRESS)
            {
                fprintf(stderr, "ERROR: GreenPower Unsupported IEEE application ID");
                return;
            }

            uint8_t commandIdentifier = 0x00;
            uint16_t options = 0;

            if (param->gpdCommandId == 0xe0)
            {
                // commissioning
                if (param->gpdCommandPayloadLength == 0)
                {
                    // XXX: seem to be receiving duplicate commissioningNotification from some devices, second one with empty payload?
                    //      this will mess with the process no doubt, so dropping them
                    return;
                }

                commandIdentifier = 0x04;
                options = (param->addr.applicationId & 0x7) | ((param->bidirectionalInfo & 0x1) << 3) | ((param->gpdfSecurityLevel & 0x3) << 4) |
                          ((param->gpdfSecurityKeyType & 0x7) << 6);
            }
            else
            {
                options = (param->addr.applicationId & 0x7) | ((param->gpdfSecurityLevel & 0x3) << 6) | ((param->gpdfSecurityKeyType & 0x7) << 8) |
                          ((param->bidirectionalInfo & 0x1) << 11);
            }

            sl_zigbee_aps_frame_t apsFrame = {0};
            apsFrame.profileId = 0xa1e0;         // GP
            apsFrame.clusterId = 0x0021;         // GP
            apsFrame.sourceEndpoint = 0xf2;      // GP
            apsFrame.destinationEndpoint = 0xf2; // GP
            apsFrame.options = 0;                // not used
            apsFrame.groupId = 0x0b84;           // GP
            apsFrame.sequence = 0;               // not used

            uint8_t messageLength = 15 + param->gpdCommandPayloadLength;
            uint8_t *msgCopy = new uint8_t[messageLength];

            uint8_t *finger = msgCopy;
            finger[0] = 0x01;
            finger[1] = param->sequenceNumber;
            finger[2] = commandIdentifier;
            finger[3] = options & 0xFF;
            finger[4] = (options >> 8) & 0xFF;
            finger[5] = param->addr.id.sourceId & 0xFF;
            finger[6] = (param->addr.id.sourceId >> 8) & 0xFF;
            finger[7] = (param->addr.id.sourceId >> 16) & 0xFF;
            finger[8] = (param->addr.id.sourceId >> 24) & 0xFF;
            finger[9] = param->gpdSecurityFrameCounter & 0xFF;
            finger[10] = (param->gpdSecurityFrameCounter >> 8) & 0xFF;
            finger[11] = (param->gpdSecurityFrameCounter >> 16) & 0xFF;
            finger[12] = (param->gpdSecurityFrameCounter >> 24) & 0xFF;
            finger[13] = param->gpdCommandId;
            finger[14] = param->gpdCommandPayloadLength;
            memcpy(&finger[15], param->gpdCommandPayload, param->gpdCommandPayloadLength);

            uint8_t lastHopLqi = param->packetInfo.last_hop_lqi;
            // convert to uint16_t for regular Zigbee node ID
            uint16_t sourceId = param->addr.id.sourceId & 0xffff;

            tsfn.BlockingCall(
                [apsFrame, lastHopLqi, sourceId, messageLength, msgCopy](Napi::Env env, Napi::Function jsCallback)
                {
                    Napi::Object event = Napi::Object::New(env);
                    event.Set("name", Napi::String::New(env, "incomingMessage"));
                    event.Set("type", Napi::Number::New(env, SL_ZIGBEE_INCOMING_UNICAST));
                    event.Set("apsFrame", ApsFrameToObject(env, &apsFrame));
                    event.Set("lastHopLqi", Napi::Number::New(env, lastHopLqi));
                    event.Set("sender", Napi::Number::New(env, sourceId));
                    event.Set("messageContents", Napi::Buffer<uint8_t>::Copy(env, msgCopy, messageLength));

                    jsCallback.Call({event});
                    delete[] msgCopy;
                });
        }
    }

    void sl_zigbee_ezsp_incoming_route_error_handler(sl_status_t status, sl_802154_short_addr_t target)
    {
        sl_zigbee_ezsp_print_elapsed_time();
        ezspDebugPrintf("EZSP: ERROR: Routing error 0x%02X for 0x%04X\r\n", status, target);
    }

    void sl_zigbee_ezsp_incoming_network_status_handler(uint8_t errorCode, sl_802154_short_addr_t target)
    {
        sl_zigbee_ezsp_print_elapsed_time();
        ezspDebugPrintf("EZSP: ERROR: Routing error 0x%02X for 0x%04X", errorCode, target);
    }

    void sl_zigbee_ezsp_id_conflict_handler(sl_802154_short_addr_t id)
    {
        sl_zigbee_ezsp_print_elapsed_time();
        ezspDebugPrintf("EZSP: ERROR: ID conflict for 0x%04X", id);
    }

    void sl_zigbee_ezsp_zigbee_key_establishment_handler(sl_802154_long_addr_t partner, sl_zigbee_key_status_t status)
    {
        char partnerEui64[19];
        Eui64ToHexString(partner, partnerEui64);

        sl_zigbee_ezsp_print_elapsed_time();
        ezspDebugPrintf("EZSP: Key establishment status 0x%02X for 0x%s", status, partnerEui64);
    }

    // #endregion EZSP Callbacks Bindings
}

namespace EzspNapi
{
    Napi::Value Init(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsObject())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Object config = info[0].As<Napi::Object>();

        if (!config.Has("baudRate") || !config.Has("stopBits") || !config.Has("rtsCts") || !config.Has("outBlockLen") || !config.Has("inBlockLen") ||
            !config.Has("traceFlags") || !config.Has("txK") || !config.Has("randomize") || !config.Has("ackTimeInit") || !config.Has("ackTimeMin") ||
            !config.Has("ackTimeMax") || !config.Has("timeRst") || !config.Has("nrLowLimit") || !config.Has("nrHighLimit") || !config.Has("nrTime") ||
            !config.Has("resetMethod"))
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        std::string serialPort = config.Has("serialPort") ? config.Get("serialPort").As<Napi::String>().Utf8Value() : "/dev/ttyS0";

        if (serialPort.length() > 39)
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        strncpy(ashHostConfig.serialPort, serialPort.c_str(), sizeof(ashHostConfig.serialPort) - 1);
        ashHostConfig.serialPort[sizeof(ashHostConfig.serialPort) - 1] = '\0';
        ashHostConfig.baudRate = config.Get("baudRate").As<Napi::Number>().Uint32Value();
        ashHostConfig.stopBits = config.Get("stopBits").As<Napi::Number>().Uint32Value();
        ashHostConfig.rtsCts = config.Get("rtsCts").As<Napi::Boolean>().Value();
        ashHostConfig.outBlockLen = config.Get("outBlockLen").As<Napi::Number>().Uint32Value();
        ashHostConfig.inBlockLen = config.Get("inBlockLen").As<Napi::Number>().Uint32Value();
        ashHostConfig.traceFlags = config.Get("traceFlags").As<Napi::Number>().Uint32Value();
        ashHostConfig.txK = config.Get("txK").As<Napi::Number>().Uint32Value();
        ashHostConfig.randomize = config.Get("randomize").As<Napi::Boolean>().Value();
        ashHostConfig.ackTimeInit = config.Get("ackTimeInit").As<Napi::Number>().Uint32Value();
        ashHostConfig.ackTimeMin = config.Get("ackTimeMin").As<Napi::Number>().Uint32Value();
        ashHostConfig.ackTimeMax = config.Get("ackTimeMax").As<Napi::Number>().Uint32Value();
        ashHostConfig.timeRst = config.Get("timeRst").As<Napi::Number>().Uint32Value();
        ashHostConfig.nrLowLimit = config.Get("nrLowLimit").As<Napi::Number>().Uint32Value();
        ashHostConfig.nrHighLimit = config.Get("nrHighLimit").As<Napi::Number>().Uint32Value();
        ashHostConfig.nrTime = config.Get("nrTime").As<Napi::Number>().Uint32Value();
        ashHostConfig.resetMethod = config.Get("resetMethod").As<Napi::Number>().Uint32Value();

        // Register callback handler if provided
        if (info.Length() >= 2)
        {
            if (!info[1].IsFunction())
            {
                Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
                return env.Undefined();
            }

            Napi::Function callback = info[1].As<Napi::Function>();
            tsfn = Napi::ThreadSafeFunction::New(env, callback, "EZSP Callback", 0, 1);
        }

        initialized = true;

        return env.Undefined();
    }

    Napi::Value Start(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (!initialized)
        {
            Napi::Error::New(env, "Not initialized - call init() first").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        ezspSequenceNumber = 0;

        // Initialize EZSP (resets NCP and starts ASH protocol)
        sl_zigbee_ezsp_status_t status = sl_zigbee_ezsp_init();

        // Start tick timer for EZSP event processing (1ms interval)
        if (status == SL_ZIGBEE_EZSP_SUCCESS && !tickTimerActive)
        {
            uv_loop_t *loop = uv_default_loop();
            uv_timer_init(loop, &tickTimer);
            uv_timer_start(&tickTimer, ezspTickCallback, 1, 1); // 1ms initial, 1ms repeat

            tickTimerActive = true;
        }

        return Napi::Number::New(env, status);
    }

    Napi::Value Stop(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        // Stop tick timer
        if (tickTimerActive)
        {
            uv_timer_stop(&tickTimer);
            uv_close((uv_handle_t *)&tickTimer, nullptr);

            tickTimerActive = false;
        }

        if (initialized)
        {
            // Stop ASH protocol and cleanup serialPort
            ashStop();

            initialized = false;
        }

        if (tsfn)
        {
            tsfn.Release();
        }

        return env.Undefined();
    }

    // #region EZSP Command Bindings

    // Base Commands

    Napi::Value Version(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsNumber())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        uint8_t desiredVersion = info[0].As<Napi::Number>().Uint32Value();

        uint8_t stackType = 0;
        uint16_t stackVersion = 0;
        uint8_t protocolVersion = sl_zigbee_ezsp_version(desiredVersion, &stackType, &stackVersion);

        // enforce protocol match (binding = 1 version supported)
        if (protocolVersion != EZSP_PROTOCOL_VERSION)
        {
            Napi::TypeError::New(env, "ERROR: NCP EZSP protocol version does not match Host version! " + std::to_string(protocolVersion) + " vs " +
                                          std::to_string(EZSP_PROTOCOL_VERSION))
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Array result = Napi::Array::New(env, 3);
        result[0u] = Napi::Number::New(env, protocolVersion);
        result[1u] = Napi::Number::New(env, stackType);
        result[2u] = Napi::Number::New(env, stackVersion);

        return result;
    }

    Napi::Value GetEui64(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        uint8_t eui64[8];

        sl_zigbee_ezsp_get_eui64(eui64);

        char hexString[19];
        Eui64ToHexString(eui64, hexString);

        return Napi::String::New(env, hexString);
    }

    // Network Management Commands

    Napi::Value GetNetworkParameters(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        sl_zigbee_node_type_t nodeType;

        sl_zigbee_network_parameters_t params;
        sl_status_t status = sl_zigbee_ezsp_get_network_parameters(&nodeType, &params);

        Napi::Array result = Napi::Array::New(env, 3);
        result[0u] = Napi::Number::New(env, status);

        if (status == SL_STATUS_OK)
        {
            result[1u] = nodeType;

            Napi::Object response = Napi::Object::New(env);
            response.Set("extendedPanId", Uint8ArrayToNumberArray(env, params.extendedPanId, 8));
            response.Set("panId", Napi::Number::New(env, params.panId));
            response.Set("radioTxPower", Napi::Number::New(env, params.radioTxPower));
            response.Set("radioChannel", Napi::Number::New(env, params.radioChannel));
            response.Set("joinMethod", Napi::Number::New(env, params.joinMethod));
            response.Set("nwkManagerId", Napi::Number::New(env, params.nwkManagerId));
            response.Set("nwkUpdateId", Napi::Number::New(env, params.nwkUpdateId));
            response.Set("channels", Napi::Number::New(env, params.channels));

            result[2u] = response;
        }

        return result;
    }

    Napi::Value NetworkInit(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsObject())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Object paramsObj = info[0].As<Napi::Object>();

        if (!paramsObj.Has("bitmask"))
        {
            Napi::TypeError::New(env, "Invalid init struct object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        sl_zigbee_network_init_struct_t initStruct = {0};
        initStruct.bitmask = paramsObj.Get("bitmask").As<Napi::Number>().Uint32Value();

        sl_status_t status = sl_zigbee_ezsp_network_init(&initStruct);

        return Napi::Number::New(env, status);
    }

    Napi::Value NetworkState(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        sl_zigbee_network_status_t status = sl_zigbee_ezsp_network_state();

        return Napi::Number::New(env, status);
    }

    Napi::Value FormNetwork(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsObject())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Object paramsObj = info[0].As<Napi::Object>();

        if (!paramsObj.Has("extendedPanId") || !paramsObj.Has("panId") || !paramsObj.Has("radioTxPower") || !paramsObj.Has("radioChannel") ||
            !paramsObj.Has("joinMethod") || !paramsObj.Has("nwkManagerId") || !paramsObj.Has("nwkUpdateId") || !paramsObj.Has("channels"))
        {
            Napi::TypeError::New(env, "Invalid network parameters object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        sl_zigbee_network_parameters_t params = {0};

        Napi::Value extPanIdVal = paramsObj.Get("extendedPanId");

        if (Uint8ArrayFromNumberArray(env, extPanIdVal, params.extendedPanId, 8) == false)
        {
            Napi::TypeError::New(env, "Invalid extendedPanId - must be array of 8 length").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        params.panId = paramsObj.Get("panId").As<Napi::Number>().Uint32Value();
        params.radioTxPower = paramsObj.Get("radioTxPower").As<Napi::Number>().Int32Value();
        params.radioChannel = paramsObj.Get("radioChannel").As<Napi::Number>().Uint32Value();
        params.joinMethod = paramsObj.Get("joinMethod").As<Napi::Number>().Uint32Value();
        params.nwkManagerId = paramsObj.Get("nwkManagerId").As<Napi::Number>().Uint32Value();
        params.nwkUpdateId = paramsObj.Get("nwkUpdateId").As<Napi::Number>().Uint32Value();
        params.channels = paramsObj.Get("channels").As<Napi::Number>().Uint32Value();

        sl_status_t status = sl_zigbee_ezsp_form_network(&params);

        return Napi::Number::New(env, status);
    }

    Napi::Value LeaveNetwork(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        sl_zigbee_leave_network_option_t options = info.Length() >= 1 && info[0].IsNumber() ? info[0].As<Napi::Number>().Uint32Value() : 0;

        sl_status_t status = sl_zigbee_ezsp_leave_network(options);

        return Napi::Number::New(env, status);
    }

    Napi::Value PermitJoining(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsNumber())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        uint8_t duration = info[0].As<Napi::Number>().Uint32Value();

        sl_status_t status = sl_zigbee_ezsp_permit_joining(duration);

        return Napi::Number::New(env, status);
    }

    // Configuration Commands

    Napi::Value GetConfigurationValue(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsNumber())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        sl_zigbee_ezsp_config_id_t configId = info[0].As<Napi::Number>().Uint32Value();

        uint16_t value = 0;
        sl_status_t status = sl_zigbee_ezsp_get_configuration_value(configId, &value);

        Napi::Array result = Napi::Array::New(env, 2);
        result[0u] = Napi::Number::New(env, status);
        result[1u] = (status == SL_STATUS_OK) ? Napi::Number::New(env, value) : env.Null();

        return result;
    }

    Napi::Value SetConfigurationValue(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        sl_zigbee_ezsp_config_id_t configId = info[0].As<Napi::Number>().Uint32Value();
        uint16_t value = info[1].As<Napi::Number>().Uint32Value();

        sl_status_t status = sl_zigbee_ezsp_set_configuration_value(configId, value);

        return Napi::Number::New(env, status);
    }

    Napi::Value GetValue(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsNumber())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        sl_zigbee_ezsp_value_id_t valueId = info[0].As<Napi::Number>().Uint32Value();

        uint8_t valueLength = 0;
        uint8_t value[255] = {0};
        sl_status_t status = sl_zigbee_ezsp_get_value(valueId, &valueLength, value);

        Napi::Array result = Napi::Array::New(env, 3);
        result[0u] = Napi::Number::New(env, status);

        if (status == SL_STATUS_OK)
        {
            result[1u] = Napi::Number::New(env, valueLength);
            result[2u] = Napi::Buffer<uint8_t>::Copy(env, value, valueLength);
        }

        return result;
    }

    Napi::Value SetValue(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 3 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsArray())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        sl_zigbee_ezsp_value_id_t valueId = info[0].As<Napi::Number>().Uint32Value();
        uint8_t valueLength = info[1].As<Napi::Number>().Uint32Value();
        uint8_t value[255] = {0};

        if (Uint8ArrayFromNumberArray(env, info[2], value, valueLength) == false)
        {
            Napi::TypeError::New(env, "Invalid value - must be array of given length").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        sl_status_t status = sl_zigbee_ezsp_set_value(valueId, valueLength, value);

        return Napi::Number::New(env, status);
    }

    Napi::Value GetExtendedValue(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        sl_zigbee_ezsp_extended_value_id_t extendedValueId = info[0].As<Napi::Number>().Uint32Value();
        uint32_t characteristics = info[1].As<Napi::Number>().Uint32Value();

        uint8_t valueLength = 0;
        uint8_t value[255] = {0};
        sl_status_t status = sl_zigbee_ezsp_get_extended_value(extendedValueId, characteristics, &valueLength, value);

        Napi::Array result = Napi::Array::New(env, 3);
        result[0u] = Napi::Number::New(env, status);

        if (status == SL_STATUS_OK)
        {
            result[1u] = Napi::Number::New(env, valueLength);
            result[2u] = Napi::Buffer<uint8_t>::Copy(env, value, valueLength);
        }

        return result;
    }

    Napi::Value SetPolicy(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        sl_zigbee_ezsp_policy_id_t policyId = info[0].As<Napi::Number>().Uint32Value();
        sl_zigbee_ezsp_decision_id_t decisionId = info[1].As<Napi::Number>().Uint32Value();

        sl_status_t status = sl_zigbee_ezsp_set_policy(policyId, decisionId);

        return Napi::Number::New(env, status);
    }

    Napi::Value TokenFactoryReset(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 2 || !info[0].IsBoolean() || !info[1].IsBoolean())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        bool excludeOutgoingFC = info[0].As<Napi::Boolean>().Value();
        bool excludeBootCounter = info[1].As<Napi::Boolean>().Value();

        sl_zigbee_ezsp_token_factory_reset(excludeOutgoingFC, excludeBootCounter);

        return env.Undefined();
    }

    // Security Commands

    Napi::Value SetInitialSecurityState(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsObject())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Object secStateObj = info[0].As<Napi::Object>();

        if (!secStateObj.Has("bitmask") || !secStateObj.Has("preconfiguredKey") || !secStateObj.Has("networkKey") ||
            !secStateObj.Has("networkKeySequenceNumber") || !secStateObj.Has("preconfiguredTrustCenterEui64"))
        {
            Napi::TypeError::New(env, "Invalid security state object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        sl_zigbee_initial_security_state_t securityState = {0};
        securityState.bitmask = secStateObj.Get("bitmask").As<Napi::Number>().Uint32Value();

        Napi::Object preconfiguredKeyObj = secStateObj.Get("preconfiguredKey").As<Napi::Object>();

        if (!ZigbeeKeyDataFromObject(env, preconfiguredKeyObj, &securityState.preconfiguredKey))
        {
            Napi::TypeError::New(env, "Invalid preconfigured key object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Object networkKeyObj = secStateObj.Get("networkKey").As<Napi::Object>();

        if (!ZigbeeKeyDataFromObject(env, networkKeyObj, &securityState.networkKey))
        {
            Napi::TypeError::New(env, "Invalid network key object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        securityState.networkKeySequenceNumber = secStateObj.Get("networkKeySequenceNumber").As<Napi::Number>().Uint32Value();

        Napi::Value euiVal = secStateObj.Get("preconfiguredTrustCenterEui64");

        if (!euiVal.IsString() || !Eui64FromHexString(env, euiVal.As<Napi::String>(), securityState.preconfiguredTrustCenterEui64))
        {
            Napi::TypeError::New(env, "Invalid preconfiguredTrustCenterEui64 - must be hex string like 0x1122334455667788")
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }

        sl_status_t status = sl_zigbee_ezsp_set_initial_security_state(&securityState);

        return Napi::Number::New(env, status);
    }

    Napi::Value GetNetworkKeyInfo(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        sl_zigbee_sec_man_network_key_info_t networkKeyInfo = {0};
        sl_status_t status = sl_zigbee_ezsp_sec_man_get_network_key_info(&networkKeyInfo);

        Napi::Array result = Napi::Array::New(env, 2);
        result[0u] = Napi::Number::New(env, status);

        if (status == SL_STATUS_OK)
        {
            Napi::Object response = Napi::Object::New(env);
            response.Set("networkKeySet", Napi::Boolean::New(env, networkKeyInfo.network_key_set));
            response.Set("alternateNetworkKeySet", Napi::Boolean::New(env, networkKeyInfo.alternate_network_key_set));
            response.Set("networkKeySequenceNumber", Napi::Number::New(env, networkKeyInfo.network_key_sequence_number));
            response.Set("altNetworkKeySequenceNumber", Napi::Number::New(env, networkKeyInfo.alt_network_key_sequence_number));
            response.Set("networkKeyFrameCounter", Napi::Number::New(env, networkKeyInfo.network_key_frame_counter));

            result[1u] = response;
        }

        return result;
    }

    Napi::Value GetApsKeyInfo(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsObject())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Object contextObj = info[0].As<Napi::Object>();

        if (!contextObj.Has("coreKeyType") || !contextObj.Has("keyIndex") || !contextObj.Has("derivedType") || !contextObj.Has("eui64") ||
            !contextObj.Has("multiNetworkIndex") || !contextObj.Has("flags") || !contextObj.Has("psaKeyAlgPermission"))
        {
            Napi::TypeError::New(env, "Invalid context").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        sl_zigbee_sec_man_context_t context = {0};
        context.core_key_type = contextObj.Get("coreKeyType").As<Napi::Number>().Uint32Value();
        context.key_index = contextObj.Get("keyIndex").As<Napi::Number>().Uint32Value();
        context.derived_type = contextObj.Get("derivedType").As<Napi::Number>().Uint32Value();

        if (!Eui64FromHexString(env, contextObj.Get("eui64").As<Napi::String>(), context.eui64))
        {
            Napi::TypeError::New(env, "Invalid context").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        context.multi_network_index = contextObj.Get("multiNetworkIndex").As<Napi::Number>().Uint32Value();
        context.flags = contextObj.Get("flags").As<Napi::Number>().Uint32Value();
        context.psa_key_alg_permission = contextObj.Get("psaKeyAlgPermission").As<Napi::Number>().Uint32Value();

        sl_zigbee_sec_man_aps_key_metadata_t key_data = {0};
        sl_status_t status = sl_zigbee_ezsp_sec_man_get_aps_key_info(&context, &key_data);

        Napi::Array result = Napi::Array::New(env, 2);
        result[0u] = Napi::Number::New(env, status);

        if (status == SL_STATUS_OK)
        {
            Napi::Object response = Napi::Object::New(env);
            response.Set("bitmask", Napi::Number::New(env, key_data.bitmask));
            response.Set("outgoingFrameCounter", Napi::Number::New(env, key_data.outgoing_frame_counter));
            response.Set("incomingFrameCounter", Napi::Number::New(env, key_data.incoming_frame_counter));
            response.Set("ttlInSeconds", Napi::Number::New(env, key_data.ttl_in_seconds));

            result[1u] = response;
        }

        return result;
    }

    Napi::Value ExportKey(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsObject())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Object contextObj = info[0].As<Napi::Object>();

        sl_zigbee_sec_man_context_t context = {0};
        if (!SecManContextFromObject(env, contextObj, &context))
        {
            Napi::TypeError::New(env, "Invalid context object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        sl_zigbee_sec_man_key_t plaintext_key = {0};
        sl_status_t status = sl_zigbee_ezsp_sec_man_export_key(&context, &plaintext_key);

        Napi::Array result = Napi::Array::New(env, 2);
        result[0u] = Napi::Number::New(env, status);

        if (status == SL_STATUS_OK)
        {
            result[1u] = SecManKeyToObject(env, &plaintext_key);
        }

        return result;
    }

    Napi::Value ExportLinkKeyByIndex(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsNumber())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        uint8_t index = info[0].As<Napi::Number>().Uint32Value();
        sl_zigbee_sec_man_context_t context = {0};
        sl_zigbee_sec_man_key_t plaintext_key = {0};
        sl_zigbee_sec_man_aps_key_metadata_t key_data = {0};

        sl_status_t status = sl_zigbee_ezsp_sec_man_export_link_key_by_index(index, &context, &plaintext_key, &key_data);

        Napi::Array result = Napi::Array::New(env, 4);
        result[0u] = Napi::Number::New(env, status);

        if (status == SL_STATUS_OK)
        {
            result[1u] = SecManContextToObject(env, &context);
            result[2u] = SecManKeyToObject(env, &plaintext_key);

            Napi::Object keyDataResponse = Napi::Object::New(env);
            keyDataResponse.Set("bitmask", Napi::Number::New(env, key_data.bitmask));
            keyDataResponse.Set("outgoingFrameCounter", Napi::Number::New(env, key_data.outgoing_frame_counter));
            keyDataResponse.Set("incomingFrameCounter", Napi::Number::New(env, key_data.incoming_frame_counter));
            keyDataResponse.Set("ttlInSeconds", Napi::Number::New(env, key_data.ttl_in_seconds));

            result[3u] = keyDataResponse;
        }

        return result;
    }

    Napi::Value ImportLinkKey(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 3 || !info[0].IsNumber() || !info[1].IsString() || !info[2].IsObject())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        uint8_t index = info[0].As<Napi::Number>().Uint32Value();
        sl_802154_long_addr_t address = {0};

        if (!info[1].IsString() || !Eui64FromHexString(env, info[1].As<Napi::String>(), address))
        {
            Napi::TypeError::New(env, "Invalid address - must be hex string like 0x1122334455667788").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Object plaintextKeyObj = info[2].As<Napi::Object>();
        sl_zigbee_sec_man_key_t plaintext_key = {0};

        if (!SecManKeyFromObject(env, plaintextKeyObj, &plaintext_key))
        {
            Napi::TypeError::New(env, "Invalid key object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        sl_status_t status = sl_zigbee_ezsp_sec_man_import_link_key(index, address, &plaintext_key);

        return Napi::Number::New(env, status);
    }

    Napi::Value ImportTransientKey(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 2 || !info[0].IsString() || !info[1].IsObject())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        sl_802154_long_addr_t eui64;

        if (!info[0].IsString() || !Eui64FromHexString(env, info[0].As<Napi::String>(), eui64))
        {
            Napi::TypeError::New(env, "Invalid EUI64 - must be hex string like 0x1122334455667788").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Object plaintextKeyObj = info[1].As<Napi::Object>();
        sl_zigbee_sec_man_key_t plaintext_key = {0};

        if (!SecManKeyFromObject(env, plaintextKeyObj, &plaintext_key))
        {
            Napi::TypeError::New(env, "Invalid key object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        sl_status_t status = sl_zigbee_ezsp_sec_man_import_transient_key(eui64, &plaintext_key);

        return Napi::Number::New(env, status);
    }

    Napi::Value EraseKeyTableEntry(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsNumber())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        uint8_t index = info[0].As<Napi::Number>().Uint32Value();

        sl_status_t status = sl_zigbee_ezsp_erase_key_table_entry(index);

        return Napi::Number::New(env, status);
    }

    Napi::Value ClearKeyTable(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        sl_status_t status = sl_zigbee_ezsp_clear_key_table();

        return Napi::Number::New(env, status);
    }

    Napi::Value ClearTransientLinkKeys(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        sl_zigbee_ezsp_clear_transient_link_keys();

        return env.Undefined();
    }

    Napi::Value BroadcastNextNetworkKey(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsObject())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Object keyObj = info[0].As<Napi::Object>();
        sl_zigbee_key_data_t key = {0};

        if (!ZigbeeKeyDataFromObject(env, keyObj, &key))
        {
            Napi::TypeError::New(env, "Invalid key object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        sl_status_t status = sl_zigbee_ezsp_broadcast_next_network_key(&key);

        return Napi::Number::New(env, status);
    }

    Napi::Value BroadcastNetworkKeySwitch(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        sl_status_t status = sl_zigbee_ezsp_broadcast_network_key_switch();

        return Napi::Number::New(env, status);
    }

    // Messaging Commands

    Napi::Value SendUnicast(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 5 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsObject() || !info[3].IsNumber() || !info[4].IsBuffer())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        sl_zigbee_outgoing_message_type_t type = info[0].As<Napi::Number>().Uint32Value();
        uint16_t indexOrDestination = info[1].As<Napi::Number>().Uint32Value();
        Napi::Object apsFrameObj = info[2].As<Napi::Object>();

        sl_zigbee_aps_frame_t apsFrame = {0};
        if (!ApsFrameFromObject(env, apsFrameObj, &apsFrame))
        {
            Napi::TypeError::New(env, "Invalid aps frame object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        uint16_t messageTag = info[3].As<Napi::Number>().Uint32Value();
        Napi::Buffer<uint8_t> messageBuffer = info[4].As<Napi::Buffer<uint8_t>>();

        uint8_t sequence = 0;
        sl_status_t status =
            sl_zigbee_ezsp_send_unicast(type, indexOrDestination, &apsFrame, messageTag, messageBuffer.Length(), messageBuffer.Data(), &sequence);

        Napi::Array result = Napi::Array::New(env, 2);
        result[0u] = Napi::Number::New(env, status);
        result[1u] = Napi::Number::New(env, sequence);

        return result;
    }

    Napi::Value SendMulticast(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 7 || !info[0].IsObject() || !info[1].IsNumber() || !info[2].IsNumber() || !info[3].IsNumber() || !info[4].IsNumber() ||
            !info[5].IsNumber() || !info[6].IsBuffer())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Object apsFrameObj = info[0].As<Napi::Object>();

        sl_zigbee_aps_frame_t apsFrame = {0};
        if (!ApsFrameFromObject(env, apsFrameObj, &apsFrame))
        {
            Napi::TypeError::New(env, "Invalid aps frame object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        uint8_t hops = info[1].As<Napi::Number>().Uint32Value();
        uint16_t broadcastAddr = info[2].As<Napi::Number>().Uint32Value();
        uint16_t alias = info[3].As<Napi::Number>().Uint32Value();
        uint8_t nwkSequence = info[4].As<Napi::Number>().Uint32Value();
        uint16_t messageTag = info[5].As<Napi::Number>().Uint32Value();
        Napi::Buffer<uint8_t> messageBuffer = info[6].As<Napi::Buffer<uint8_t>>();

        uint8_t sequence = 0;
        sl_status_t status = sl_zigbee_ezsp_send_multicast(&apsFrame, hops, broadcastAddr, alias, nwkSequence, messageTag, messageBuffer.Length(),
                                                           messageBuffer.Data(), &sequence);

        Napi::Array result = Napi::Array::New(env, 2);
        result[0u] = Napi::Number::New(env, status);
        result[1u] = Napi::Number::New(env, sequence);

        return result;
    }

    Napi::Value SendBroadcast(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 7 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsNumber() || !info[3].IsObject() || !info[4].IsNumber() ||
            !info[5].IsNumber() || !info[6].IsBuffer())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        sl_802154_short_addr_t alias = info[0].As<Napi::Number>().Uint32Value();
        sl_802154_short_addr_t destination = info[1].As<Napi::Number>().Uint32Value();
        uint8_t nwkSequence = info[2].As<Napi::Number>().Uint32Value();

        Napi::Object apsFrameObj = info[3].As<Napi::Object>();

        sl_zigbee_aps_frame_t apsFrame = {0};
        if (!ApsFrameFromObject(env, apsFrameObj, &apsFrame))
        {
            Napi::TypeError::New(env, "Invalid aps frame object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        uint8_t radius = info[4].As<Napi::Number>().Uint32Value();
        uint16_t messageTag = info[5].As<Napi::Number>().Uint32Value();
        Napi::Buffer<uint8_t> messageBuffer = info[6].As<Napi::Buffer<uint8_t>>();

        uint8_t sequence = 0;
        sl_status_t status = sl_zigbee_ezsp_send_broadcast(alias, destination, nwkSequence, &apsFrame, radius, messageTag, messageBuffer.Length(),
                                                           messageBuffer.Data(), &sequence);

        Napi::Array result = Napi::Array::New(env, 2);
        result[0u] = Napi::Number::New(env, status);
        result[1u] = Napi::Number::New(env, sequence);

        return result;
    }

    Napi::Value SendRawMessage(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 3 || !info[0].IsBuffer() || !info[1].IsNumber() || !info[2].IsBoolean())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Buffer<uint8_t> messageBuffer = info[0].As<Napi::Buffer<uint8_t>>();
        uint8_t priority = info[1].As<Napi::Number>().Uint32Value();
        bool useCca = info[2].As<Napi::Boolean>().Value();

        sl_status_t status = sl_zigbee_ezsp_send_raw_message(messageBuffer.Length(), messageBuffer.Data(), priority, useCca);

        return Napi::Number::New(env, status);
    }

    // Radio/Hardware Commands

    Napi::Value SetRadioPower(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsNumber())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        int8_t power = info[0].As<Napi::Number>().Int32Value(); // actually int8 (-128..127)

        sl_status_t status = sl_zigbee_ezsp_set_radio_power(power);

        return Napi::Number::New(env, status);
    }

    Napi::Value SetRadioIeee802154CcaMode(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsNumber())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        uint8_t ccaMode = info[0].As<Napi::Number>().Uint32Value();

        sl_status_t status = sl_zigbee_ezsp_set_radio_ieee802154_cca_mode(ccaMode);

        return Napi::Number::New(env, status);
    }

    Napi::Value SetLogicalAndRadioChannel(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsNumber())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        uint8_t channel = info[0].As<Napi::Number>().Uint32Value();

        sl_status_t status = sl_zigbee_ezsp_set_logical_and_radio_channel(channel);

        return Napi::Number::New(env, status);
    }

    Napi::Value SetManufacturerCode(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsNumber())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        uint16_t code = info[0].As<Napi::Number>().Uint32Value();

        sl_status_t status = sl_zigbee_ezsp_set_manufacturer_code(code);

        return Napi::Number::New(env, status);
    }

    // Routing/Table Commands

    Napi::Value SetConcentrator(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 7 || !info[0].IsBoolean() || !info[1].IsNumber() || !info[2].IsNumber() || !info[3].IsNumber() || !info[4].IsNumber() ||
            !info[5].IsNumber() || !info[6].IsNumber())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        bool on = info[0].As<Napi::Boolean>().Value();
        uint16_t concentratorType = info[1].As<Napi::Number>().Uint32Value();
        uint16_t minTime = info[2].As<Napi::Number>().Uint32Value();
        uint16_t maxTime = info[3].As<Napi::Number>().Uint32Value();
        uint8_t routeErrorThreshold = info[4].As<Napi::Number>().Uint32Value();
        uint8_t deliveryFailureThreshold = info[5].As<Napi::Number>().Uint32Value();
        uint8_t maxHops = info[6].As<Napi::Number>().Uint32Value();

        sl_status_t status =
            sl_zigbee_ezsp_set_concentrator(on, concentratorType, minTime, maxTime, routeErrorThreshold, deliveryFailureThreshold, maxHops);

        return Napi::Number::New(env, status);
    }

    Napi::Value SetSourceRouteDiscoveryMode(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsNumber())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        uint8_t mode = info[0].As<Napi::Number>().Uint32Value();

        uint32_t remainingTime = sl_zigbee_ezsp_set_source_route_discovery_mode(mode);

        return Napi::Number::New(env, remainingTime);
    }

    Napi::Value SetMulticastTableEntry(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsObject())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        uint8_t index = info[0].As<Napi::Number>().Uint32Value();
        Napi::Object entryObj = info[1].As<Napi::Object>();

        if (!entryObj.Has("multicastId") || !entryObj.Has("endpoint") || !entryObj.Has("networkIndex"))
        {
            Napi::TypeError::New(env, "Invalid multicast table entry object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        sl_zigbee_multicast_table_entry_t entry = {0};
        entry.multicastId = entryObj.Get("multicastId").As<Napi::Number>().Uint32Value();
        entry.endpoint = entryObj.Get("endpoint").As<Napi::Number>().Uint32Value();
        entry.networkIndex = entryObj.Get("networkIndex").As<Napi::Number>().Uint32Value();

        sl_status_t status = sl_zigbee_ezsp_set_multicast_table_entry(index, &entry);

        return Napi::Number::New(env, status);
    }

    Napi::Value AddEndpoint(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 6 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsNumber() || !info[3].IsNumber() || !info[4].IsArray() ||
            !info[5].IsArray())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        uint8_t endpoint = info[0].As<Napi::Number>().Uint32Value();
        uint16_t profileId = info[1].As<Napi::Number>().Uint32Value();
        uint16_t deviceId = info[2].As<Napi::Number>().Uint32Value();
        uint8_t appFlags = info[3].As<Napi::Number>().Uint32Value();

        // Accept both TypedArray and regular Array for cluster lists
        uint16_t inputClusters[255] = {0};
        uint16_t outputClusters[255] = {0};
        uint8_t inputClusterCount = 0;
        uint8_t outputClusterCount = 0;

        if (info[4].IsTypedArray())
        {
            Napi::Uint16Array inputArray = info[4].As<Napi::Uint16Array>();
            inputClusterCount = inputArray.ElementLength();
            memcpy(inputClusters, inputArray.Data(), inputClusterCount * sizeof(uint16_t));
        }
        else if (info[4].IsArray())
        {
            Napi::Array inputArray = info[4].As<Napi::Array>();
            inputClusterCount = inputArray.Length();
            for (uint32_t i = 0; i < inputClusterCount; i++)
            {
                inputClusters[i] = inputArray.Get(i).As<Napi::Number>().Uint32Value();
            }
        }

        if (info[5].IsTypedArray())
        {
            Napi::Uint16Array outputArray = info[5].As<Napi::Uint16Array>();
            outputClusterCount = outputArray.ElementLength();
            memcpy(outputClusters, outputArray.Data(), outputClusterCount * sizeof(uint16_t));
        }
        else if (info[5].IsArray())
        {
            Napi::Array outputArray = info[5].As<Napi::Array>();
            outputClusterCount = outputArray.Length();
            for (uint32_t i = 0; i < outputClusterCount; i++)
            {
                outputClusters[i] = outputArray.Get(i).As<Napi::Number>().Uint32Value();
            }
        }

        sl_status_t status = sl_zigbee_ezsp_add_endpoint(endpoint, profileId, deviceId, appFlags, inputClusterCount, outputClusterCount,
                                                         inputClusters, outputClusters);

        return Napi::Number::New(env, status);
    }

    // Monitoring

    Napi::Value ReadAndClearCounters(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        uint16_t values[SL_ZIGBEE_COUNTER_TYPE_COUNT] = {0};

        sl_zigbee_ezsp_read_and_clear_counters(values);

        Napi::Array result = Napi::Array::New(env, SL_ZIGBEE_COUNTER_TYPE_COUNT);

        for (int i = 0; i < SL_ZIGBEE_COUNTER_TYPE_COUNT; i++)
        {
            result[i] = Napi::Number::New(env, values[i]);
        }

        return result;
    }

    // Convenience Wrappers

    Napi::Value SetNWKFrameCounter(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsNumber())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        uint32_t frameCounter = info[0].As<Napi::Number>().Uint32Value();
        uint8_t value[4];
        value[0] = frameCounter & 0xFF;
        value[1] = (frameCounter >> 8) & 0xFF;
        value[2] = (frameCounter >> 16) & 0xFF;
        value[3] = (frameCounter >> 24) & 0xFF;

        sl_status_t status = sl_zigbee_ezsp_set_value(SL_ZIGBEE_EZSP_VALUE_NWK_FRAME_COUNTER, 4, value);

        return Napi::Number::New(env, status);
    }

    Napi::Value SetAPSFrameCounter(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsNumber())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        uint32_t frameCounter = info[0].As<Napi::Number>().Uint32Value();
        uint8_t value[4];
        value[0] = frameCounter & 0xFF;
        value[1] = (frameCounter >> 8) & 0xFF;
        value[2] = (frameCounter >> 16) & 0xFF;
        value[3] = (frameCounter >> 24) & 0xFF;

        sl_status_t status = sl_zigbee_ezsp_set_value(SL_ZIGBEE_EZSP_VALUE_APS_FRAME_COUNTER, 4, value);

        return Napi::Number::New(env, status);
    }

    Napi::Value StartWritingStackTokens(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        sl_status_t status = sl_zigbee_start_writing_stack_tokens();

        return Napi::Number::New(env, status);
    }

    Napi::Value SetExtendedSecurityBitmask(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsNumber())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        uint16_t bitmask = info[0].As<Napi::Number>().Uint32Value();

        sl_status_t status = sl_zigbee_set_extended_security_bitmask(bitmask);

        return Napi::Number::New(env, status);
    }

    Napi::Value GetEndpointFlags(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsNumber())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        uint8_t endpoint = info[0].As<Napi::Number>().Uint32Value();

        sl_zigbee_ezsp_endpoint_flags_t returnFlags;
        sl_status_t status = sl_zigbee_ezsp_get_endpoint_flags(endpoint, &returnFlags);

        Napi::Array result = Napi::Array::New(env, 2);
        result[0u] = Napi::Number::New(env, status);

        if (status == SL_STATUS_OK)
        {
            result[1u] = returnFlags;
        }

        return result;
    }

    Napi::Value GetVersionStruct(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        sl_zigbee_version_t version;

        sl_status_t status = sl_zigbee_ezsp_get_version_struct(&version);

        Napi::Array result = Napi::Array::New(env, 2);
        result[0u] = Napi::Number::New(env, status);

        if (status == SL_STATUS_OK)
        {
            Napi::Object response = Napi::Object::New(env);
            response.Set("build", Napi::Number::New(env, version.build));
            response.Set("major", Napi::Number::New(env, version.major));
            response.Set("minor", Napi::Number::New(env, version.minor));
            response.Set("patch", Napi::Number::New(env, version.patch));
            response.Set("special", Napi::Number::New(env, version.special));
            response.Set("type", Napi::Number::New(env, version.type));

            result[1u] = response;
        }

        return result;
    }

    // sli_zigbee_af_send
    Napi::Value Send(const Napi::CallbackInfo &info)
    {
        Napi::Env env = info.Env();

        if (info.Length() < 6 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsObject() || !info[3].IsBuffer() || !info[4].IsNumber() ||
            !info[5].IsNumber())
        {
            Napi::TypeError::New(env, "Invalid arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        sl_zigbee_outgoing_message_type_t type = info[0].As<Napi::Number>().Uint32Value();
        uint16_t indexOrDestination = info[1].As<Napi::Number>().Uint32Value();
        Napi::Object apsFrameObj = info[2].As<Napi::Object>();

        sl_zigbee_aps_frame_t apsFrame = {0};
        if (!ApsFrameFromObject(env, apsFrameObj, &apsFrame))
        {
            Napi::TypeError::New(env, "Invalid aps frame object").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        Napi::Buffer<uint8_t> messageBuffer = info[3].As<Napi::Buffer<uint8_t>>();

        sl_status_t status = SL_STATUS_OK;
        uint16_t messageTag = ezspNextSequence();
        uint8_t nwkRadius = 12; // ZA_MAX_HOPS
        sl_802154_short_addr_t nwkAlias = SL_ZIGBEE_NULL_NODE_ID;

        switch (type)
        {
        case SL_ZIGBEE_OUTGOING_VIA_BINDING:
        case SL_ZIGBEE_OUTGOING_VIA_ADDRESS_TABLE:
        case SL_ZIGBEE_OUTGOING_DIRECT:
        {
            status = sl_zigbee_ezsp_send_unicast(type, indexOrDestination, &apsFrame, messageTag, messageBuffer.Length(), messageBuffer.Data(),
                                                 &apsFrame.sequence);

            // mutate Node.js object
            apsFrameObj.Set("sequence", apsFrame.sequence);

            break;
        }
        case SL_ZIGBEE_OUTGOING_MULTICAST:
        case SL_ZIGBEE_OUTGOING_MULTICAST_WITH_ALIAS:
        {
            uint8_t sequence = info[5].As<Napi::Number>().Uint32Value();

            if (type == SL_ZIGBEE_OUTGOING_MULTICAST_WITH_ALIAS ||
                (apsFrame.sourceEndpoint == SL_ZIGBEE_GP_ENDPOINT && apsFrame.destinationEndpoint == SL_ZIGBEE_GP_ENDPOINT &&
                 apsFrame.options & SL_ZIGBEE_APS_OPTION_USE_ALIAS_SEQUENCE_NUMBER))
            {
                uint16_t alias = info[4].As<Napi::Number>().Uint32Value();

                nwkRadius = apsFrame.radius;
                nwkAlias = alias;
            }

            status = sl_zigbee_ezsp_send_multicast(&apsFrame, nwkRadius, 0, nwkAlias, sequence, messageTag, messageBuffer.Length(),
                                                   messageBuffer.Data(), &apsFrame.sequence);

            // mutate Node.js object
            apsFrameObj.Set("sequence", apsFrame.sequence);

            break;
        }
        case SL_ZIGBEE_OUTGOING_BROADCAST:
        case SL_ZIGBEE_OUTGOING_BROADCAST_WITH_ALIAS:
        {
            uint8_t sequence = info[5].As<Napi::Number>().Uint32Value();

            if (type == SL_ZIGBEE_OUTGOING_BROADCAST_WITH_ALIAS ||
                (apsFrame.sourceEndpoint == SL_ZIGBEE_GP_ENDPOINT && apsFrame.destinationEndpoint == SL_ZIGBEE_GP_ENDPOINT &&
                 apsFrame.options & SL_ZIGBEE_APS_OPTION_USE_ALIAS_SEQUENCE_NUMBER))
            {
                uint16_t alias = info[4].As<Napi::Number>().Uint32Value();

                nwkRadius = apsFrame.radius;
                nwkAlias = alias;
            }

            status = sl_zigbee_ezsp_send_broadcast(nwkAlias, indexOrDestination, sequence, &apsFrame, nwkRadius, messageTag, messageBuffer.Length(),
                                                   messageBuffer.Data(), &apsFrame.sequence);

            // mutate Node.js object
            apsFrameObj.Set("sequence", apsFrame.sequence);

            break;
        }
        default:
            status = SL_STATUS_INVALID_PARAMETER;
        }

        Napi::Array result = Napi::Array::New(env, 3);
        result[0u] = Napi::Number::New(env, status);
        result[1u] = Napi::Number::New(env, messageTag);

        return result;
    }

    // #endregion EZSP Command Bindings

} // namespace EzspNapi

// Module initialization
Napi::Object InitAll(Napi::Env env, Napi::Object exports)
{
    exports.Set("init", Napi::Function::New(env, EzspNapi::Init)); // ctor equivalent
    exports.Set("start", Napi::Function::New(env, EzspNapi::Start));
    exports.Set("stop", Napi::Function::New(env, EzspNapi::Stop));

    // Base
    exports.Set("ezspVersion", Napi::Function::New(env, EzspNapi::Version));
    exports.Set("ezspGetEui64", Napi::Function::New(env, EzspNapi::GetEui64));

    // Network management
    exports.Set("ezspGetNetworkParameters", Napi::Function::New(env, EzspNapi::GetNetworkParameters));
    exports.Set("ezspNetworkInit", Napi::Function::New(env, EzspNapi::NetworkInit));
    exports.Set("ezspNetworkState", Napi::Function::New(env, EzspNapi::NetworkState));
    exports.Set("ezspFormNetwork", Napi::Function::New(env, EzspNapi::FormNetwork));
    exports.Set("ezspLeaveNetwork", Napi::Function::New(env, EzspNapi::LeaveNetwork));
    exports.Set("ezspPermitJoining", Napi::Function::New(env, EzspNapi::PermitJoining));

    // Configuration
    exports.Set("ezspGetConfigurationValue", Napi::Function::New(env, EzspNapi::GetConfigurationValue));
    exports.Set("ezspSetConfigurationValue", Napi::Function::New(env, EzspNapi::SetConfigurationValue));
    exports.Set("ezspGetValue", Napi::Function::New(env, EzspNapi::GetValue));
    exports.Set("ezspSetValue", Napi::Function::New(env, EzspNapi::SetValue));
    exports.Set("ezspGetExtendedValue", Napi::Function::New(env, EzspNapi::GetExtendedValue));
    exports.Set("ezspSetPolicy", Napi::Function::New(env, EzspNapi::SetPolicy));
    exports.Set("ezspTokenFactoryReset", Napi::Function::New(env, EzspNapi::TokenFactoryReset));

    // Security
    exports.Set("ezspSetInitialSecurityState", Napi::Function::New(env, EzspNapi::SetInitialSecurityState));
    exports.Set("ezspGetNetworkKeyInfo", Napi::Function::New(env, EzspNapi::GetNetworkKeyInfo));
    exports.Set("ezspGetApsKeyInfo", Napi::Function::New(env, EzspNapi::GetApsKeyInfo));
    exports.Set("ezspExportKey", Napi::Function::New(env, EzspNapi::ExportKey));
    exports.Set("ezspExportLinkKeyByIndex", Napi::Function::New(env, EzspNapi::ExportLinkKeyByIndex));
    exports.Set("ezspImportLinkKey", Napi::Function::New(env, EzspNapi::ImportLinkKey));
    exports.Set("ezspImportTransientKey", Napi::Function::New(env, EzspNapi::ImportTransientKey));
    exports.Set("ezspEraseKeyTableEntry", Napi::Function::New(env, EzspNapi::EraseKeyTableEntry));
    exports.Set("ezspClearKeyTable", Napi::Function::New(env, EzspNapi::ClearKeyTable));
    exports.Set("ezspClearTransientLinkKeys", Napi::Function::New(env, EzspNapi::ClearTransientLinkKeys));
    exports.Set("ezspBroadcastNextNetworkKey", Napi::Function::New(env, EzspNapi::BroadcastNextNetworkKey));
    exports.Set("ezspBroadcastNetworkKeySwitch", Napi::Function::New(env, EzspNapi::BroadcastNetworkKeySwitch));

    // Messaging
    exports.Set("ezspSendUnicast", Napi::Function::New(env, EzspNapi::SendUnicast));
    exports.Set("ezspSendMulticast", Napi::Function::New(env, EzspNapi::SendMulticast));
    exports.Set("ezspSendBroadcast", Napi::Function::New(env, EzspNapi::SendBroadcast));
    exports.Set("ezspSendRawMessage", Napi::Function::New(env, EzspNapi::SendRawMessage));

    // Radio/hardware
    exports.Set("ezspSetRadioPower", Napi::Function::New(env, EzspNapi::SetRadioPower));
    exports.Set("ezspSetRadioIeee802154CcaMode", Napi::Function::New(env, EzspNapi::SetRadioIeee802154CcaMode));
    exports.Set("ezspSetLogicalAndRadioChannel", Napi::Function::New(env, EzspNapi::SetLogicalAndRadioChannel));
    exports.Set("ezspSetManufacturerCode", Napi::Function::New(env, EzspNapi::SetManufacturerCode));

    // Routing/tables
    exports.Set("ezspSetConcentrator", Napi::Function::New(env, EzspNapi::SetConcentrator));
    exports.Set("ezspSetSourceRouteDiscoveryMode", Napi::Function::New(env, EzspNapi::SetSourceRouteDiscoveryMode));
    exports.Set("ezspSetMulticastTableEntry", Napi::Function::New(env, EzspNapi::SetMulticastTableEntry));
    exports.Set("ezspAddEndpoint", Napi::Function::New(env, EzspNapi::AddEndpoint));

    // Monitoring
    exports.Set("ezspReadAndClearCounters", Napi::Function::New(env, EzspNapi::ReadAndClearCounters));

    // Convenience wrappers
    exports.Set("ezspSetNWKFrameCounter", Napi::Function::New(env, EzspNapi::SetNWKFrameCounter));
    exports.Set("ezspSetAPSFrameCounter", Napi::Function::New(env, EzspNapi::SetAPSFrameCounter));
    exports.Set("ezspStartWritingStackTokens", Napi::Function::New(env, EzspNapi::StartWritingStackTokens));
    exports.Set("ezspSetExtendedSecurityBitmask", Napi::Function::New(env, EzspNapi::SetExtendedSecurityBitmask));
    exports.Set("ezspGetEndpointFlags", Napi::Function::New(env, EzspNapi::GetEndpointFlags));
    exports.Set("ezspGetVersionStruct", Napi::Function::New(env, EzspNapi::GetVersionStruct));
    exports.Set("send", Napi::Function::New(env, EzspNapi::Send));

    return exports;
}

NODE_API_MODULE(ezsp_ash_posix, InitAll)
