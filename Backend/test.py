import serial
import threading
import time
from flask import Flask, render_template, jsonify, request

app = Flask(__name__)

ser = serial.Serial('COM4', 115200, timeout=1)

# Flag to signal the serial_reader thread to stop
running = True

def serial_reader():
    global running
    while running:
        print("Reading")
        if ser.in_waiting > 0:
            data = ser.readline().decode('utf-8').strip()
            print(f'Received from Pi Pico: {data}')
            # Handle received data as needed, e.g., store in a database or process further
        time.sleep(0.1)  # Adjust as needed

try:
    thread = threading.Thread(target=serial_reader)
    thread.start()

    # Keep the main thread alive until interrupted
    while True:
        time.sleep(1)

except KeyboardInterrupt:
    print("Interrupted, stopping...")
    running = False  # Set the flag to stop the serial_reader thread
    thread.join()    # Wait for the serial_reader thread to complete

print("Program exited gracefully.")

@app.route('/')
def index():
    return render_template('index.html')

if __name__ == '__main__':
    app.run(debug=True)