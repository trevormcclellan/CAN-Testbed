# app.py
from flask import Flask, request, jsonify
from flask_cors import CORS
import os

app = Flask(__name__)
CORS(app)

UPLOAD_FOLDER = 'uploads'
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

def parse_candump_file(filepath):
    can_ids = set()
    with open(filepath, 'r') as file:
        for line in file:
            parts = line.strip().split()
            if len(parts) > 2:
                # Example line format: (1290000000.000000) can0 585#0040000000020018
                can_id = parts[2].split('#')[0]  # Extract CAN ID before the #
                can_ids.add(can_id)
    return len(can_ids)

@app.route('/upload', methods=['POST'])
def upload_file():
    if 'file' not in request.files:
        return jsonify({'status': 'No file part in the request'}), 400
    
    file = request.files['file']
    if file.filename == '':
        return jsonify({'status': 'No selected file'}), 400
    
    filepath = os.path.join(UPLOAD_FOLDER, file.filename)
    file.save(filepath)
    
    unique_id_count = parse_candump_file(filepath)
    return jsonify({'status': 'File uploaded successfully', 'unique_id_count': unique_id_count}), 200

if __name__ == '__main__':
    app.run(debug=True)
