import fs from "node:fs";
import path from "node:path";

function backupFilePath(filePath: string): string {
    return `${filePath}.original`;
}

function patch(key: string, filePath: string, original: string, patched: string) {
    const backupFile = backupFilePath(filePath);

    if (!fs.existsSync(filePath)) {
        console.error(`Error: File not found: ${filePath}`);
        process.exit(1);
    }

    if (!fs.existsSync(backupFile)) {
        console.log(`Creating backup of '${key}'`);
        fs.copyFileSync(filePath, backupFile);
    }

    let content = fs.readFileSync(filePath, "utf8");

    if (content.includes(patched)) {
        console.log("✓ Already patched");
        return;
    }

    if (!content.includes(original)) {
        console.error("Error: Cannot find original in file");
        console.error("The source may have been modified or this script needs updating");
        process.exit(1);
    }

    content = content.replace(original, patched);
    fs.writeFileSync(filePath, content, "utf8");

    console.log(`✓ '${key}' patched successfully`);
}

function revert(key: string, filePath: string) {
    const backupFile = backupFilePath(filePath);

    if (!fs.existsSync(backupFile)) {
        console.log("No backup found, nothing to revert");
        return;
    }

    if (!fs.existsSync(filePath)) {
        console.error(`Error: File not found: ${filePath}`);
        process.exit(1);
    }

    console.log(`Restoring '${key}' from backup`);
    fs.copyFileSync(backupFile, filePath);
    console.log("✓ Reverted to original state");

    console.log("Removing backup file...");
    fs.unlinkSync(backupFile);
    console.log("✓ Backup removed");
}

