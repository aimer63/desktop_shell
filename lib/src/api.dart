import 'dart:async';
import 'dart:io';

import 'package:flutter/services.dart';
import 'package:path/path.dart' as path;
import 'package:unwrap_me/unwrap_me.dart';

import 'errors.dart';
import 'menu/menu.dart';

/// The single platform channel for all operations.
const _channel = MethodChannel('desktop_shell');

/// Initialize desktop shell with tray and window management.
///
/// Returns a [Result] containing [DesktopShell] instance on success,
/// or [DesktopShellError] on failure.
Future<Result<DesktopShell, DesktopShellError>> initialize({
  required String trayIcon,
  required List<MenuItem> trayItems,
  required void Function(DesktopShell shell) onWindowClose,
  required void Function(DesktopShell shell) onTrayIconClick,
  required void Function(DesktopShell shell, MenuItem item) onTrayMenuItemClick,
}) async {
  // Check platform support
  if (!Platform.isWindows && !Platform.isLinux && !Platform.isMacOS) {
    return const Err(UnsupportedPlatformError('unsupported'));
  }

  // Create shell instance
  final shell = _DesktopShellImpl(
    onWindowClose: onWindowClose,
    onTrayIconClick: onTrayIconClick,
    onTrayMenuItemClick: onTrayMenuItemClick,
  );

  // Register handler for native events
  _channel.setMethodCallHandler((call) async {
    await shell._dispatchNativeEvent(call);
  });

  // Initialize tray icon
  final iconResult = await shell.setTrayIcon(trayIcon);
  if (iconResult case Err(:final error)) {
    return Err(error);
  }

  // Initialize tray menu
  final menuResult = await shell.setTrayMenu(trayItems);
  if (menuResult case Err(:final error)) {
    return Err(error);
  }

  return Ok(shell);
}

/// Main desktop shell controller interface.
///
/// All operations return [Result] for explicit error handling.
abstract class DesktopShell {
  /// Set tray icon from path.
  Future<Result<(), TrayIconError>> setTrayIcon(String iconPath);

  /// Set tray context menu.
  Future<Result<(), TrayMenuError>> setTrayMenu(List<MenuItem> items);

  /// Show context menu (Linux: no-op, automatic on click).
  Future<Result<(), TrayPopupError>> popUpTrayMenu();

  /// Show window from tray.
  Future<Result<(), WindowShowError>> show();

  /// Hide window to tray.
  Future<Result<(), WindowHideError>> hide();

  /// Focus window.
  Future<Result<(), WindowFocusError>> focus();

  /// Set prevent close flag.
  Future<Result<(), WindowPreventCloseError>> setPreventClose(bool prevent);

  /// Cleanup and destroy resources.
  Future<Result<(), ShellDestroyError>> destroy();
}

/// Implementation of [DesktopShell].
final class _DesktopShellImpl implements DesktopShell {
  final void Function(DesktopShell shell) onWindowClose;
  final void Function(DesktopShell shell) onTrayIconClick;
  final void Function(DesktopShell shell, MenuItem item) onTrayMenuItemClick;

  Menu? _currentMenu;
  bool _isDestroyed = false;

  _DesktopShellImpl({
    required this.onWindowClose,
    required this.onTrayIconClick,
    required this.onTrayMenuItemClick,
  });

  Future<void> _dispatchNativeEvent(MethodCall call) async {
    switch (call.method) {
      case 'onWindowClose':
        onWindowClose(this);
      case 'onTrayIconClick':
        onTrayIconClick(this);
      case 'onTrayMenuItemClick':
        final id = call.arguments['id'] as int;
        final item = _currentMenu?.getMenuItemById(id);
        if (item != null) {
          onTrayMenuItemClick(this, item);
        }
    }
  }

  @override
  Future<Result<(), TrayIconError>> setTrayIcon(String iconPath) async {
    if (_isDestroyed) {
      return const Err(TrayIconError('Shell has been destroyed'));
    }

    try {
      final resolvedPath = path.joinAll([
        path.dirname(Platform.resolvedExecutable),
        'data/flutter_assets',
        iconPath,
      ]);

      final result = await _channel.invokeMethod<Map<dynamic, dynamic>>(
        'setTrayIcon',
        {'iconPath': resolvedPath},
      );

      if (result == null) {
        return const Err(TrayIconError('Native returned null'));
      }

      if (result['success'] == true) {
        return const Ok(());
      }

      return Err(
        TrayIconError(
          result['message'] as String? ?? 'Unknown error',
          code: Option.fromNullable(result['code'] as String?),
        ),
      );
    } catch (e) {
      return Err(TrayIconError(e.toString()));
    }
  }

