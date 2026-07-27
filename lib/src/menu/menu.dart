/// A menu item for the system tray context menu.
final class MenuItem {
  /// Unique identifier for the menu item.
  final String key;

  /// Display label for the menu item.
  final String label;

  /// Whether this item is a separator.
  final bool isSeparator;

  /// Whether this item is checked (for checkbox items).
  final bool checked;

  const MenuItem({required this.key, required this.label, this.checked = false})
    : isSeparator = false;

  const MenuItem.separator()
    : key = '',
      label = '',
      isSeparator = true,
      checked = false;

  /// Convert to JSON for platform channel.
  Map<String, dynamic> toJson() {
    return {
      'id': key.hashCode,
      'key': key,
      'label': label,
      'type': isSeparator ? 'separator' : (checked ? 'checkbox' : 'normal'),
      'checked': checked,
    };
  }
}

/// A menu containing multiple menu items.
final class Menu {
  final List<MenuItem> items;

  const Menu({required this.items});

  /// Convert to JSON for platform channel.
  List<Map<String, dynamic>> toJson() {
    return items.map((item) => item.toJson()).toList();
  }

  /// Find a menu item by its key hash.
  MenuItem? getMenuItemById(int id) {
    for (final item in items) {
      if (item.key.hashCode == id) {
        return item;
      }
    }
    return null;
  }
}