const PATCHES: Record<string, { path: string; original: string; patched: string }> = {
    /**
     * Enable high-speed baudrates
     */
    siSdkEzspHostIoC: {
        path: path.join(import.meta.dirname, "../simplicity_sdk/protocol/zigbee/app/ezsp-host/ezsp-host-io.c"),
        original: `static const struct { int bps; speed_t posixBaud; } baudTable[] =
{ { 600, B600    },
  { 1200, B1200   },
  { 2400, B2400   },
  { 4800, B4800   },
  { 9600, B9600   },
  { 19200, B19200  },
  { 38400, B38400  },
  { 57600, B57600  },
  { 115200, B115200 },
  { 230400, B230400 } };`,
        patched: `static const struct { int bps; speed_t posixBaud; } baudTable[] =
{ { 600, B600    },
  { 1200, B1200   },
  { 2400, B2400   },
  { 4800, B4800   },
  { 9600, B9600   },
  { 19200, B19200  },
  { 38400, B38400  },
  { 57600, B57600  },
  { 115200, B115200 },
  { 230400, B230400 },
  { 460800, B460800 },
  { 921600, B921600 } };`,
    },
    /**
     * Tracing mishandles EZSP frame ID
     */
    siSdkEzspHostUiC: {
        path: path.join(import.meta.dirname, "../simplicity_sdk/protocol/zigbee/app/ezsp-host/ezsp-host-ui.c"),
        original: `    if ((ezspFrame[EZSP_EXTENDED_FRAME_CONTROL_HB_INDEX] & EZSP_EXTENDED_FRAME_FORMAT_VERSION_MASK)
        == EZSP_EXTENDED_FRAME_FORMAT_VERSION) {
      frameId |= (ezspFrame[EZSP_EXTENDED_FRAME_ID_HB_INDEX] << 8);
    }`,
        patched: `    if ((ezspFrame[EZSP_EXTENDED_FRAME_CONTROL_HB_INDEX] & EZSP_EXTENDED_FRAME_FORMAT_VERSION_MASK)
        == EZSP_EXTENDED_FRAME_FORMAT_VERSION) {
      frameId = ezspFrame[EZSP_EXTENDED_FRAME_ID_LB_INDEX] | (ezspFrame[EZSP_EXTENDED_FRAME_ID_HB_INDEX] << 8);
    }`,
    },
    /**
     * Tracing mishandles EZSP frame ID
     */
    siSdkSerialInterfaceUartC1: {
        path: path.join(import.meta.dirname, "../simplicity_sdk/protocol/zigbee/app/util/ezsp/serial-interface-uart.c"),
        original: `  sl_zigbee_ezsp_trace_ezsp_verbose("serialSendCommand(): ID=0x%02X Seq=0x%02X",
                                    ezspFrameContents[EZSP_FRAME_ID_INDEX],
                                    ezspFrameContents[EZSP_SEQUENCE_INDEX]);`,
        patched: `  sl_zigbee_ezsp_trace_ezsp_verbose("serialSendCommand(): ID=0x%02X Seq=0x%02X",
                                    (ezspFrameContents[EZSP_EXTENDED_FRAME_CONTROL_HB_INDEX] & EZSP_EXTENDED_FRAME_FORMAT_VERSION_MASK) == EZSP_EXTENDED_FRAME_FORMAT_VERSION ? ezspFrameContents[EZSP_EXTENDED_FRAME_ID_LB_INDEX] | (ezspFrameContents[EZSP_EXTENDED_FRAME_ID_HB_INDEX] << 8) : ezspFrameContents[EZSP_FRAME_ID_INDEX],
                                    ezspFrameContents[EZSP_SEQUENCE_INDEX]);`,
    },
    /**
     * Tracing mishandles EZSP frame ID
     */
    siSdkSerialInterfaceUartC2: {
        path: path.join(import.meta.dirname, "../simplicity_sdk/protocol/zigbee/app/util/ezsp/serial-interface-uart.c"),
        original: `sl_zigbee_ezsp_trace_ezsp_verbose("serialResponseReceived(): ID=0x%02X Seq=0x%02X Buffer=%u",
                                        buffer->data[EZSP_FRAME_ID_INDEX],
                                        buffer->data[EZSP_SEQUENCE_INDEX],
                                        buffer);`,
        patched: `sl_zigbee_ezsp_trace_ezsp_verbose("serialResponseReceived(): ID=0x%02X Seq=0x%02X Buffer=%u",
                                        (buffer->data[EZSP_EXTENDED_FRAME_CONTROL_HB_INDEX] & EZSP_EXTENDED_FRAME_FORMAT_VERSION_MASK) == EZSP_EXTENDED_FRAME_FORMAT_VERSION ? buffer->data[EZSP_EXTENDED_FRAME_ID_LB_INDEX] | (buffer->data[EZSP_EXTENDED_FRAME_ID_HB_INDEX] << 8) : buffer->data[EZSP_FRAME_ID_INDEX],
                                        buffer->data[EZSP_SEQUENCE_INDEX],
                                        buffer);`,
    },
    /**
     * This one doesn't make much sense...
     * Tries to restore connection only to drop it again with `ncpNeedsResetAndInit` in `sl_zigbee_ezsp_error_handler(SL_ZIGBEE_EZSP_NOT_CONNECTED)`
     */
    siSdkSerialInterfaceUartC3: {
        path: path.join(import.meta.dirname, "../simplicity_sdk/protocol/zigbee/app/util/ezsp/serial-interface-uart.c"),
        original: `if (!connected) {
    // Attempt to restore the connection. This will reset the EM260.
    sl_zigbee_ezsp_close();
    sl_zigbee_ezsp_init();
  }`,
        patched: `//if (!connected) {
//    // Attempt to restore the connection. This will reset the EM260.
//    sl_zigbee_ezsp_close();
//    sl_zigbee_ezsp_init();
//  }`,
    },
    siSdkAshHostC: {
        path: path.join(import.meta.dirname, "../simplicity_sdk/protocol/zigbee/app/ezsp-host/ash/ash-host.c"),
        original: `        sl_zigbee_ezsp_trace_ezsp_verbose("ashReceiveFrame(): ID=0x%02X Seq=0x%02X Buffer=%u",
                                          rxDataBuffer->data[EZSP_FRAME_ID_INDEX],
                                          rxDataBuffer->data[EZSP_SEQUENCE_INDEX],
                                          rxDataBuffer);`,
        patched: `        sl_zigbee_ezsp_trace_ezsp_verbose("ashReceiveFrame(): ID=0x%02X Seq=0x%02X Buffer=%u",
                                          (rxDataBuffer->data[EZSP_EXTENDED_FRAME_CONTROL_HB_INDEX] & EZSP_EXTENDED_FRAME_FORMAT_VERSION_MASK) == EZSP_EXTENDED_FRAME_FORMAT_VERSION ? rxDataBuffer->data[EZSP_EXTENDED_FRAME_ID_LB_INDEX] | (rxDataBuffer->data[EZSP_EXTENDED_FRAME_ID_HB_INDEX] << 8) : rxDataBuffer->data[EZSP_FRAME_ID_INDEX],
                                          rxDataBuffer->data[EZSP_SEQUENCE_INDEX],
                                          rxDataBuffer);`,
    },
};

const command = process.argv[2];

switch (command) {
    case "patch":
        for (const key in PATCHES) {
            const patchI = PATCHES[key];

            patch(key, patchI.path, patchI.original, patchI.patched);
        }
        break;
    case "revert":
        for (const key in PATCHES) {
            const patchI = PATCHES[key];

            revert(key, patchI.path);
        }
        break;
    default:
        console.log("Usage: apply_patches.ts [patch|revert]");
        console.log("  patch   - Apply patch");
        console.log("  revert - Revert original file");
        process.exit(1);
}
