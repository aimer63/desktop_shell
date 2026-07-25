import 'dart:async';

import 'package:flutter/services.dart';

/// The single platform channel for all desktop shell operations.
const MethodChannel desktopShellChannel = MethodChannel('desktop_shell');

/// Set up method call handler from native code.
void setMethodCallHandler(Future<dynamic> Function(MethodCall call)? handler) {
  desktopShellChannel.setMethodCallHandler(handler);
}

/// Invoke a method on the native side.
Future<T?> invokeMethod<T>(String method, [dynamic arguments]) async {
  return await desktopShellChannel.invokeMethod<T>(method, arguments);
}

/// Invoke a method and return raw result with error handling.
Future<Map<String, dynamic>?> invokeMethodWithResult(
  String method, [
  dynamic arguments,
]) async {
  try {
    final result = await desktopShellChannel.invokeMethod<dynamic>(
      method,
      arguments,
    );
    if (result == null) {
      return null;
    }
    // Handle both Map and bool responses from native
    if (result is Map) {
      return Map<String, dynamic>.from(result);
    }
    // Success responses return true (bool) from native
    if (result == true) {
      return null;
    }
    return {'error': true, 'message': 'Unexpected result: $result'};
  } on PlatformException catch (e) {
    return {
      'error': true,
      'code': e.code,
      'message': e.message ?? 'Unknown error',
      'details': e.details,
    };
  } catch (e) {
    return {
      'error': true,
      'code': 'UNKNOWN',
      'message': e.toString(),
    };
  }
}
