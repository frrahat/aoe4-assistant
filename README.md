# Age of Empires 4 Assistant

A real-time screen monitoring tool for Age of Empires IV that helps players track villager production and other gameplay metrics. The assistant captures specific screen regions, analyzes gameplay elements using computer vision, and provides audio notifications to help optimize your gameplay.

## Features

- **Real-time Villager Production Monitoring**: Automatically detects when you're not producing villagers and alerts you with audio notifications
- **Multi-Civilization Support**: Supports all 16 civilizations with civilization-specific screen coordinates
- **Hotkey Control**: Start/stop monitoring with Numpad '*' key
- **Web-based Configuration**: Live configuration interface for adjusting capture regions
- **Screenshot Management**: Automatic screenshot capture and cleanup (keeps latest 5)
- **Template Matching**: Uses OpenCV for robust image recognition across different civilizations

## Supported Civilizations

- Abbasid Dynasty
- Ayyubids  
- Byzantines
- Chinese
- Delhi Sultanate
- English
- French
- Holy Roman Empire
- Japanese
- Jeanne d'Arc
- Malians
- Mongols
- Order of the Dragon
- Ottomans
- Rus
- Zhu Xi's Legacy

## Requirements

### System Requirements
- Windows 10/11
- Visual Studio Build Tools or Visual Studio Community
- Age of Empires IV

### Dependencies
- **OpenCV 4.11.0**: Computer vision library for image processing
- **nlohmann/json**: JSON parsing library (included in `third_party/`)
- **Windows GDI+**: For screen capture functionality
- **Python 3.x**: For the web configuration interface
- **Flask**: Python web framework for the streamer server

### OpenCV Setup
1. Download OpenCV 4.11.0 from [opencv.org](https://opencv.org/releases/)
2. Extract to `C:\Users\[username]\Downloads\opencv`
3. The build script expects the following structure:
   ```
   C:\Users\[username]\Downloads\opencv\
   ├── build\
   │   ├── include\
   │   └── x64\vc16\
   │       ├── lib\
   │       └── bin\
   ```

## Installation

1. **Clone the repository**:
   ```bash
   git clone https://github.com/frrahat/aoe4-assistant.git
   cd aoe4-assistant
   ```

2. **Install Python dependencies** (for web interface):
   ```bash
   python -m venv .venv
   .venv\Scripts\activate
   pip install flask
   ```

3. **Build and run**:
   ```bash
   build_and_run.bat
   ```

## Usage

### Basic Usage

1. **Start the application**:
   ```bash
   build_and_run.bat
   ```

2. **Select your civilization** from the numbered list (0-15)

3. **Launch Age of Empires IV** and start a match

4. **Press Numpad '*'** to start monitoring
   - You'll hear a notification sound when monitoring starts
   - The assistant will continuously check your villager production
   - You'll hear an alarm sound if no villagers are being produced

5. **Press Numpad '*' again** to stop monitoring

### Advanced Usage

#### Development Mode with Streaming
```bash
build_and_run.bat 1
```
This enables:
- Live configuration updates
- Web interface at `http://localhost:5000`
- Screenshot streaming for debugging

#### Testing Mode
```bash
build_and_run.bat test
```
Runs only the villager production checker tests.

### Web Configuration Interface

When running in streaming mode, access `http://localhost:5000` to:
- View live screenshots of the monitored region
- Adjust capture coordinates (x, y, width, height)
- Test different capture regions in real-time
- Save captured images with metadata

## Configuration

### Config File Format (`temp/config.json`)
```json
{
  "interval_ms": 1000,
  "search_rectangles": [
    {
      "name": "villager_production_checker",
      "width": 300,
      "height": 36,
      "x": 11,
      "y": 733
    }
  ]
}
```

### Civilization-Specific Coordinates
Each civilization has pre-configured screen coordinates for optimal detection:
- Most civilizations: `{x: 11, y: 733}`
- Japanese: `{x: 11, y: 655}` (different UI layout)
- Abbasid/Ayyubids: `{x: 11, y: 729}`

## Project Structure

```
aoe4_assistant/
├── assistant/
│   ├── main.cpp                           # Main application entry point
│   └── villager_production_checker/       # Villager production monitoring
├── build/                                 # Compiled binaries and data
├── data/                                  # Template images and samples
│   ├── templates/villagers/               # Civilization-specific villager icons
│   └── samples/production_queue/          # Test images for development
├── matcher/                               # OpenCV template matching utilities
├── recorder/                              # Screen capture functionality  
├── streamer/                              # Web interface for configuration
│   ├── client/index.html                  # Web UI
│   └── server/server.py                   # Flask backend
├── third_party/json.hpp                   # JSON parsing library
├── build_and_run.bat                      # Build and run script
└── README.md                              # This file
```

## How It Works

1. **Screen Capture**: The application captures specific screen regions at configurable intervals (default: 1000ms)

2. **Template Matching**: Uses OpenCV to match villager production icons against pre-recorded templates for each civilization

3. **Production Detection**: Analyzes the production queue area to determine if villagers are being produced

4. **Audio Notifications**: 
   - Info sound: Monitoring started
   - Warning sound: Monitoring stopped  
   - Error sound: No villager production detected

5. **Web Interface**: Real-time configuration and debugging through a Flask web server

## Development

### Building from Source
```bash
# Build main application
cl.exe /EHsc /std:c++17 assistant/main.cpp recorder/recorder.cpp matcher/matcher.cpp assistant/villager_production_checker/villager_production_checker.cpp /I./ /I%OPENCV_INCLUDE% /Fe:build/aoe4_assistant.exe /link gdiplus.lib user32.lib gdi32.lib %OPENCV_LIB%\opencv_world4110.lib

# Build tests
cl.exe /EHsc /std:c++17 assistant/villager_production_checker/villager_production_checker_test.cpp assistant/villager_production_checker/villager_production_checker.cpp matcher/matcher.cpp /I./ /I%OPENCV_INCLUDE% /Fe:build/villager_production_test.exe /link gdiplus.lib user32.lib gdi32.lib %OPENCV_LIB%\opencv_world4110.lib
```

### Adding New Features
1. Create new checker modules following the `villager_production_checker` pattern
2. Add new search rectangles to the configuration
3. Implement template matching for new UI elements
4. Add corresponding test cases

## Troubleshooting

### Common Issues

**"Config file not found" error**:
- Run in streaming mode first: `build_and_run.bat 1`
- This creates the necessary `temp/config.json` file

**OpenCV linking errors**:
- Verify OpenCV is installed at the expected path
- Check that `opencv_world4110.dll` is in your PATH
- Ensure you're using the correct Visual Studio toolchain (vc16)

**No villager detection**:
- Use streaming mode to verify capture coordinates
- Check that your game resolution matches the configured coordinates
- Ensure Age of Empires IV is running in windowed or borderless mode

**Hotkey not working**:
- Make sure the application has focus
- Try running as administrator
- Verify Numpad is not disabled

### Getting Help

1. Run the test suite: `build_and_run.bat test`
2. Use streaming mode to debug: `build_and_run.bat 1`
3. Check the web interface at `http://localhost:5000`
4. Review captured screenshots in the `images/` folder

## Contributing

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Submit a pull request

## License

This project is open source. Please ensure you comply with Age of Empires IV's terms of service when using this tool.

## Disclaimer

This tool is designed to assist with gameplay awareness and does not provide any unfair advantages. It simply monitors publicly visible UI elements and provides audio cues that a human player would notice anyway. Use responsibly and in accordance with the game's terms of service.