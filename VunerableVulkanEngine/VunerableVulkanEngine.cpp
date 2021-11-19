// VunerableVulkanEngine.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "VunerableVulkanEngine.h"
#include "Graphics/VulkanGraphics.h"
#include "IMGUI/imgui_impl_win32.h"
#include "ECS/Domain.h"

struct TestComponent : ECS::ComponentBase
{
    int m_Value;

    TestComponent()
        : m_Value(0)
    {        
    }

    TestComponent(int value)
        : m_Value(value)
    {
    }
};

struct TestAnotherComponent : ECS::ComponentBase
{
    float m_Value;

    TestAnotherComponent()
        : m_Value(0.0f)
    {
    }

    TestAnotherComponent(float value)
        : m_Value(value)
    {
    }
};

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

VulkanGraphics* gGraphicsPtr = NULL;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_VUNERABLEVULKANENGINE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_VUNERABLEVULKANENGINE));

    MSG msg;

    // [DUMMY CODE] TEST MY ECS
    {
        ECS::ComponentTypeUtility::RegisterComponentType<TestComponent>();
        ECS::ComponentTypeUtility::RegisterComponentType<TestAnotherComponent>();

        auto componentTypesKey1 = ECS::Domain::CreateComponentTypesKey();
        ECS::Domain::AddComponentTypeToKey<TestComponent>(componentTypesKey1);

        auto componentTypesKey2 = ECS::Domain::CreateComponentTypesKey();
        ECS::Domain::AddComponentTypeToKey<TestComponent>(componentTypesKey2);
        ECS::Domain::AddComponentTypeToKey<TestAnotherComponent>(componentTypesKey2);

        auto entity1 = ECS::Domain::CreateEntity(componentTypesKey1);
        auto component = ECS::Domain::GetComponent<TestComponent>(entity1);
        ++component.m_Value;
        ECS::Domain::SetComponent<TestComponent>(entity1, component);

        auto entity2 = ECS::Domain::CreateEntity(componentTypesKey2);
        ECS::Domain::SetComponent<TestComponent>(entity2, TestComponent(3));

        auto entity3 = ECS::Domain::CreateEntity(componentTypesKey2);
        ECS::Domain::SetComponent<TestComponent>(entity3, TestComponent(5));
        ECS::Domain::SetComponent<TestAnotherComponent>(entity3, TestAnotherComponent(15.0f));
        ECS::Domain::DestroyEntity(entity2);

        auto component1 = ECS::Domain::GetComponent<TestComponent>(entity1);
        auto component3 = ECS::Domain::GetComponent<TestComponent>(entity3);
        auto component3another = ECS::Domain::GetComponent<TestAnotherComponent>(entity3);
        ECS::Domain::DestroyAllEntities();
        ECS::ComponentTypeUtility::UnregisterAllComponentTypes();
    }

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (gGraphicsPtr != NULL)
        {
            gGraphicsPtr->SubmitPrimary();
            gGraphicsPtr->PresentFrame();
        }
    }  

    return (int) msg.wParam;
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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_VUNERABLEVULKANENGINE));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_VUNERABLEVULKANENGINE);
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
   gGraphicsPtr = new VulkanGraphics();
   gGraphicsPtr->Initialize(hInstance, hWnd);

   return TRUE;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;

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
        if (gGraphicsPtr != NULL)
        {
            delete gGraphicsPtr;
            gGraphicsPtr = NULL;
        }

        PostQuitMessage(0);
        break;
    case WM_SIZE:
        if (gGraphicsPtr)
        { 
            gGraphicsPtr->Invalidate();
        } 
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
