#pragma once
#include <dcomp.h>
#include <d2d1.h>
#include <atlbase.h>
#include <assert.h>
#include <string>
#include <atlcomcli.h>
#include "obj_helper.h"

typedef struct s_Rect_Data {
    float x, y, width, height;
    D2D1_COLOR_F color;
} Rect_Data;

typedef void (*Component_Draw_Function)(void* data, ID2D1DeviceContext* render_target, POINT offset);

class DC_Surface_Helper {
public:
    explicit DC_Surface_Helper(Easy_Object surface_obj) : m_surface_obj(surface_obj) {
        assert(surface_obj.get("data").has_COM_interface(__uuidof(IDCompositionSurface)));
        if (surface_obj.get("components").is_null()) {
            surface_obj.insert("components", Easy_Object::make_map());
        }
    }
    ~DC_Surface_Helper() {}

    void addRect(const std::string &name, Rect_Data rect_data);

    void compile();
private:
    Easy_Object m_surface_obj;
};