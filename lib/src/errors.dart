import 'package:unwrap_me/unwrap_me.dart';

/// Base error type for all desktop_shell errors.
sealed class DesktopShellError {
  const DesktopShellError();
  String get message;
}

/// Failed to initialize tray icon.
final class TrayInitError extends DesktopShellError {
  final String details;
  final Option<String> code;

  const TrayInitError(
    this.details, {
    this.code = const None(),
  });

  @override
  String get message => '[$code] Tray initialization failed: $details';
}

/// Failed to initialize window management.
final class WindowInitError extends DesktopShellError {
  final String details;
  final Option<String> code;

  const WindowInitError(
    this.details, {
    this.code = const None(),
  });

  @override
  String get message => '[$code] Window initialization failed: $details';
}

/// Platform is not supported.
final class UnsupportedPlatformError extends DesktopShellError {
  final String platform;

  const UnsupportedPlatformError(this.platform);

  @override
  String get message => 'Platform not supported: $platform';
}

/// Tray operation failed.
sealed class TrayError extends DesktopShellError {
  const TrayError();
}

/// Failed to set tray icon.
final class TrayIconError extends TrayError {
  final String details;
  final Option<String> code;

  const TrayIconError(
    this.details, {
    this.code = const None(),
  });

  @override
  String get message => '[$code] Failed to set tray icon: $details';
}

/// Failed to set tray menu.
final class TrayMenuError extends TrayError {
  final String details;
  final Option<String> code;

  const TrayMenuError(
    this.details, {
    this.code = const None(),
  });

  @override
  String get message => '[$code] Failed to set tray menu: $details';
}

/// Failed to show tray context menu.
final class TrayPopupError extends TrayError {
  final String details;
  final Option<String> code;

  const TrayPopupError(
    this.details, {
    this.code = const None(),
  });

  @override
  String get message => '[$code] Failed to show tray context menu: $details';
}

/// Window operation failed.
sealed class WindowOperationError extends DesktopShellError {
  const WindowOperationError();
}

/// Failed to show window.
final class WindowShowError extends WindowOperationError {
  final String details;
  final Option<String> code;

  const WindowShowError(
    this.details, {
    this.code = const None(),
  });

  @override
  String get message => '[$code] Failed to show window: $details';
}

/// Failed to hide window.
final class WindowHideError extends WindowOperationError {
  final String details;
  final Option<String> code;

  const WindowHideError(
    this.details, {
    this.code = const None(),
  });

  @override
  String get message => '[$code] Failed to hide window: $details';
}

/// Failed to focus window.
final class WindowFocusError extends WindowOperationError {
  final String details;
  final Option<String> code;

  const WindowFocusError(
    this.details, {
    this.code = const None(),
  });

  @override
  String get message => '[$code] Failed to focus window: $details';
}

/// Failed to set prevent close.
final class WindowPreventCloseError extends WindowOperationError {
  final String details;
  final Option<String> code;

  const WindowPreventCloseError(
    this.details, {
    this.code = const None(),
  });

  @override
  String get message => '[$code] Failed to set prevent close: $details';
}

/// Failed to destroy resources.
final class ShellDestroyError extends DesktopShellError {
  final String details;
  final Option<String> code;

  const ShellDestroyError(
    this.details, {
    this.code = const None(),
  });

  @override
  String get message => '[$code] Failed to destroy resources: $details';
}
