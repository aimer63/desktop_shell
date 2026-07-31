import 'package:unwrap_me/unwrap_me.dart';

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
  /// Note: ID is assigned by Menu.toJson() to ensure sequential 16-bit compatible IDs.
  Map<String, dynamic> toJson(int id) {
    return {
      'id': id,
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

  /// Maps sequential IDs (1024+) to menu items for O(1) lookup.
  late final Map<int, MenuItem> _idToItem;

  Menu({required this.items}) {
    // Assign sequential IDs starting at 1024 (avoids confusion with menu indices)
    _idToItem = {};
    var nextId = 1024;
    for (final item in items) {
      _idToItem[nextId++] = item;
    }
  }

  /// Convert to JSON for platform channel.
  /// Uses the pre-computed ID mapping to ensure consistency between
  /// the IDs sent to native and the IDs used for lookup.
  List<Map<String, dynamic>> toJson() {
    // Use _idToItem entries to guarantee IDs match the lookup table.
    return _idToItem.entries.map((entry) {
      final id = entry.key; // Sequential ID (1024, 1025, ...)
      final item = entry.value; // Corresponding MenuItem
      return item.toJson(id);
    }).toList();
  }

  /// Find a menu item by its sequential ID.
  Option<MenuItem> getMenuItemById(int id) => Option.fromNullable(_idToItem[id]);
}
