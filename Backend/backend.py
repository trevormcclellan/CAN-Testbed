from flask import Flask, request, jsonify
from flask_cors import CORS
import can
import time
import threading
import subprocess

app = Flask(__name__)
CORS(app)  # Enable CORS for all routes and origins

can_messages = []

def set_transmit_queue_size(interface, size):
    """
    Set the transmit queue size for the specified CAN interface.
    """
    try:
        subprocess.run(
            ["sudo", "ip", "link", "set", interface, "txqueuelen", str(size)],
            check=True
        )
        return True
    except subprocess.CalledProcessError as e:
        print(f"Failed to set transmit queue size: {e}")
        return False

@app.route('/load_can_messages', methods=['POST'])
def load_can_messages():
    global can_messages
    data = request.get_json()

    if not isinstance(data, list):
        return jsonify({"status": "error", "message": "Expected an array of CAN messages"}), 400

    parsed_messages = []
    for message in data:
        try:
            timestamp = message['timestamp']
            can_id = int(message['id'], 16)
            payload = bytes.fromhex(message['data'])

            if len(payload) > 8:
                return jsonify({"status": "error", "message": "Message data cannot exceed 8 bytes"}), 400

            parsed_messages.append({'timestamp': timestamp, 'id': can_id, 'data': payload})

        except (KeyError, ValueError) as e:
            return jsonify({"status": "error", "message": f"Invalid message format: {e}"}), 400

    can_messages = parsed_messages
    return jsonify({"status": "success", "message": "CAN messages loaded successfully"})

@app.route('/start_simulation', methods=['POST'])
def start_simulation():
    global can_messages

    if not can_messages:
        return jsonify({"status": "error", "message": "No CAN messages to replay"}), 400

    data = request.get_json()
    interface = data.get('interface', 'can0')
    bitrate = data.get('bitrate', 500000)
    tx_queue_size = data.get('tx_queue_size', 1000)  # Default to 1000 if not provided

    # Set the transmit queue size
    if not set_transmit_queue_size(interface, tx_queue_size):
        return jsonify({"status": "error", "message": "Failed to set transmit queue size"}), 500

    # Setup CAN interface
    try:
        subprocess.run(
            ["sudo", "ip", "link", "set", interface, "up", "type", "can", "bitrate", str(bitrate)],
            check=True
        )
    except subprocess.CalledProcessError as e:
        return jsonify({"status": "error", "message": f"Failed to set up interface {interface}: {e}"}), 500

    def transform_data(data):
        # Add code here to transform data before sending
        return data

    def replay_messages():
        try:
            bus = can.interface.Bus(channel=interface, interface="socketcan")
            start_time = time.time()

            for message in sorted(can_messages, key=lambda x: x['timestamp']):
                elapsed_time = time.time() - start_time
                time_to_wait = message['timestamp'] - elapsed_time
                
                # time_to_wait = start_time + message['timestamp'] - time.time()
                # time_to_wait = 0.01
                # print(message['timestamp'])
                # if time_to_wait <= 0:
                #     if prev_time_to_wait is not None and prev_time_to_wait <= 0:
                #         time_to_wait = 0.01
                if time_to_wait > 0:
                    time.sleep(time_to_wait)

                # transform data before sending
                message['data'] = transform_data(message['data'])

                can_message = can.Message(
                    arbitration_id=message['id'],
                    data=message['data'],
                    is_extended_id=False
                )
                try:
                    bus.send(can_message)
                    # print(f"Num Skips: {num_skips}")
                    # print(f"Sent: {can_message}")
                except can.CanError as e:
                    print(f"Failed to send message: {e}")

        except Exception as e:
            print(f"Error during simulation: {e}")
        finally:
            subprocess.run(["sudo", "ip", "link", "set", interface, "down"], check=False)

    simulation_thread = threading.Thread(target=replay_messages)
    simulation_thread.start()

    return jsonify({"status": "success", "message": "Simulation started"})

# @app.route('/start_simulation', methods=['POST'])
# def start_simulation():
#     global can_messages

#     if not can_messages:
#         return jsonify({"status": "error", "message": "No CAN messages to replay"}), 400

#     data = request.get_json()
#     interface = data.get('interface', 'can0')
#     bitrate = data.get('bitrate', 500000)
#     tx_queue_size = data.get('tx_queue_size', 1000)  # Default to 1000 if not provided

#     # Set the transmit queue size
#     if not set_transmit_queue_size(interface, tx_queue_size):
#         return jsonify({"status": "error", "message": "Failed to set transmit queue size"}), 500

#     # Setup CAN interface
#     try:
#         subprocess.run(
#             ["sudo", "ip", "link", "set", interface, "up", "type", "can", "bitrate", str(bitrate)],
#             check=True
#         )
#     except subprocess.CalledProcessError as e:
#         return jsonify({"status": "error", "message": f"Failed to set up interface {interface}: {e}"}), 500

#     def read_error_frames():
#         try:
#             bus = can.interface.Bus(channel=interface, interface="socketcan")
#             while True:
#                 message = bus.recv()  # Receive CAN message (blocks)
                
#                 if message is not None:
#                     if isinstance(message, can.ErrorFrame):
#                         print(f"Received Error Frame: {message}")
#                         # Handle or log the error frame here
#                     else:
#                         # Handle normal CAN messages if necessary
#                         pass
#         except Exception as e:
#             print(f"Error while reading error frames: {e}")

#     def replay_messages():
#         try:
#             bus = can.interface.Bus(channel=interface, interface="socketcan")
#             start_time = time.time()
#             num_skips = 0

#             for message in sorted(can_messages, key=lambda x: x['timestamp']):
#                 elapsed_time = time.time() - start_time
#                 time_to_wait = message['timestamp'] - elapsed_time
#                 if time_to_wait > 0:
#                     time.sleep(time_to_wait)
#                     num_skips = 0
#                 else:
#                     num_skips += 1

#                 can_message = can.Message(
#                     arbitration_id=message['id'],
#                     data=message['data'],
#                     is_extended_id=False
#                 )
#                 try:
#                     bus.send(can_message)
#                 except can.CanError as e:
#                     print(f"Failed to send message: {e}")

#         except Exception as e:
#             print(f"Error during simulation: {e}")
#         finally:
#             subprocess.run(["sudo", "ip", "link", "set", interface, "down"], check=False)

#     # Start reading error frames in a separate thread
#     error_thread = threading.Thread(target=read_error_frames)
#     error_thread.daemon = True  # Ensures the thread will close when the program exits
#     error_thread.start()

#     # Start message replay simulation
#     simulation_thread = threading.Thread(target=replay_messages)
#     simulation_thread.start()

#     return jsonify({"status": "success", "message": "Simulation started"})


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
