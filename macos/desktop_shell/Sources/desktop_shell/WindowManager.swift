import Cocoa

class WindowManager {
    private weak var window: NSWindow?
    private var channel: FlutterMethodChannel?
    private var preventClose: Bool = false
    private var originalDelegate: NSWindowDelegate?

    init(window: NSWindow?, channel: FlutterMethodChannel?) {
        self.window = window
        self.channel = channel

        // Set up window delegate to intercept close
        if let win = window {
            originalDelegate = win.delegate
            win.delegate = self
        }
    }

    func show() -> Bool {
        window?.makeKeyAndOrderFront(nil)
        NSApp.activate(ignoringOtherApps: true)
        return true
    }

    func hide() -> Bool {
        window?.orderOut(nil)
        return true
    }

    func focus() -> Bool {
        window?.makeKeyAndOrderFront(nil)
        NSApp.activate(ignoringOtherApps: true)
        return true
    }

    func setPreventClose(_ prevent: Bool) -> Bool {
        preventClose = prevent
        return true
    }

    func destroy() -> Bool {
        // Restore original delegate
        window?.delegate = originalDelegate
        return true
    }
}

// MARK: - NSWindowDelegate

extension WindowManager: NSWindowDelegate {
    func windowShouldClose(_ sender: NSWindow) -> Bool {
        if preventClose {
            // Hide instead of close
            hide()
            channel?.invokeMethod("onWindowClose", arguments: nil)
            return false
        }
        return originalDelegate?.windowShouldClose?(sender) ?? true
    }

    // Forward all other delegate methods to original delegate
    func windowWillClose(_ notification: Notification) {
        originalDelegate?.windowWillClose?(notification)
    }

    func windowDidBecomeKey(_ notification: Notification) {
        originalDelegate?.windowDidBecomeKey?(notification)
    }

    func windowDidResignKey(_ notification: Notification) {
        originalDelegate?.windowDidResignKey?(notification)
    }

    func windowDidMiniaturize(_ notification: Notification) {
        originalDelegate?.windowDidMiniaturize?(notification)
    }

    func windowDidDeminiaturize(_ notification: Notification) {
        originalDelegate?.windowDidDeminiaturize?(notification)
    }
}
