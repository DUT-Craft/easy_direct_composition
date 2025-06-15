
#include <string>

#include "dc_env.h"

DC_Env* DC_Env::s_application;

DC_Env::DC_Env(HINSTANCE hInstance, Easy_Object root_obj)
    : m_hInstance(hInstance), m_root_obj(root_obj), m_hMainWindow(0)
{
    s_application = this;
}

DC_Env::~DC_Env()
{
    Destroy();
}

int DC_Env::Run()
{
    int result = 0;

    result = EnterMessageLoop();

    Destroy();

    return result;
}

Easy_Object DC_Env::makeVisual(Easy_Object parent_visual)
{
    Easy_Object parent_data = parent_visual.get("data");
    CComPtr<IDCompositionVisual> visual;
    m_pDevice->CreateVisual(&visual);
    Easy_Object visual_obj_data = Easy_Object::pack_COM_object(visual);
    Easy_Object ret = Easy_Object::make_map();
    ret.insert("data", visual_obj_data);
    ret.insert("childs", Easy_Object::make_array());
    if (!parent_data.is_null()) {
        CComPtr<IDCompositionVisual> parent_visual_obj;
        parent_data.get_COM_interface(parent_visual_obj);
        parent_visual_obj->AddVisual(visual, FALSE, NULL);
        Easy_Object parent_childs = parent_visual.get("childs");
        parent_childs.push_back(ret);
    }
    return ret;
}

Easy_Object DC_Env::createSurfaceForVisual(Easy_Object visual, int width, int height)
{
    Easy_Object visual_data = visual.get("data");
    if (visual_data.is_null()) return Easy_Object();
    if (!visual.get("surface").is_null()) return visual.get("surface");
    CComPtr<IDCompositionVisual> visual_obj;
    visual_data.get_COM_interface(visual_obj);
    CComPtr<IDCompositionSurface> surface;
    HRESULT hr = m_pDevice->CreateSurface(width, height, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ALPHA_MODE_PREMULTIPLIED, &surface);
    if (FAILED(hr)) return Easy_Object();
    visual_obj->SetContent(surface);
    Easy_Object ret = Easy_Object::make_map();
    ret.insert("data", Easy_Object::pack_COM_object(surface));
    visual.insert("surface", ret);
    Easy_Object context_data = Easy_Object::pack_COM_object(m_d2dContext);
    ret.insert("context", context_data);
    return ret;
}

int DC_Env::EnterMessageLoop()
{
    int result = 0;

    MSG msg = { 0 };

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    result = static_cast<int>(msg.wParam);

    return result;
}



HRESULT DC_Env::Initialize()
{
    HRESULT hr = InitializeMainWindow();

    if (SUCCEEDED(hr))
    {
        CreateObjectTree();
    }

    if (SUCCEEDED(hr))
    {
        hr = CreateD3D11Device();
    }

    if (SUCCEEDED(hr))
    {
        hr = CreateDCompositionDevice();
    }

    if (SUCCEEDED(hr))
    {
        hr = CreateDCompositionRenderTarget();
    }

    if (SUCCEEDED(hr))
    {
        hr = CreateDCompositionVisualTree();
    }

    if (SUCCEEDED(hr))
    {
        // Commit the batch.
        hr = m_pDevice->Commit();
    }

    return hr;

}

void DC_Env::CreateObjectTree()
{
    m_dc_obj = Easy_Object::make_map();
    m_root_obj.insert("DirectComposition", m_dc_obj);

}

// Creates the main application window.
HRESULT DC_Env::InitializeMainWindow()
{
    HRESULT hr = S_OK;

    // Register the window class.
    WNDCLASSEXW wc     = {0};
    wc.cbSize         = sizeof(wc);
    wc.style          = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc    = WindowProc;
    wc.hInstance      = m_hInstance;
    wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground  = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszClassName  = L"GerritClientWindowClass";

    RegisterClassExW(&wc);

    // Creates the m_hMainWindow window.
    m_hMainWindow = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW | WS_EX_NOREDIRECTIONBITMAP,                          // Extended window style
                                   wc.lpszClassName,                                // Name of window class
                                   L"Client", // Title-bar string
                                   WS_OVERLAPPED | WS_SYSMENU,                      // Top-level window
                                   CW_USEDEFAULT,                                   // Horizontal position
                                   CW_USEDEFAULT,                                   // Vertical position
                                   1000,                                            // Width
                                   700,                                             // Height
                                   NULL,                                            // Parent
                                   NULL,                                            // Class menu
                                   GetModuleHandle(NULL),                           // Handle to application instance
                                   NULL                                             // Window-creation data
                                   );

    if (!m_hMainWindow)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if (SUCCEEDED(hr))
    {
        ShowWindow(m_hMainWindow, SW_SHOWDEFAULT);
        wchar_t buffer[256];
        GetWindowTextW(m_hMainWindow, buffer, 256);
        OutputDebugStringW(L"Current window title: ");
        OutputDebugStringW(buffer);
        OutputDebugStringW(L"\n");
    }

    return hr;
}

