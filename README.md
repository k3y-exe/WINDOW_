### WINDOW_ – Single Header Cross-Platform Window Library

WINDOW_ is a minimal C89 single header library for creating windows on Windows, Linux (X11), and macOS (Cocoa). It provides a simple API to create, poll, and destroy windows with a clean naming convention: WINDOW_ namespace, PascalCase functions, camelCase struct members, and p_ prefix for parameters.

To use, include window.h in your project and call WINDOW_WindowCreate to open a window. Use `WINDOW_WindowPoll` to process events, `WINDOW_WindowShouldClose` to check if the window should close, and `WINDOW_WindowDestroy` to clean up. The API is simple and minimal, making it suitable for small games, demos, or GUI experiments.

On Linux, build with `cc main.c -o app -lX11`. On Windows (MinGW), use `gcc main.c -o app.exe -lgdi32 -luser32 -mwindows` to avoid opening a console window. On macOS, compile with `cc main.c -o app -framework` Cocoa. Linux requires X11 development headers such as libx11-dev.

Currently, WINDOW_ supports window creation, resizing, and basic event polling. Future improvements may include keyboard and mouse input handling, callback support, and OpenGL/Vulkan integration. The library is licensed under the MIT License, allowing free use, modification, and distribution.
