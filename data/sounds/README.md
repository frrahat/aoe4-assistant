# Custom Sound Files

This directory contains custom sound files for different notification categories in the AOE4 Assistant.

## Required Files

Place the following WAV files in this directory for custom notification sounds:

- `info.wav` - Played when recording starts/stops (Info notifications)
- `warning.wav` - Played when idle workers are detected (Warning notifications)  
- `error.wav` - Played when villager production stops (Error notifications)

## File Requirements

- Format: WAV files (.wav extension)
- The application will fallback to system beep sounds if custom files are not found
- Files are played asynchronously, so they won't block the application

## Sound Suggestions

- **info.wav**: A pleasant chime or ding sound for start/stop recording
- **warning.wav**: A moderate alert sound for idle worker notifications
- **error.wav**: An urgent/alarming sound for critical issues like stopped villager production

You can find free sound effects from:
- freesound.org
- zapsplat.com
- Adobe Audition's built-in sound library
- Windows system sounds (can be exported from C:\Windows\Media\)
