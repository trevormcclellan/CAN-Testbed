import serial
import threading
import atexit
import time
from flask import Flask, render_template, jsonify, request

app = Flask(__name__)
ser = None
initialized = False

def initialize_serial():
    global ser, initialized
    try:
        # Initialize serial communication
        ser = serial.Serial('COM4', 115200, timeout=1)
        initialized = True
        
        # Start serial reader thread
        thread = threading.Thread(target=serial_reader)
        thread.daemon = True
        thread.start()

        # Register cleanup function
        atexit.register(close_serial_port)
        
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")

def close_serial_port():
    global ser
    if ser and ser.is_open:
        ser.close()
        print("Serial port closed")

def serial_reader():
    global ser
    while True:
        if ser and ser.is_open:
            try:
                data = ser.readline().decode().strip()
                if data:
                    print(f"Received: {data}")
                    # Process received data as needed
            except serial.SerialException as e:
                print(f"Serial error: {e}")
        else:
            print("Serial port not open")
        time.sleep(0.1)  # Adjust sleep time as needed

@app.before_request
def before_request():
    global initialized
    if not initialized:
        initialize_serial()

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/send_command', methods=['POST'])
def send_command():
    global ser
    try:
        command = request.json.get('command', '')
        if command and ser and ser.is_open:
            ser.write((command + '\n').encode('utf-8'))
            return jsonify({'status': 'success', 'message': f'Sent command: {command}'})
        else:
            return jsonify({'status': 'error', 'message': 'Serial port is not open or no command provided'})
    except serial.SerialException as e:
        return jsonify({'status': 'error', 'message': f'Serial communication error: {e}'})

if __name__ == '__main__':
    app.run(debug=True)
