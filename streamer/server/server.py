import os
from flask import Flask, send_from_directory, request, abort, jsonify
from pathlib import Path

app = Flask(__name__, static_folder='../client', static_url_path='')

IMAGE_FOLDER = 'images'  # You can change this to your image directory
CONFIG_FILE = 'recorder/config.txt'

def read_config():
    """Read current config values"""
    config = {'interval_ms': 1000, 'width': 800, 'height': 600, 'x': 0, 'y': 0}  # defaults
    try:
        with open(CONFIG_FILE, 'r') as f:
            for line in f:
                line = line.strip()
                if '=' in line:
                    key, value = line.split('=', 1)
                    if key in config:
                        config[key] = int(value)
    except FileNotFoundError:
        pass
    return config

def write_config(config):
    """Write config values to file"""
    os.makedirs(os.path.dirname(CONFIG_FILE), exist_ok=True)
    with open(CONFIG_FILE, 'w') as f:
        f.write(f"interval_ms={config['interval_ms']}\n")
        f.write(f"width={config['width']}\n")
        f.write(f"height={config['height']}\n")
        f.write(f"x={config['x']}\n")
        f.write(f"y={config['y']}\n")

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
        
        # Update config
        config = read_config()
        config['width'] = width
        config['height'] = height
        config['x'] = x
        config['y'] = y
        write_config(config)
        
        return jsonify({'success': True, 'config': config})
    
    abort(404)

@app.route('/latest-image')
def latest_image():
    config = read_config()
    folder = Path(IMAGE_FOLDER)
    if not folder.exists() or not any(folder.iterdir()):
        abort(404)
    # Find the latest image by modified time or timestamp in filename
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