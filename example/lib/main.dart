import 'dart:io';

import 'package:desktop_shell/desktop_shell.dart';
import 'package:flutter/material.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  final result = await DesktopShell.initialize(
    trayIcon: Platform.isWindows
        ? 'assets/app_icon.ico'
        : 'assets/app_icon.png',
    trayItems: [
      MenuItem(key: 'show', label: 'Show Window'),
      MenuItem.separator(),
      MenuItem(key: 'quit', label: 'Quit'),
    ],
    onWindowClose: (s) async {
      // Hide window to tray instead of closing
      await s.hideWindow();
    },
    onTrayIconClick: (s) async {
      // On Windows/macOS: this shows the context menu
      // On Linux: menu appears automatically on tray icon click, this does nothing
      await s.popUpTrayMenu();
    },
    onTrayMenuItemClick: (s, item) async {
      switch (item.key) {
        case 'show':
          await s.showWindow();
          await s.focusWindow();
        case 'quit':
          await s.destroy();
          exit(0);
      }
    },
  );

  switch (result) {
    case Ok(value: final shell):
      await shell.setPreventClose(true);
      runApp(MyApp(shell: shell));
    case Err(:final error):
      stderr.writeln('Failed to initialize: ${error.message}');
      exit(1);
  }
}

class MyApp extends StatelessWidget {
  final DesktopShell shell;

  const MyApp({super.key, required this.shell});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'desktop_shell Example',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.blue),
        useMaterial3: true,
      ),
      home: MyHomePage(shell: shell),
    );
  }
}

class MyHomePage extends StatefulWidget {
  final DesktopShell shell;

  const MyHomePage({super.key, required this.shell});

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  String _status = 'Window is visible';

  Future<void> _hideWindow() async {
    final result = await widget.shell.hideWindow();
    result.map((_) {
      setState(() => _status = 'Window is hidden (check tray)');
    });
  }

  Future<void> _showDot() async {
    final result = await widget.shell.setTrayIcon(
      Platform.isWindows ? 'assets/app_icon_dot.ico' : 'assets/app_icon_dot.png',
    );
    result.map((_) {
      setState(() => _status = 'Dot shown on tray icon');
    });
  }

  Future<void> _removeDot() async {
    final result = await widget.shell.setTrayIcon(
      Platform.isWindows ? 'assets/app_icon.ico' : 'assets/app_icon.png',
    );
    result.map((_) {
      setState(() => _status = 'Dot removed from tray icon');
    });
  }

  Future<void> _quit() async {
    final result = await widget.shell.destroy();
    result.map((_) => exit(0));
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('desktop_shell Example'),
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: <Widget>[
            Text(_status, style: Theme.of(context).textTheme.headlineSmall),
            const SizedBox(height: 32),
            ElevatedButton(
              onPressed: _hideWindow,
              child: const Text('Hide to Tray'),
            ),
            const SizedBox(height: 16),
            ElevatedButton(
              onPressed: _showDot,
              child: const Text('Show Dot'),
            ),
            const SizedBox(height: 16),
            ElevatedButton(
              onPressed: _removeDot,
              child: const Text('Remove Dot'),
            ),
            const SizedBox(height: 16),
            ElevatedButton(
              onPressed: _quit,
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.red,
                foregroundColor: Colors.white,
              ),
              child: const Text('Quit'),
            ),
            const SizedBox(height: 32),
            const Padding(
              padding: EdgeInsets.all(16.0),
              child: Text(
                'Try clicking the X button - it should hide to tray instead of closing!',
                textAlign: TextAlign.center,
                style: TextStyle(color: Colors.grey),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
