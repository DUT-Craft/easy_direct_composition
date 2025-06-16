#pragma once

/**
 * @file easy_direct_composition.h
 * @brief Easy Direct Composition Library - Main Public Header
 * @version 1.0.0
 * 
 * This library provides a simplified interface for Windows Direct Composition
 * with object tree management and surface rendering capabilities.
 * 
 * Usage:
 * ```cpp
 * #include "easy_direct_composition.h"
 * 
 * // Initialize the library
 * edc::initialize();
 * 
 * // Create environment
 * edc::Environment env(hInstance, edc::Object::get_root());
 * env.Initialize();
 * 
 * // Create visuals and surfaces
 * auto visual = env.makeVisual(env.getRootVisual());
 * auto surface = env.createSurfaceForVisual(visual, 100, 100);
 * 
 * // Render content
 * edc::SurfaceHelper helper(surface);
 * helper.addRect("rect1", {0, 0, 100, 100, {1.0f, 0.0f, 0.0f, 1.0f}});
 * helper.compile();
 * 
 * env.commit();
 * env.Run();
 * ```
 */

#ifndef EASY_DIRECT_COMPOSITION_H
#define EASY_DIRECT_COMPOSITION_H

// Include all public headers
#include "src/obj_tree.h"
#include "src/obj_helper.h"
#include "src/dc_env.h"
#include "src/dc_surface.h"

/**
 * @namespace edc
 * @brief Easy Direct Composition namespace
 * 
 * All library components are contained within this namespace to avoid
 * naming conflicts with other libraries.
 */
namespace edc {
    
    // Type aliases for convenience
    using Object = Easy_Object;
    using Environment = DC_Env;
    using SurfaceHelper = DC_Surface_Helper;
    using RectData = Rect_Data;
    
    /**
     * @brief Initialize the Easy Direct Composition library
     * 
     * This function must be called before using any other library functions.
     * It initializes the object system and type registry.
     * 
     */
    inline void initialize() {
        obj_init_key_map();
        Easy_Object::TypeSystemInit();
    }

    /**
     * @brief Clean up the Easy Direct Composition library
     * 
     * This function should be called when the library is no longer needed.
     * It releases all resources and performs necessary cleanup.
     * 
     */

    inline void cleanup() {
        //TODO
        Easy_Object::get_root().clear();
    }
    
    /**
     * @brief Get the library version
     * @return Version string in format "major.minor.patch"
     */
    inline const char* version() {
        return "1.0.0";
    }
    
    /**
     * @brief Check if the library is initialized
     * @return true if initialized, false otherwise
     */
    inline bool is_initialized() {
        return !Easy_Object::get_root().is_null();
    }
    
    /**
     * @brief Create a color from RGBA values
     * @param r Red component (0.0-1.0)
     * @param g Green component (0.0-1.0)
     * @param b Blue component (0.0-1.0)
     * @param a Alpha component (0.0-1.0)
     * @return D2D1_COLOR_F color structure
     */
    inline D2D1_COLOR_F make_color(float r, float g, float b, float a = 1.0f) {
        return D2D1::ColorF(r, g, b, a);
    }
    
    /**
     * @brief Create a rectangle data structure
     * @param x X coordinate
     * @param y Y coordinate
     * @param width Width
     * @param height Height
     * @param color Fill color
     * @return RectData structure
     */
    inline RectData make_rect(float x, float y, float width, float height, D2D1_COLOR_F color) {
        return {x, y, width, height, color};
    }
    
    /**
     * @brief Create a rectangle with color components
     * @param x X coordinate
     * @param y Y coordinate
     * @param width Width
     * @param height Height
     * @param r Red component (0.0-1.0)
     * @param g Green component (0.0-1.0)
     * @param b Blue component (0.0-1.0)
     * @param a Alpha component (0.0-1.0)
     * @return RectData structure
     */
    inline RectData make_rect(float x, float y, float width, float height, 
                             float r, float g, float b, float a = 1.0f) {
        return make_rect(x, y, width, height, make_color(r, g, b, a));
    }
}

#endif // EASY_DIRECT_COMPOSITION_H
