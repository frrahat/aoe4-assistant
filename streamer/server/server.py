import os
import json
import argparse
from flask import Flask, send_from_directory, request, abort, jsonify
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--stream', type=int, default=1, help='Index (1-based) of search_rectangle to update')
args, _ = parser.parse_known_args()
STREAM_INDEX = max(1, args.stream) - 1

app = Flask(__name__, static_folder='../client', static_url_path='')

IMAGE_FOLDER = 'images'  # You can change this to your image directory

DEFAULT_CONFIG = {
    "interval_ms": 1000,
    "search_rectangles": [
        {"name": "villager_production_checker", "width": 800, "height": 600, "x": 0, "y": 0}
    ]
}

def get_config_path():
    here = Path(__file__).resolve().parent
    return (here.parent.parent / 'assistant' / 'config.json').resolve()

def read_config():
    try:
        with open(get_config_path(), 'r') as f:
            return json.load(f)
    except Exception:
        return DEFAULT_CONFIG.copy()

def write_config(config):
    os.makedirs(get_config_path().parent, exist_ok=True)
    with open(get_config_path(), 'w') as f:
        json.dump(config, f, indent=2)

@app.route('/')
def index():
    return app.send_static_file('index.html')

@app.route('/config', methods=['GET', 'POST'])
def config():
    if request.method == 'GET':
        return jsonify(read_config())
    elif request.method == 'POST':
        data = request.get_json()
        width = data.get('width')
        height = data.get('height')
        x = data.get('x')
        y = data.get('y')
        # Validation
        if not isinstance(width, int) or not isinstance(height, int) or not isinstance(x, int) or not isinstance(y, int):
            return jsonify({'error': 'Width, height, x, and y must be integers'}), 400
        if not (10 <= width <= 1000) or not (10 <= height <= 1000):
            return jsonify({'error': 'Width and height must be between 10 and 1000'}), 400
        if not (0 <= x <= 10000) or not (0 <= y <= 50000):
            return jsonify({'error': 'X must be between 0 and 10000, Y must be between 0 and 50000'}), 400
        config = read_config()
        # Update the search_rectangle at STREAM_INDEX
        if 'search_rectangles' in config and len(config['search_rectangles']) > STREAM_INDEX:
            config['search_rectangles'][STREAM_INDEX]['width'] = width
            config['search_rectangles'][STREAM_INDEX]['height'] = height
            config['search_rectangles'][STREAM_INDEX]['x'] = x
            config['search_rectangles'][STREAM_INDEX]['y'] = y
        write_config(config)
        return jsonify({'success': True, 'config': config})
    abort(404)

@app.route('/latest-image')
def latest_image():
    config = read_config()
    folder = Path(IMAGE_FOLDER)
    if not folder.exists() or not any(folder.iterdir()):
        abort(404)
    images = sorted(folder.glob('*.png'), key=lambda f: f.stat().st_mtime, reverse=True)
    if not images:
        abort(404)
    latest = images[0]
    return send_from_directory(Path('../../', IMAGE_FOLDER), Path(latest).name, mimetype='image/png', as_attachment=False)

@app.route('/<path:path>')
def static_proxy(path):
    return app.send_static_file(path)

if __name__ == '__main__':
    os.makedirs(IMAGE_FOLDER, exist_ok=True)
    app.run(host='0.0.0.0', port=5000, debug=True) 