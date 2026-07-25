/// Base error type for all desktop_shell errors.
sealed class DesktopShellError {
  const DesktopShellError();
  String get message;
}

/// Failed to initialize tray icon.
final class TrayInitError extends DesktopShellError {
  final String details;
  const TrayInitError(this.details);

  @override
  String get message => 'Tray initialization failed: $details';
}

/// Failed to initialize window management.
final class WindowInitError extends DesktopShellError {
  final String details;
  const WindowInitError(this.details);

  @override
  String get message => 'Window initialization failed: $details';
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
  const TrayIconError(this.details);

  @override
  String get message => 'Failed to set tray icon: $details';
}

/// Failed to set tray menu.
final class TrayMenuError extends TrayError {
  final String details;
  const TrayMenuError(this.details);

  @override
  String get message => 'Failed to set tray menu: $details';
}

/// Failed to show tray context menu.
final class TrayPopupError extends TrayError {
  final String details;
  const TrayPopupError(this.details);

  @override
  String get message => 'Failed to show tray context menu: $details';
}

/// Window operation failed.
sealed class WindowOperationError extends DesktopShellError {
  const WindowOperationError();
}

/// Failed to show window.
final class WindowShowError extends WindowOperationError {
  final String details;
  const WindowShowError(this.details);

  @override
  String get message => 'Failed to show window: $details';
}

/// Failed to hide window.
final class WindowHideError extends WindowOperationError {
  final String details;
  const WindowHideError(this.details);

  @override
  String get message => 'Failed to hide window: $details';
}

/// Failed to focus window.
final class WindowFocusError extends WindowOperationError {
  final String details;
  const WindowFocusError(this.details);

  @override
  String get message => 'Failed to focus window: $details';
}

/// Failed to set prevent close.
final class WindowPreventCloseError extends WindowOperationError {
  final String details;
  const WindowPreventCloseError(this.details);

  @override
  String get message => 'Failed to set prevent close: $details';
}

/// Failed to destroy resources.
final class ShellDestroyError extends DesktopShellError {
  final String details;
  const ShellDestroyError(this.details);

  @override
  String get message => 'Failed to destroy resources: $details';
}
