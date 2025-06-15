#include "dc_surface.h"
#include <atlbase.h>


static void draw_rect(void* data, ID2D1DeviceContext* render_target, POINT offset)
{
    Rect_Data *rect_data = (Rect_Data*)data;
    ID2D1SolidColorBrush *brush;
    render_target->CreateSolidColorBrush(rect_data->color, &brush);
    render_target->FillRectangle(D2D1::RectF(rect_data->x, rect_data->y, rect_data->x + rect_data->width, rect_data->y + rect_data->height), brush);
}

void DC_Surface_Helper::addRect(const std::string &name, Rect_Data rect_data)
{
    Easy_Object rect_obj = Easy_Object::make_map();
    m_surface_obj.get("components").insert(name, rect_obj);
    rect_obj.insert("type", Easy_Object::make_char32_string(U"rect"));
    Easy_Object rect_data_obj = Easy_Object::make_raw(&rect_data, sizeof(Rect_Data), alignof(Rect_Data));
    rect_obj.insert("data", rect_data_obj);
    Component_Draw_Function draw_func = draw_rect;
    rect_obj.insert("draw_func", Easy_Object::make_raw(&draw_func, sizeof(Component_Draw_Function*), alignof(Component_Draw_Function*)));
}

void DC_Surface_Helper::compile()
{
    Easy_Object surface_data = m_surface_obj.get("data");
    CComPtr<ID2D1DeviceContext> d2dContext;
    m_surface_obj.get("context").get_COM_interface(d2dContext);
    CComPtr<IDCompositionSurface> surface;
    surface_data.get_COM_interface(surface);

    CComPtr<IDXGISurface> dxgiSurface;
    POINT offset = {0, 0};
    HRESULT hr = surface->BeginDraw(NULL, IID_PPV_ARGS(&dxgiSurface), &offset);
    if (FAILED(hr)) return;

    // 1. 获取表面描述
    DXGI_SURFACE_DESC surfaceDesc;
    dxgiSurface->GetDesc(&surfaceDesc);

    // 2. 创建正确的位图属性
    float dpiX, dpiY;
    d2dContext->GetDpi(&dpiX, &dpiY);

    D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(
            surfaceDesc.Format,  // 使用表面实际格式
            D2D1_ALPHA_MODE_PREMULTIPLIED),  // 表面格式是DXGI_FORMAT_B8G8R8A8_UNORM
        dpiX,
        dpiY
    );

    // 3. 创建位图
    CComPtr<ID2D1Bitmap1> d2dTargetBitmap;
    hr = d2dContext->CreateBitmapFromDxgiSurface(
        dxgiSurface, 
        &bitmapProperties, 
        &d2dTargetBitmap
    );
    if (FAILED(hr)) return;
    d2dContext->SetTarget(d2dTargetBitmap);

    d2dContext->BeginDraw();
    d2dContext->Clear(D2D1::ColorF(0, 0, 0, 0));
    Easy_Object components = m_surface_obj.get("components");
    Map_Data *map_data = components.get_map_data();
    for (auto &pair : *map_data) {
        Easy_Object component = pair.second;
        Component_Draw_Function draw_func = *((Component_Draw_Function*)component.get("draw_func").get_data_ptr());
        void *data = component.get("data").get_data_ptr();
        draw_func(data, d2dContext, offset);
    }
    d2dContext->EndDraw();
    surface->EndDraw();
}
