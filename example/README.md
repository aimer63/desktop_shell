# desktop_shell Example App

This example app demonstrates the desktop_shell plugin functionality for system tray and window management.

## Features Demonstrated

- **System Tray Integration**: Shows an icon in the system tray with a context menu
- **Minimize to Tray**: Clicking the X button hides the window to the tray instead of closing
- **Window Management**: Show, hide, and focus window operations
- **Result-based Error Handling**: All operations use `unwrap_me` Result types

## Running the Example

```bash
# From the example directory
cd example

# Get dependencies
flutter pub get

# Run on your platform
flutter run -d linux      # For Linux
flutter run -d windows    # For Windows  
flutter run -d macos      # For macOS
```

## What to Test

1. **Window Controls**: Click "Hide to Tray" - window should disappear but app keeps running
2. **Tray Menu**: Click the tray icon to show the menu, then click "Show Window"
3. **Close Button**: Click the X (close) button - should hide to tray, not quit
4. **Tray Quit**: Use "Quit" from tray menu to properly exit the app

## Expected Behavior

- App starts with a visible window and tray icon
- "Hide to Tray" button hides the window
- "Show Window" button restores the window
- "Focus Window" button brings window to front
- Clicking X minimizes to tray (doesn't quit)
- Tray icon shows context menu with "Show Window" and "Quit" options

## Platform-Specific Notes

**Linux**: Menu appears automatically on tray icon click (no explicit popup needed)
**Windows/macOS**: Menu appears on right-click of tray icon