  @override
  Future<Result<(), TrayMenuError>> setTrayMenu(List<MenuItem> items) async {
    if (_isDestroyed) {
      return const Err(TrayMenuError('Shell has been destroyed'));
    }

    try {
      _currentMenu = Menu(items: items);

      final result = await _channel.invokeMethod<Map<dynamic, dynamic>>(
        'setTrayMenu',
        {'menu': _currentMenu!.toJson()},
      );

      if (result == null) {
        return const Err(TrayMenuError('Native returned null'));
      }

      if (result['success'] == true) {
        return const Ok(());
      }

      return Err(
        TrayMenuError(
          result['message'] as String? ?? 'Unknown error',
          code: Option.fromNullable(result['code'] as String?),
        ),
      );
    } catch (e) {
      return Err(TrayMenuError(e.toString()));
    }
  }

  @override
  Future<Result<(), TrayPopupError>> popUpTrayMenu() async {
    if (_isDestroyed) {
      return const Err(TrayPopupError('Shell has been destroyed'));
    }

    // Linux: no-op, menu appears automatically on tray icon click
    if (Platform.isLinux) {
      return const Ok(());
    }

    try {
      final result = await _channel.invokeMethod<Map<dynamic, dynamic>>(
        'popUpTrayMenu',
      );

      if (result == null) {
        return const Err(TrayPopupError('Native returned null'));
      }

      if (result['success'] == true) {
        return const Ok(());
      }

      return Err(
        TrayPopupError(
          result['message'] as String? ?? 'Unknown error',
          code: Option.fromNullable(result['code'] as String?),
        ),
      );
    } catch (e) {
      return Err(TrayPopupError(e.toString()));
    }
  }

  @override
  Future<Result<(), WindowShowError>> show() async {
    if (_isDestroyed) {
      return const Err(WindowShowError('Shell has been destroyed'));
    }

    try {
      final result = await _channel.invokeMethod<Map<dynamic, dynamic>>('show');

      if (result == null) {
        return const Err(WindowShowError('Native returned null'));
      }

      if (result['success'] == true) {
        return const Ok(());
      }

      return Err(
        WindowShowError(
          result['message'] as String? ?? 'Unknown error',
          code: Option.fromNullable(result['code'] as String?),
        ),
      );
    } catch (e) {
      return Err(WindowShowError(e.toString()));
    }
  }

  @override
  Future<Result<(), WindowHideError>> hide() async {
    if (_isDestroyed) {
      return const Err(WindowHideError('Shell has been destroyed'));
    }

    try {
      final result = await _channel.invokeMethod<Map<dynamic, dynamic>>('hide');

      if (result == null) {
        return const Err(WindowHideError('Native returned null'));
      }

      if (result['success'] == true) {
        return const Ok(());
      }

      return Err(
        WindowHideError(
          result['message'] as String? ?? 'Unknown error',
          code: Option.fromNullable(result['code'] as String?),
        ),
      );
    } catch (e) {
      return Err(WindowHideError(e.toString()));
    }
  }

  @override
  Future<Result<(), WindowFocusError>> focus() async {
    if (_isDestroyed) {
      return const Err(WindowFocusError('Shell has been destroyed'));
    }

    try {
      final result = await _channel.invokeMethod<Map<dynamic, dynamic>>(
        'focus',
      );

      if (result == null) {
        return const Err(WindowFocusError('Native returned null'));
      }

      if (result['success'] == true) {
        return const Ok(());
      }

      return Err(
        WindowFocusError(
          result['message'] as String? ?? 'Unknown error',
          code: Option.fromNullable(result['code'] as String?),
        ),
      );
    } catch (e) {
      return Err(WindowFocusError(e.toString()));
    }
  }

  @override
  Future<Result<(), WindowPreventCloseError>> setPreventClose(
    bool prevent,
  ) async {
    if (_isDestroyed) {
      return const Err(WindowPreventCloseError('Shell has been destroyed'));
    }

    try {
      final result = await _channel.invokeMethod<Map<dynamic, dynamic>>(
        'setPreventClose',
        {'prevent': prevent},
      );

      if (result == null) {
        return const Err(WindowPreventCloseError('Native returned null'));
      }

      if (result['success'] == true) {
        return const Ok(());
      }

      return Err(
        WindowPreventCloseError(
          result['message'] as String? ?? 'Unknown error',
          code: Option.fromNullable(result['code'] as String?),
        ),
      );
    } catch (e) {
      return Err(WindowPreventCloseError(e.toString()));
    }
  }

  @override
  Future<Result<(), ShellDestroyError>> destroy() async {
    if (_isDestroyed) {
      return const Ok(());
    }

    try {
      final result = await _channel.invokeMethod<Map<dynamic, dynamic>>(
        'destroy',
      );

      _isDestroyed = true;
      _channel.setMethodCallHandler(null);

      if (result == null) {
        return const Err(ShellDestroyError('Native returned null'));
      }

      if (result['success'] == true) {
        return const Ok(());
      }

      return Err(
        ShellDestroyError(
          result['message'] as String? ?? 'Unknown error',
          code: Option.fromNullable(result['code'] as String?),
        ),
      );
    } catch (e) {
      return Err(ShellDestroyError(e.toString()));
    }
  }
}
