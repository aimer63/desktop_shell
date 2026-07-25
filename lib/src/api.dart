import 'dart:async';
import 'dart:io';

import 'package:flutter/services.dart';
import 'package:path/path.dart' as path;
import 'package:shortid/shortid.dart';
import 'package:unwrap_me/unwrap_me.dart';

import 'errors.dart';
import 'menu/menu.dart';
import 'platform_channel.dart';

/// Initialize desktop shell with tray and window management.
///
/// Returns a [Result] containing [DesktopShell] instance on success,
/// or [DesktopShellError] on failure.
///
/// Example:
/// ```dart
/// final result = await initialize(
///   trayIcon: Platform.isWindows ? 'assets/icon.ico' : 'assets/icon.png',
///   trayItems: [
///     MenuItem(key: 'show', label: 'Open'),
///     MenuItem.separator(),
///     MenuItem(key: 'quit', label: 'Quit'),
///   ],
///   onWindowClose: (shell) => shell.hideWindow(),
///   onTrayIconClick: (shell) => shell.popUpContextMenu(),
/// );
///
/// switch (result) {
///   case Ok(:final value):
///     runApp(MyApp(shell: value));
///   case Err(:final error):
///     stderr.writeln('Failed to initialize: ${error.message}');
///     exit(1);
/// }
/// ```
Future<Result<DesktopShell, DesktopShellError>> initialize({
  required String trayIcon,
  required List<MenuItem> trayItems,
  required void Function(DesktopShell shell) onWindowClose,
  void Function(DesktopShell shell)? onTrayIconClick,
}) async {
  // Check platform support
  if (!Platform.isWindows && !Platform.isLinux && !Platform.isMacOS) {
    return const Err(UnsupportedPlatformError('unsupported'));
  }

  // Create shell instance
  final shell = _DesktopShellImpl(
    onWindowClose: onWindowClose,
    onTrayIconClick: onTrayIconClick,
  );

  // Set up method call handler for events from native
  setMethodCallHandler((call) async {
    await shell._handleMethodCall(call);
  });

  // Initialize tray icon
  final iconResult = await shell._initializeTray(trayIcon);
  if (iconResult case Err(:final error)) {
    return Err(TrayInitError(error.message));
  }

  // Initialize tray menu
  final menuResult = await shell._setTrayMenu(trayItems);
  if (menuResult case Err(:final error)) {
    return Err(TrayInitError(error.message));
  }

  // Initialize window management (prevent close)
  final windowResult = await shell.setPreventClose(true);
  if (windowResult case Err(:final error)) {
    return Err(WindowInitError(error.message));
  }

  return Ok(shell);
}

/// Main desktop shell controller interface.
///
/// All operations return [Result] for explicit error handling.
abstract class DesktopShell {
  /// Set the tray icon.
  Future<Result<(), TrayError>> setTrayIcon(String iconPath);

  /// Set the tray context menu.
  Future<Result<(), TrayError>> setTrayMenu(List<MenuItem> items);

  /// Show the tray context menu.
  Future<Result<(), TrayError>> popUpContextMenu();

  /// Show window from tray.
  Future<Result<(), WindowOperationError>> showWindow();

  /// Hide window to tray.
  Future<Result<(), WindowOperationError>> hideWindow();

  /// Focus window.
  Future<Result<(), WindowOperationError>> focusWindow();

  /// Set whether to prevent window close (intercept close button).
  Future<Result<(), WindowOperationError>> setPreventClose(bool prevent);

  /// Destroy tray icon and window resources.
  Future<Result<(), DesktopShellError>> destroy();
}

/// Implementation of [DesktopShell].
final class _DesktopShellImpl implements DesktopShell {
  final void Function(DesktopShell shell) onWindowClose;
  final void Function(DesktopShell shell)? onTrayIconClick;

  Menu? _currentMenu;
  bool _isDestroyed = false;

  _DesktopShellImpl({
    required this.onWindowClose,
    this.onTrayIconClick,
  });

  Future<void> _handleMethodCall(MethodCall call) async {
    switch (call.method) {
      case 'onWindowClose':
        onWindowClose(this);
      case 'onTrayIconClick':
        onTrayIconClick?.call(this);
      case 'onTrayMenuItemClick':
        final id = call.arguments['id'] as int;
        final menuItem = _currentMenu?.getMenuItemById(id);
        if (menuItem?.onClick != null) {
          menuItem!.onClick!(menuItem);
        }
    }
  }

