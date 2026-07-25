import Cocoa
import FlutterMacOS

public class DesktopShellPlugin: NSObject, FlutterPlugin {
    private var channel: FlutterMethodChannel?
    private var trayIcon: TrayIcon?
    private var windowManager: WindowManager?
    private var onWindowClose: (() -> Void)?
    private var onTrayIconClick: (() -> Void)?

    public static func register(with registrar: FlutterPluginRegistrar) {
        let channel = FlutterMethodChannel(
            name: "desktop_shell",
            binaryMessenger: registrar.messenger
        )
        let instance = DesktopShellPlugin()
        instance.channel = channel
        registrar.addMethodCallDelegate(instance, channel: channel)

        // Initialize tray and window managers
        instance.trayIcon = TrayIcon(channel: channel)
        instance.windowManager = WindowManager(
            window: NSApplication.shared.mainWindow,
            channel: channel
        )
    }

    public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
        let success: () -> Void = {
            result(true)
        }

        let error: (String) -> Void = { message in
            result([
                "error": true,
                "message": message
            ])
        }

        switch call.method {
        case "setTrayIcon":
            guard let args = call.arguments as? [String: Any],
                  let iconPath = args["iconPath"] as? String else {
                error("Missing or invalid iconPath")
                return
            }

            if trayIcon?.setIcon(path: iconPath) ?? false {
                success()
            } else {
                error("Failed to set tray icon")
            }

        case "setTrayMenu":
            guard let args = call.arguments as? [String: Any],
                  let menuItems = args["menu"] as? [[String: Any]] else {
                error("Missing or invalid menu")
                return
            }

            if trayIcon?.setMenu(items: menuItems) ?? false {
                success()
            } else {
                error("Failed to set tray menu")
            }

        case "popUpContextMenu":
            trayIcon?.popUpContextMenu()
            success()

        case "showWindow":
            if windowManager?.show() ?? false {
                success()
            } else {
                error("Failed to show window")
            }

        case "hideWindow":
            if windowManager?.hide() ?? false {
                success()
            } else {
                error("Failed to hide window")
            }

        case "focusWindow":
            if windowManager?.focus() ?? false {
                success()
            } else {
                error("Failed to focus window")
            }

        case "setPreventClose":
            guard let args = call.arguments as? [String: Any],
                  let prevent = args["prevent"] as? Bool else {
                error("Missing or invalid prevent flag")
                return
            }

            if windowManager?.setPreventClose(prevent) ?? false {
                success()
            } else {
                error("Failed to set prevent close")
            }

        case "destroy":
            trayIcon?.destroy()
            windowManager?.destroy()
            success()

        default:
            result(FlutterMethodNotImplemented)
        }
    }
}
