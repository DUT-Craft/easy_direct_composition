# Easy Direct Composition Library

A simplified C++ library for Windows Direct Composition with object tree management and surface rendering capabilities.

## Features

- **Object Tree System**: C-based object tree with reference counting and attribute management
- **Direct Composition Integration**: Simplified interface for Windows Direct Composition
- **Surface Rendering**: Easy-to-use surface helpers for drawing graphics
- **COM Integration**: Seamless integration with COM objects
- **Type System**: Extensible type system with custom destructors
- **Memory Management**: Automatic reference counting and RAII

## Requirements

- Windows 10 or later
- Visual Studio 2019 or later
- CMake 3.10 or later
- C++20 support

## Building

### As a Git Submodule

This library is designed to be used as a git submodule:

```bash
git submodule add <repository-url> easy_direct_composition
git submodule update --init --recursive
```

### CMake Integration

Add to your CMakeLists.txt:

```cmake
# Add the submodule
add_subdirectory(easy_direct_composition)

# Link to your target
target_link_libraries(your_target
    PRIVATE
        EasyDirectComposition::easy_direct_composition
)
```

### Manual Build

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Usage

### Basic Example

```cpp
#include "easy_direct_composition/easy_direct_composition.h"

INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, INT) {
    // Initialize the library
    edc::initialize();

    // Create environment
    edc::Environment env(hInstance, edc::Object::get_root());
    if (FAILED(env.Initialize())) {
        return -1;
    }

    // Create visual and surface
    auto visual = env.makeVisual(env.getRootVisual());
    auto surface = env.createSurfaceForVisual(visual, 200, 200);

    // Add graphics content
    edc::SurfaceHelper helper(surface);
    helper.addRect("background", edc::make_rect(0, 0, 200, 200, 0.2f, 0.3f, 0.8f));
    helper.addRect("foreground", edc::make_rect(50, 50, 100, 100, 1.0f, 0.5f, 0.0f));
    helper.compile();

    // Commit and run
    env.commit();
    return env.Run();
}
```

### Object System

The library provides a powerful object system:

```cpp
// Create objects
auto map_obj = edc::Object::make_map();
auto array_obj = edc::Object::make_array();

// Map operations
map_obj.insert("key1", edc::Object::make_char32_string(U"value1"));
auto value = map_obj.get("key1");

// Array operations
array_obj.push_back(edc::Object::make_char32_string(U"item1"));
auto item = array_obj.get(0);
```

### Surface Rendering

```cpp
edc::SurfaceHelper helper(surface);

// Add rectangles
helper.addRect("rect1", edc::make_rect(10, 10, 80, 80, 1.0f, 0.0f, 0.0f));

// Custom drawing (advanced)
helper.addCustomComponent("custom", my_draw_function, my_data);

// Render all components
helper.compile();
```

## API Reference

### Core Classes

- `edc::Object`: Main object wrapper with reference counting
- `edc::Environment`: Direct Composition environment manager
- `edc::SurfaceHelper`: Surface rendering helper

### Utility Functions

- `edc::initialize()`: Initialize the library
- `edc::make_color()`: Create color from RGBA values
- `edc::make_rect()`: Create rectangle data structure

## License

See LICENSE file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## Version History

- **1.0.0**: Initial release with basic Direct Composition support
