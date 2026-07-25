import Cocoa

class TrayIcon: NSObject {
    private var statusItem: NSStatusItem?
    private var channel: FlutterMethodChannel?
    private var menuItems: [[String: Any]] = []

    init(channel: FlutterMethodChannel?) {
        self.channel = channel
        super.init()
    }

    func setIcon(path: String) -> Bool {
        // Create status item if not exists
        if statusItem == nil {
            statusItem = NSStatusBar.shared.statusItem(withLength: NSStatusItem.variableLength)
            statusItem?.button?.action = #selector(onTrayIconClick)
            statusItem?.button?.target = self
        }

        // Load icon
        if let image = NSImage(contentsOfFile: path) {
            image.size = NSSize(width: 18, height: 18)
            image.isTemplate = true
            statusItem?.button?.image = image
            return true
        }

        return false
    }

    func setMenu(items: [[String: Any]]) -> Bool {
        menuItems = items

        let menu = NSMenu()

        for itemData in items {
            let type = itemData["type"] as? String ?? "normal"

            if type == "separator" {
                menu.addItem(NSMenuItem.separator())
            } else {
                let label = itemData["label"] as? String ?? ""
                let key = itemData["key"] as? String ?? ""
                let menuItem = NSMenuItem(
                    title: label,
                    action: #selector(onMenuItemClick(_:)),
                    keyEquivalent: ""
                )
                menuItem.target = self
                menuItem.representedObject = key
                menu.addItem(menuItem)
            }
        }

        statusItem?.menu = menu
        return true
    }

    func popUpContextMenu() {
        if let button = statusItem?.button {
            statusItem?.menu?.popUp(
                positioning: nil,
                at: NSPoint(x: 0, y: 0),
                in: button
            )
        }
    }

    func destroy() {
        if let item = statusItem {
            NSStatusBar.shared.removeStatusItem(item)
            statusItem = nil
        }
    }

    @objc private func onTrayIconClick() {
        channel?.invokeMethod("onTrayIconClick", arguments: nil)
    }

    @objc private func onMenuItemClick(_ sender: NSMenuItem) {
        guard let key = sender.representedObject as? String else { return }
        let id = key.hashValue
        channel?.invokeMethod("onTrayMenuItemClick", arguments: ["id": id])
    }
}
