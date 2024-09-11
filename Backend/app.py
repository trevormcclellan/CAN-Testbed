# app.py
from flask import Flask, request, jsonify
from flask_cors import CORS
import os

app = Flask(__name__)
CORS(app)

UPLOAD_FOLDER = 'uploads'
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

# Hardcoded number of nodes
NUM_NODES = 3

def parse_candump_file(filepath):
    can_ids = set()
    with open(filepath, 'r') as file:
        for line in file:
            parts = line.strip().split()
            if len(parts) > 2:
                # Example line format: (1290000000.000000) can0 585#0040000000020018
                can_id = parts[2].split('#')[0]  # Extract CAN ID before the #
                can_ids.add(can_id)
    return list(can_ids)

@app.route('/upload', methods=['POST'])
def upload_file():
    if 'file' not in request.files:
        return jsonify({'status': 'No file part in the request'}), 400
    
    file = request.files['file']
    if file.filename == '':
        return jsonify({'status': 'No selected file'}), 400
    
    filepath = os.path.join(UPLOAD_FOLDER, file.filename)
    file.save(filepath)
    
    unique_ids = parse_candump_file(filepath)
    return jsonify({'status': 'File uploaded successfully', 'unique_ids': unique_ids}), 200

@app.route('/nodes', methods=['GET'])
def get_nodes():
    nodes = []
    for node_id in range(1, NUM_NODES + 1):
        nodes.append({
            'node_id': node_id,
            'name': f'Node {node_id}',
            'description': f'This is node {node_id}'
        })
    return jsonify({'num_nodes': NUM_NODES, 'nodes': nodes}), 200

@app.route('/node/<int:node_id>/select_ids', methods=['POST'])
def select_ids_for_node(node_id):
    if not 1 <= node_id <= NUM_NODES:
        return jsonify({'status': 'Invalid node ID'}), 400
    
    selected_ids = request.json.get('selected_ids', [])
    # Here we can process or store the selected IDs as needed.
    # For now, we just return them in the response.
    return jsonify({'status': 'IDs selected successfully', 'node_id': node_id, 'selected_ids': selected_ids}), 200

if __name__ == '__main__':
    app.run(debug=True)