  Future<Result<(), TrayIconError>> _initializeTray(String iconPath) async {
    try {
      final resolvedPath = path.joinAll([
        path.dirname(Platform.resolvedExecutable),
        'data/flutter_assets',
        iconPath,
      ]);

      final arguments = <String, dynamic>{
        'id': shortid.generate(),
        'iconPath': resolvedPath,
      };

      // Platform-specific handling
      if (Platform.isLinux) {
        // Linux sandbox handling
        arguments['iconPath'] = iconPath;
      }

      final result = await invokeMethodWithResult('setTrayIcon', arguments);

      if (result != null && result['error'] == true) {
        return Err(TrayIconError(result['message'] as String));
      }

      return const Ok(());
    } catch (e) {
      return Err(TrayIconError(e.toString()));
    }
  }

  @override
  Future<Result<(), TrayError>> setTrayIcon(String iconPath) async {
    if (_isDestroyed) {
      return const Err(TrayIconError('Shell has been destroyed'));
    }
    return _initializeTray(iconPath);
  }

  Future<Result<(), TrayMenuError>> _setTrayMenu(List<MenuItem> items) async {
    try {
      _currentMenu = Menu(items: items);

      final arguments = <String, dynamic>{
        'menu': _currentMenu!.toJson(),
      };

      final result = await invokeMethodWithResult('setTrayMenu', arguments);

      if (result != null && result['error'] == true) {
        return Err(TrayMenuError(result['message'] as String));
      }

      return const Ok(());
    } catch (e) {
      return Err(TrayMenuError(e.toString()));
    }
  }

  @override
  Future<Result<(), TrayError>> setTrayMenu(List<MenuItem> items) async {
    if (_isDestroyed) {
      return const Err(TrayMenuError('Shell has been destroyed'));
    }
    return _setTrayMenu(items);
  }

  @override
  Future<Result<(), TrayError>> popUpContextMenu() async {
    if (_isDestroyed) {
      return const Err(TrayPopupError('Shell has been destroyed'));
    }

    try {
      final result = await invokeMethodWithResult('popUpContextMenu');

      if (result != null && result['error'] == true) {
        return Err(TrayPopupError(result['message'] as String));
      }

      return const Ok(());
    } catch (e) {
      return Err(TrayPopupError(e.toString()));
    }
  }

  @override
  Future<Result<(), WindowShowError>> showWindow() async {
    if (_isDestroyed) {
      return const Err(WindowShowError('Shell has been destroyed'));
    }

    try {
      final result = await invokeMethodWithResult('showWindow');

      if (result != null && result['error'] == true) {
        return Err(WindowShowError(result['message'] as String));
      }

      return const Ok(());
    } catch (e) {
      return Err(WindowShowError(e.toString()));
    }
  }

  @override
  Future<Result<(), WindowHideError>> hideWindow() async {
    if (_isDestroyed) {
      return const Err(WindowHideError('Shell has been destroyed'));
    }

    try {
      final result = await invokeMethodWithResult('hideWindow');

      if (result != null && result['error'] == true) {
        return Err(WindowHideError(result['message'] as String));
      }

      return const Ok(());
    } catch (e) {
      return Err(WindowHideError(e.toString()));
    }
  }

  @override
  Future<Result<(), WindowFocusError>> focusWindow() async {
    if (_isDestroyed) {
      return const Err(WindowFocusError('Shell has been destroyed'));
    }

    try {
      final result = await invokeMethodWithResult('focusWindow');

      if (result != null && result['error'] == true) {
        return Err(WindowFocusError(result['message'] as String));
      }

      return const Ok(());
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
      final arguments = <String, dynamic>{
        'prevent': prevent,
      };

      final result = await invokeMethodWithResult(
        'setPreventClose',
        arguments,
      );

      if (result != null && result['error'] == true) {
        return Err(WindowPreventCloseError(result['message'] as String));
      }

      return const Ok(());
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
      final result = await invokeMethodWithResult('destroy');

      _isDestroyed = true;
      setMethodCallHandler(null);

      if (result != null && result['error'] == true) {
        return Err(ShellDestroyError(result['message'] as String));
      }

      return const Ok(());
    } catch (e) {
      return Err(ShellDestroyError(e.toString()));
    }
  }
}
