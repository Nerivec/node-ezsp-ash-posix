#!/bin/sh

# Use this if you already have the SDK cloned locally somewhere, adjust the relative paths below

mkdir -p simplicity_sdk/protocol/zigbee/ \
    simplicity_sdk/platform/service/ \
    simplicity_sdk/platform/common/ \
    simplicity_sdk/platform/radio/ \
    simplicity_sdk/util/silicon_labs/

cp -R ../../simplicity_sdk/protocol/zigbee/app ./simplicity_sdk/protocol/zigbee/
cp -R ../../simplicity_sdk/protocol/zigbee/stack ./simplicity_sdk/protocol/zigbee/
cp -R ../../simplicity_sdk/platform/service/legacy_common_ash ./simplicity_sdk/platform/service/
cp -R ../../simplicity_sdk/platform/service/legacy_hal ./simplicity_sdk/platform/service/
cp -R ../../simplicity_sdk/platform/common/inc ./simplicity_sdk/platform/common/
cp -R ../../simplicity_sdk/platform/radio/mac ./simplicity_sdk/platform/radio/
cp -R ../../simplicity_sdk/util/silicon_labs/silabs_core ./simplicity_sdk/util/silicon_labs/
