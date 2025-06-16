/**
 * @file basic_example.cpp
 * @brief Basic usage example for Easy Direct Composition Library
 * 
 * This example demonstrates how to:
 * - Initialize the library
 * - Create a Direct Composition environment
 * - Create visuals and surfaces
 * - Render basic graphics (rectangles)
 * - Run the message loop
 */

#include "../easy_direct_composition.h"
#include <windows.h>
#include <cassert>

/**
 * @brief Application entry point
 * 
 * Creates a simple window with two colored rectangles using Direct Composition.
 */
INT WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ INT)
{
    // Initialize the Easy Direct Composition library
    // This must be called before using any other library functions
    edc::initialize();
    
    // Verify initialization
    if (!edc::is_initialized()) {
        MessageBoxA(nullptr, "Failed to initialize Easy Direct Composition library", "Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    
    // Create the Direct Composition environment
    // This sets up the window, D3D11 device, and DirectComposition device
    edc::Environment dc_env(hInstance, edc::Object::get_root());
    
    // Initialize the environment
    HRESULT hr = dc_env.Initialize();
    if (FAILED(hr)) {
        MessageBoxA(nullptr, "Failed to initialize Direct Composition environment", "Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    
    // Create a visual object
    // Visuals are the building blocks of the composition tree
    edc::Object visual1 = dc_env.makeVisual(dc_env.getRootVisual());
    
    // Create a surface for the visual
    // Surfaces are where we can draw graphics content
    edc::Object surface1 = dc_env.createSurfaceForVisual(visual1, 200, 200);
    
    // Create a surface helper to manage drawing operations
    edc::SurfaceHelper surface_helper(surface1);
    
    // Add a background rectangle (blue with some transparency)
    edc::RectData background = edc::make_rect(
        0, 0,           // x, y position
        200, 200,       // width, height
        0.2f, 0.3f, 0.8f, 0.8f  // r, g, b, a (blue with 80% opacity)
    );
    surface_helper.addRect("background", background);
    
    // Add a foreground rectangle (orange, fully opaque)
    edc::RectData foreground = edc::make_rect(
        50, 50,         // x, y position
        100, 100,       // width, height
        1.0f, 0.5f, 0.0f, 1.0f  // r, g, b, a (orange, fully opaque)
    );
    surface_helper.addRect("foreground", foreground);
    
    // Add a small accent rectangle (green)
    edc::RectData accent = edc::make_rect(
        75, 75,         // x, y position
        50, 50,         // width, height
        0.0f, 1.0f, 0.0f, 0.9f  // r, g, b, a (green with 90% opacity)
    );
    surface_helper.addRect("accent", accent);
    
    // Compile all the drawing operations
    // This renders all added components to the surface
    surface_helper.compile();
    
    // Commit the changes to the composition
    // This makes the visual changes visible
    dc_env.commit();
    
    // Run the message loop
    // This keeps the application running and handles window messages
    return dc_env.Run();
}

/**
 * @brief Alternative example using the convenience functions
 * 
 * This shows a more concise way to create the same visual result.
 */
INT WINAPI AlternativeExample(HINSTANCE hInstance)
{
    edc::initialize();
    
    edc::Environment env(hInstance, edc::Object::get_root());
    assert(SUCCEEDED(env.Initialize()));
    
    auto visual = env.makeVisual(env.getRootVisual());
    auto surface = env.createSurfaceForVisual(visual, 200, 200);
    
    edc::SurfaceHelper helper(surface);
    
    // Using the convenience color creation function
    helper.addRect("bg", {0, 0, 200, 200, edc::make_color(0.2f, 0.3f, 0.8f, 0.8f)});
    helper.addRect("fg", {50, 50, 100, 100, edc::make_color(1.0f, 0.5f, 0.0f)});
    helper.addRect("accent", {75, 75, 50, 50, edc::make_color(0.0f, 1.0f, 0.0f, 0.9f)});
    
    helper.compile();
    env.commit();
    
    return env.Run();
}
