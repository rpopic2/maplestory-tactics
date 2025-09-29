// nexon-tutorial-2025-maplestory.cpp : Defines the entry point for the application.
//
#define WIN32_LEAN_AND_MEAN
#include "framework.h"
#include "nexon-tutorial-2025-maplestory.h"
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <iostream>
#include <cassert>
#include <wrl.h>
#include <dxgi1_3.h>
#include <SpriteBatch.h>
#include <WICTextureLoader.h>


#pragma comment( lib, "user32" )          // link against the win32 library
#pragma comment( lib, "d3d11.lib" )       // direct3D library
#pragma comment( lib, "dxgi.lib" )        // directx graphics interface
#pragma comment( lib, "d3dcompiler.lib" ) // shader compiler

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

ID3D11Device *device_ptr = nullptr;
ID3D11DeviceContext *device_context_ptr = nullptr;
IDXGISwapChain *swap_chain_ptr = nullptr;
ID3D11RenderTargetView *render_target_view_ptr = nullptr;
DXGI_SWAP_CHAIN_DESC swap_chain_descr{};

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    // Register the window class.
    const wchar_t CLASS_NAME[] = L"Sample Window Class";

    WNDCLASS wc = { };

    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClass(&wc))
    {
        DWORD dwError = GetLastError();
        if (dwError != ERROR_CLASS_ALREADY_EXISTS)
            return HRESULT_FROM_WIN32(dwError);
    }

    // Create the window.

    int x = CW_USEDEFAULT;
    int y = CW_USEDEFAULT;

    RECT m_rc;
    UINT dpi = GetDpiForSystem();
    float scale = dpi / 96.0f;
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    SetRect(&m_rc, 0, 0, 640 * scale, 480 * scale);
    AdjustWindowRect(&m_rc, WS_OVERLAPPEDWINDOW, false);

    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        L"maple tactics",               // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, (m_rc.right - m_rc.left), (m_rc.bottom - m_rc.top),

        NULL,       // Parent window
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    if (hwnd == NULL)
    {
        DWORD dwError = GetLastError();
        return HRESULT_FROM_WIN32(dwError);
    }

    ShowWindow(hwnd, nCmdShow);

    // Create D11 Device and Context

    D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1,
    };
    UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(DEBUG) || defined(_DEBUG)
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL featureLevel;
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    HRESULT hr = D3D11CreateDevice(
        nullptr, // default adapter
        D3D_DRIVER_TYPE_HARDWARE, // hardware graphics driver
        0, // 0 if hardware
        deviceFlags, // set debug and Direct2D compat flags
        levels,
        ARRAYSIZE(levels),
        D3D11_SDK_VERSION,
        &device,
        &featureLevel,
        &context
    );
    if (FAILED(hr))
    {
        // handle fail!
    }

    // swap chain

    DXGI_SWAP_CHAIN_DESC desc{};
    desc.Windowed = TRUE;
    desc.BufferCount = 2;
    desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.OutputWindow = hwnd;

    Microsoft::WRL::ComPtr<IDXGIDevice3> dxgiDevice;
    device.As(&dxgiDevice);

    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    Microsoft::WRL::ComPtr<IDXGIFactory> factory;

    hr = dxgiDevice->GetAdapter(&adapter);
    IDXGISwapChain *swapChain = nullptr;
    if (SUCCEEDED(hr))
    {
        adapter->GetParent(IID_PPV_ARGS(&factory));
        hr = factory->CreateSwapChain(
            device.Get(),
            &desc,
            &swapChain
        );
    }

    // create render target

    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTarget;
    hr = swapChain->GetBuffer(
        0,
        __uuidof(ID3D11Texture2D),
        (void **)&backBuffer
    );
    hr = device->CreateRenderTargetView(
        backBuffer.Get(),
        nullptr,
        renderTarget.GetAddressOf()
    );
    D3D11_TEXTURE2D_DESC bbDesc;
    backBuffer->GetDesc(&bbDesc);

    Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencil;
    CD3D11_TEXTURE2D_DESC depthStencilDesc(
        DXGI_FORMAT_D24_UNORM_S8_UINT,
        static_cast<UINT>(bbDesc.Width),
        static_cast<UINT>(bbDesc.Height),
        1, // has only texture
        1, // use a single mipmap level
        D3D11_BIND_DEPTH_STENCIL
    );
    device->CreateTexture2D(
        &depthStencilDesc,
        nullptr,
        &depthStencil
    );

    D3D11_VIEWPORT viewport{};
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
    viewport.Height = (float)bbDesc.Height;
    viewport.Width = (float)bbDesc.Width;
    viewport.MinDepth = 0;
    viewport.MaxDepth = 1;
    context->RSSetViewports(
        1,
        &viewport
    );

    const float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    // load texture
    auto spriteBatch = std::make_unique<DirectX::SpriteBatch>(context.Get());
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> myTexture;
    Microsoft::WRL::ComPtr<ID3D11Resource> tex;
    hr = DirectX::CreateWICTextureFromFile(device.Get(), L"vendor_sprites/0100100.png", nullptr, myTexture.GetAddressOf());
    if (FAILED(hr))
    {
        DWORD dwError = GetLastError();
        return HRESULT_FROM_WIN32(dwError);
    }
    Microsoft::WRL::ComPtr<ID3D11SamplerState> pointSampler;
    D3D11_SAMPLER_DESC samplerDesc{};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    // Run the message loop.

    MSG msg{ };
    bool bGotMsg = false;
    PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);

    DirectX::XMFLOAT2 characterPos = { 100.0f, 100.0f };
    while (msg.message != WM_QUIT)
    {
        bGotMsg = PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0;
        if (bGotMsg)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            context->OMSetRenderTargets(1, renderTarget.GetAddressOf(), nullptr);

            context->ClearRenderTargetView(renderTarget.Get(), clearColor);
            spriteBatch->Begin(DirectX::SpriteSortMode_Deferred, nullptr, pointSampler.Get());
            spriteBatch->Draw(myTexture.Get(), characterPos);
            spriteBatch->End();
            swapChain->Present(1, 0);
            if (GetAsyncKeyState(VK_RIGHT))
                characterPos.x += 1.0f;
            if (GetAsyncKeyState(VK_LEFT))
                characterPos.x -= 1.0f;
            if (GetAsyncKeyState(VK_LCONTROL))
                characterPos.y -= 1.0f;

        }
    }

    return 0;

}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_NEXONTUTORIAL2025MAPLESTORY));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_NEXONTUTORIAL2025MAPLESTORY);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
