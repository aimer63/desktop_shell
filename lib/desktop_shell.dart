/// A unified Flutter desktop plugin combining system tray and window management.
///
/// This plugin provides a minimal, focused API for desktop Flutter applications
/// that need system tray integration and window management. It uses explicit
/// error handling via the `unwrap_me` package's [Result] and [Option] types.
///
/// ## Basic Usage
///
/// ```dart
/// import 'package:desktop_shell/desktop_shell.dart';
/// import 'dart:io';
///
/// void main() async {
///   final result = await DesktopShell.initialize(
///     trayIcon: Platform.isWindows
///         ? 'assets/icon.ico'
///         : 'assets/icon.png',
///     trayItems: [
///       MenuItem(key: 'show', label: 'Open'),
///       MenuItem.separator(),
///       MenuItem(key: 'quit', label: 'Quit'),
///     ],
///     onWindowClose: (shell) async => await shell.hideWindow(),
///     onTrayIconClick: (shell) async {},
///     onTrayMenuItemClick: (shell, item) async {
///       switch (item.key) {
///         case 'show':
///           await shell.showWindow();
///           await shell.focusWindow();
///         case 'quit':
///           await shell.destroy();
///           exit(0);
///       }
///     },
///   );
///
///   switch (result) {
///     case Ok(value: final shell):
///       runApp(MyApp(shell: shell));
///     case Err(:final error):
///       stderr.writeln('Failed: ${error.message}');
///       exit(1);
///   }
/// }
/// ```
library;

export 'package:unwrap_me/unwrap_me.dart' show Result, Ok, Err;

export 'src/api.dart' show DesktopShell;
export 'src/errors.dart';
export 'src/menu/menu.dart' show Menu, MenuItem;
