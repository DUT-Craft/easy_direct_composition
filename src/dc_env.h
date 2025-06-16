/**
 * @file dc_env.h
 * @brief Direct Composition Environment
 * @version 1.0.0
 *
 * This module provides the main environment class for managing Direct Composition
 * devices, windows, and visual trees. It handles the initialization and management
 * of D3D11, D2D1, and DirectComposition resources.
 *
 * Features:
 * - Window creation and management
 * - D3D11 device initialization
 * - DirectComposition device and visual tree setup
 * - Message loop handling
 * - Visual and surface creation
 */

#pragma once
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <dcomp.h>
#include <windowsx.h>
#include <windows.h>
#include <atlbase.h>
#include <d2d1.h>

#include "obj_helper.h"


class DC_Env{
public:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    explicit DC_Env(HINSTANCE hInstance, Easy_Object root_obj);
    ~DC_Env();

    HRESULT Initialize();
    int Run();

    Easy_Object makeVisual(Easy_Object parent_visual);
    Easy_Object createSurfaceForVisual(Easy_Object visual, int width, int height);

    Easy_Object getRootVisual() {return m_dc_obj.get("root_visual");}

    void commit() {m_pDevice->Commit();}
private:


    void CreateObjectTree();

    // Initialize main window functions including layered child window
    HRESULT InitializeMainWindow();

    // Initialize Direct Composition and D3D functions
    HRESULT CreateD3D11Device();
    HRESULT CreateDCompositionDevice();
    HRESULT CreateDCompositionRenderTarget();
    HRESULT CreateDCompositionVisualTree();

    // Window message handlers
    LRESULT OnClose(HWND hwnd);
    LRESULT OnDestroy(HWND hwnd); 

    // Destroy
    VOID Destroy();
    VOID DestroyMainWindow();
    VOID DestroyDCompositionVisualTree();
    VOID DestroyDCompositionRenderTarget();
    VOID DestroyDCompositionDevice();
    VOID DestroyD3D11Device();

    int EnterMessageLoop();


    static DC_Env* s_application;
    HINSTANCE m_hInstance;
    Easy_Object m_root_obj, m_dc_obj;

    HWND  m_hMainWindow;         // Main window

    CComPtr<ID3D11Device> _d3d11Device;
    CComPtr<ID3D11DeviceContext> _d3d11DeviceContext;
    CComPtr<ID2D1Factory> m_d2dFactory;
    CComPtr<IDXGIDevice> m_dxgiDevice;
    CComPtr<ID2D1Device> m_d2dDevice;
    CComPtr<ID2D1DeviceContext> m_d2dContext;

    CComPtr<IDCompositionDevice> m_pDevice;
    CComPtr<IDCompositionTarget> m_pHwndRenderTarget;
    CComPtr<IDCompositionVisual> m_pRootVisual;

};