HRESULT DC_Env::CreateD3D11Device()
{
    HRESULT hr = S_OK;

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
    };

    D3D_FEATURE_LEVEL featureLevelSupported;

    for (int i = 0; i < sizeof(driverTypes) / sizeof(driverTypes[0]); ++i)
    {
        CComPtr<ID3D11Device> d3d11Device;
        CComPtr<ID3D11DeviceContext> d3d11DeviceContext;

        hr = D3D11CreateDevice(
            nullptr,
            driverTypes[i],
            NULL,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            NULL,
            0,
            D3D11_SDK_VERSION,
            &d3d11Device,
            &featureLevelSupported,
            &d3d11DeviceContext);

        if (SUCCEEDED(hr))
        {
            _d3d11Device = d3d11Device.Detach();
            _d3d11DeviceContext = d3d11DeviceContext.Detach();

            break;
        }
    }
    if (SUCCEEDED(hr))
    {
        D2D1_FACTORY_OPTIONS options = {};
        hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), &options, reinterpret_cast<void**>(&m_d2dFactory));
    }

    if (SUCCEEDED(hr))
    {
        hr = _d3d11Device->QueryInterface(&m_dxgiDevice);
    }

    if (SUCCEEDED(hr))
    {
        hr = D2D1CreateDevice(m_dxgiDevice, NULL, &m_d2dDevice);
    }

    if (SUCCEEDED(hr))
    {
        hr = m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_d2dContext);
    }

    return hr;
}

HRESULT DC_Env::CreateDCompositionDevice()
{
    HRESULT hr = (_d3d11Device == nullptr) ? E_UNEXPECTED : S_OK;

    if (SUCCEEDED(hr))
    {
        hr = DCompositionCreateDevice(m_dxgiDevice, __uuidof(IDCompositionDevice), reinterpret_cast<void **>(&m_pDevice));
    }

    return hr;
}

HRESULT DC_Env::CreateDCompositionRenderTarget()
{
    HRESULT hr = ((m_pDevice == nullptr) || (m_hMainWindow == NULL)) ? E_UNEXPECTED : S_OK;

    if (SUCCEEDED(hr))
    {
        // FALSE puts the composition content beneath the Win32 buttons.
        hr = m_pDevice->CreateTargetForHwnd(m_hMainWindow, FALSE, &m_pHwndRenderTarget);
    }

    return hr;
}

HRESULT DC_Env::CreateDCompositionVisualTree()
{
    HRESULT hr = ((m_pDevice == nullptr) || (m_hMainWindow == NULL)) ? E_UNEXPECTED : S_OK;

    if (SUCCEEDED(hr))
    {
        // Create the root visual.
        hr = m_pDevice->CreateVisual(&m_pRootVisual);
    }

    if (SUCCEEDED(hr))
    {
        // Make the visual the root of the tree.
        hr = m_pHwndRenderTarget->SetRoot(m_pRootVisual);
    }

    if (SUCCEEDED(hr))
    {
        Easy_Object root_visual = Easy_Object::make_map();
        root_visual.insert("data", Easy_Object::pack_COM_object(m_pRootVisual));
        root_visual.insert("childs", Easy_Object::make_array());
        m_dc_obj.insert("root_visual", root_visual);
    }

    return hr;
}

//------------------------------------------------------
// In Action
//------------------------------------------------------

// Main window procedure
LRESULT CALLBACK DC_Env::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch (uMsg)
    {
        case WM_COMMAND:
            break;

        case WM_RBUTTONUP:
            break;

        case WM_TIMER:
            break;

        case WM_PAINT:
            break;

        case WM_CLOSE:
            result = s_application->OnClose(hwnd);
            break;

        case WM_DESTROY:
            result = s_application->OnDestroy(hwnd);
            break;

        default:
            result = DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return result;
}

// Handles the WM_CLOSE message.
LRESULT DC_Env::OnClose(HWND /*hwnd*/)
{
    // Destroy the main window.
    DestroyWindow(m_hMainWindow);

    return 0;
}

// Handles the WM_DESTROY message.
LRESULT DC_Env::OnDestroy(HWND /*hwnd*/)
{
    PostQuitMessage(0);

    return 0;
}


//------------------------------------------------------
// Destroy
//------------------------------------------------------

VOID DC_Env::Destroy()
{
    DestroyMainWindow();
    DestroyDCompositionVisualTree();
    DestroyDCompositionRenderTarget();
    DestroyDCompositionDevice();
    DestroyD3D11Device();
    CoUninitialize();
}

VOID DC_Env::DestroyMainWindow()
{
    if (m_hMainWindow != NULL)
    {
       DestroyWindow(m_hMainWindow);
       m_hMainWindow = NULL;
    }
}


VOID DC_Env::DestroyDCompositionVisualTree()
{
}

VOID DC_Env::DestroyDCompositionRenderTarget()
{
    m_pHwndRenderTarget = nullptr;
}

VOID DC_Env::DestroyDCompositionDevice()
{
    m_pDevice = nullptr;
}

VOID DC_Env::DestroyD3D11Device()
{
    _d3d11DeviceContext = nullptr;
    _d3d11Device = nullptr;
}
