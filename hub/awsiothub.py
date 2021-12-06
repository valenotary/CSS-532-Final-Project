# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0.
import asyncio
from concurrent.futures import Future
from bleak import BleakScanner, BleakClient
from bleak.exc import BleakError

import argparse
from awscrt import io, mqtt, auth, http
from awsiot import mqtt_connection_builder
import sys
import threading
import time
from uuid import uuid4
import json

from pathlib import Path
# Modified version of pubsub from the samples of the AWS Python IOT
# This should maintain a connection to the AWS IOT Core as a thing, listening to messages from "sdgc/hub/incoming"
# And also maintain a BLE connection to the LED
# The callback should handle writing the BLE characteristic to the LED

# GLOBALS
my_endpoint = "a21pvb772khs3n-ats.iot.us-west-2.amazonaws.com"
my_cert_filepath = "./hub/certs/certificate.pem.crt"
my_key_filepath = "./hub/certs/private.pem.key"
my_root_ca_filepath = "./hub/certs/AmazonRootCA1.pem"
my_client_id = "SDGC-Hub"
my_subscribed_topic = "SDGC/hub/incoming"
my_publishing_topic = "SDGC/hub/outgoing"

gesture_to_action = {"SwipeUp": "Turn On", "SwipeDown": "Turn Off"}
action_to_byte = {"Turn On": 0x01, "Turn Off": 0x00}

# global stuff for the mqtt client
event_loop_group = io.EventLoopGroup(1)
host_resolver = io.DefaultHostResolver(event_loop_group)
client_bootstrap = io.ClientBootstrap(event_loop_group, host_resolver)

proxy_options = None

global_future_name = None
global_ready = False

# BLE Stuff
BLE_LED_switch_characteristic = "19B10001-E8F2-537E-4F6C-D104768A1214"


def notification_handler(sender, data):
    """Simple notification handler which prints the data received."""
    print("{0}: {1}".format(sender, data))

# Callback when connection is accidentally lost.


def on_connection_interrupted(connection, error, **kwargs):
    print("Connection interrupted. error: {}".format(error))

# Callback when an interrupted connection is re-established.


def on_connection_resumed(connection, return_code, session_present, **kwargs):
    print("Connection resumed. return_code: {} session_present: {}".format(
        return_code, session_present))

    if return_code == mqtt.ConnectReturnCode.ACCEPTED and not session_present:
        print("Session did not persist. Resubscribing to existing topics...")
        resubscribe_future, _ = connection.resubscribe_existing_topics()

        # Cannot synchronously wait for resubscribe result because we're on the connection's event-loop thread,
        # evaluate result with a callback instead.
        resubscribe_future.add_done_callback(on_resubscribe_complete)


def on_resubscribe_complete(resubscribe_future):
    resubscribe_results = resubscribe_future.result()
    print("Resubscribe results: {}".format(resubscribe_results))

    for topic, qos in resubscribe_results['topics']:
        if qos is None:
            sys.exit("Server rejected resubscribe to topic: {}".format(topic))

# callback subscribed to


def on_message_received(topic, payload, dup, qos, retain, **kwargs):
    print("Received message from topic '{}': {}".format(topic, payload))
    message_dict = json.loads(payload)
    if (not global_ready):
        return
    if ("gesture" in message_dict):
        global_future_name.set_result(
            (gesture_to_action[message_dict["gesture"]], action_to_byte[gesture_to_action[message_dict["gesture"]]]))
    if ("action" in message_dict and message_dict["action"] == "connecting"):
        print("Sending Alive MQTT message to AWS IoT...")
        message_json = json.dumps(
            {"action": "connecting", "clientID": my_client_id})
        print("Publishing message to topic '{}': {}".format(
            my_publishing_topic, message_json))
        mqtt_connection_global.publish(
            topic=my_publishing_topic,
            payload=message_json,
            qos=mqtt.QoS.AT_LEAST_ONCE)


mqtt_connection_global = mqtt_connection_builder.mtls_from_path(
    endpoint=my_endpoint,
    port=8883,  # should default to 8883 im hoping ?
    cert_filepath=my_cert_filepath,
    pri_key_filepath=my_key_filepath,
    client_bootstrap=client_bootstrap,
    ca_filepath=my_root_ca_filepath,
    on_connection_interrupted=on_connection_interrupted,
    on_connection_resumed=on_connection_resumed,
    client_id=my_client_id,
    clean_session=False,
    keep_alive_secs=30,
    http_proxy_options=proxy_options)


async def main():
    print("Connecting to {} with client ID '{}'...".format(
        my_endpoint, my_client_id))

    connect_future = mqtt_connection_global.connect()
    # Future.result() waits until a result is available
    connect_future.result()
    print("Connected!")

    # subscribe to callback
    print("Subscribing to topic '{}'...".format(my_subscribed_topic))
    subscribe_future, packet_id = mqtt_connection_global.subscribe(
        topic=my_subscribed_topic,
        qos=mqtt.QoS.AT_LEAST_ONCE,
        callback=on_message_received)

    subscribe_result = subscribe_future.result()
    print("Subscribed with {}".format(str(subscribe_result['qos'])))

    # basically https://github.com/hbldh/bleak/blob/develop/examples/connect_by_bledevice.py, repurposed
    print("Searching for LED device...")
    ledDevice = None
    while(not ledDevice):
        async with BleakScanner() as scanner:
            await asyncio.sleep(5.0)
        for d in scanner.discovered_devices:
            if d.name == "LED":
                print('Found LED device!')
                ledDevice = d
                break

    # assume that the device is connected at this point
    async with BleakClient(ledDevice) as client:

        # let MQTT broker know that this hub is alive; starts main loop of SDGC
        print("Sending Alive MQTT message to AWS IoT...")
        message_json = json.dumps(
            {"action": "connecting", "clientID": my_client_id})
        print("Publishing message to topic '{}': {}".format(
            my_publishing_topic, message_json))
        mqtt_connection_global.publish(
            topic=my_publishing_topic,
            payload=message_json,
            qos=mqtt.QoS.AT_LEAST_ONCE)
        global global_ready
        global_ready = True
        print("starting while loop...")
        while (client.is_connected):
            # 1. wait for callback to be called, with bluetooth value
            # 2. send byte value to ble
            global global_future_name
            global_future_name = Future()
            res = global_future_name.result()
            print("Received action: {}".format(res[0]))
            print("byte representation: {}".format(bytearray([res[1]])))
            await client.write_gatt_char(
                BLE_LED_switch_characteristic, bytearray([res[1]]), True)
            print("gatt characteristic written to. sending notification to device...")
            message_json = json.dumps(
                {"action": "notify",
                 "fromClientID": my_client_id,
                 "toClientID": "SDGC-Arduino",
                 "message": "actionCompleted"})
            print("Publishing message to topic '{}': {}".format(
                my_publishing_topic, message_json))
            mqtt_connection_global.publish(
                topic=my_publishing_topic,
                payload=message_json,
                qos=mqtt.QoS.AT_LEAST_ONCE)
            # await client.disconnect()

    # ... after everything...
    # Disconnect
    print("Disconnecting...")
    disconnect_future = mqtt_connection_global.disconnect()
    disconnect_future.result()
    print("Disconnected!")

# ACTUAL start of the program
if __name__ == "__main__":
    asyncio.run(main())
