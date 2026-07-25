// swift-tools-version:5.3
import PackageDescription

let package = Package(
    name: "desktop_shell",
    platforms: [
        .macOS("10.14")
    ],
    products: [
        .library(
            name: "desktop_shell",
            targets: ["desktop_shell"]
        ),
    ],
    dependencies: [],
    targets: [
        .target(
            name: "desktop_shell",
            dependencies: []
        ),
    ]
)
