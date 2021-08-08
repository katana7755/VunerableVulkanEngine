// VunerableVulkanEngine.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "VunerableVulkanEngine.h"
#include "Graphics/VulkanGraphics.h"
#include "IMGUI/imgui.h"
#include "IMGUI/imgui_impl_win32.h"
#include "IMGUI/imgui_impl_vulkan.h"

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
int gImGuiRenderPassIndex = -1;
int gImGuiDescriptorPoolIndex = -1;
bool gImGuiFontUpdated = false;

void check_vk_result(VkResult err)
{
    if (err == 0)
        return;

    printf_console("[vulkan] Error: VkResult = %d\n", err);

    throw;
}

void InitializeImGui(HWND hWnd)
{
    std::vector<VkAttachmentDescription> attachmentDescArray;
    {
        auto desc = VkAttachmentDescription();
        desc.flags = 0;
        desc.format = VulkanGraphicsResourceSwapchain::GetSwapchainFormat();
        desc.samples = VK_SAMPLE_COUNT_1_BIT; // TODO: need to be modified when starting to consider msaa...(NECESSARY!!!)
        desc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        desc.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachmentDescArray.push_back(desc);
    }
    {
        auto desc = VkAttachmentDescription();
        desc.flags = 0;
        desc.format = VK_FORMAT_D32_SFLOAT; // TODO: need to define depth format (NECESSARY!!!)
        desc.samples = VK_SAMPLE_COUNT_1_BIT; // TODO: need to be modified when starting to consider msaa...(NECESSARY!!!)
        desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachmentDescArray.push_back(desc);
    }

    // TODO: think about optimizable use-cases of subpass...one can be postprocess...
    std::vector<VkSubpassDescription> subPassDescArray;
    std::vector<VkAttachmentReference> subPassInputAttachmentArray;
    std::vector<VkAttachmentReference> subPassColorAttachmentArray;
    auto subPassDepthStencilAttachment = VkAttachmentReference();
    std::vector<uint32_t> subPassPreserveAttachmentArray;
    {
        // subPassInputAttachmentArray
        {
            // TODO: find out when we have to use this...
        }

        // subPassColorAttachmentArray
        {
            auto ref = VkAttachmentReference();
            ref.attachment = 0;
            ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            subPassColorAttachmentArray.push_back(ref);
        }

        subPassDepthStencilAttachment.attachment = 1;
        subPassDepthStencilAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // subPassPreserveAttachmentArray
        {
            // TODO: find out when we have to use this...
        }

        auto desc = VkSubpassDescription();
        desc.flags = 0; // TODO: subpass has various flags...need to check what these all are for...
        desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // TODO: one day we need to implement compute pipeline as well...(NECESSARY!!!)
        desc.inputAttachmentCount = subPassInputAttachmentArray.size();
        desc.pInputAttachments = subPassInputAttachmentArray.data();
        desc.colorAttachmentCount = subPassColorAttachmentArray.size();
        desc.pColorAttachments = subPassColorAttachmentArray.data();
        desc.pResolveAttachments = NULL; // TODO: msaa...(NECESSARY!!!)
        desc.pDepthStencilAttachment = &subPassDepthStencilAttachment;
        desc.preserveAttachmentCount = subPassPreserveAttachmentArray.size();
        desc.pPreserveAttachments = subPassPreserveAttachmentArray.data();
        subPassDescArray.push_back(desc);
    }

    // TODO: still it's not perfectly clear... let's study more on this...
    std::vector<VkSubpassDependency> subPassDepArray;
    {
        auto dep = VkSubpassDependency();
        dep.srcSubpass = VK_SUBPASS_EXTERNAL;
        dep.dstSubpass = 0;
        dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep.srcAccessMask = 0;
        dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dep.dependencyFlags = 0;
        subPassDepArray.push_back(dep);
    }

    gImGuiRenderPassIndex = VulkanGraphicsResourceRenderPassManager::CreateRenderPass(attachmentDescArray, subPassDescArray, subPassDepArray);

    auto poolSizeArray = std::vector<VkDescriptorPoolSize>();
    {
        auto poolSize = VkDescriptorPoolSize();
        poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLER;
        poolSize.descriptorCount = 1000;
        poolSizeArray.push_back(poolSize);
    }
    {
        auto poolSize = VkDescriptorPoolSize();
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = 1000;
        poolSizeArray.push_back(poolSize);
    }
    {
        auto poolSize = VkDescriptorPoolSize();
        poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        poolSize.descriptorCount = 1000;
        poolSizeArray.push_back(poolSize);
    }
    {
        auto poolSize = VkDescriptorPoolSize();
        poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        poolSize.descriptorCount = 1000;
        poolSizeArray.push_back(poolSize);
    }
    {
        auto poolSize = VkDescriptorPoolSize();
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        poolSize.descriptorCount = 1000;
        poolSizeArray.push_back(poolSize);
    }
    {
        auto poolSize = VkDescriptorPoolSize();
        poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        poolSize.descriptorCount = 1000;
        poolSizeArray.push_back(poolSize);
    }
    {
        auto poolSize = VkDescriptorPoolSize();
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = 1000;
        poolSizeArray.push_back(poolSize);
    }
    {
        auto poolSize = VkDescriptorPoolSize();
        poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSize.descriptorCount = 1000;
        poolSizeArray.push_back(poolSize);
    }
    {
        auto poolSize = VkDescriptorPoolSize();
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        poolSize.descriptorCount = 1000;
        poolSizeArray.push_back(poolSize);
    }
    {
        auto poolSize = VkDescriptorPoolSize();
        poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        poolSize.descriptorCount = 1000;
        poolSizeArray.push_back(poolSize);
    }
    {
        auto poolSize = VkDescriptorPoolSize();
        poolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        poolSize.descriptorCount = 1000;
        poolSizeArray.push_back(poolSize);
    }

    gImGuiDescriptorPoolIndex = VulkanGraphicsResourcePipelineManager::CreateDescriptorPool(poolSizeArray);

    auto descriptorPool = VulkanGraphicsResourcePipelineManager::GetDescriptorPool(gImGuiDescriptorPoolIndex);

    //gImGuiWindow.Surface = VulkanGraphicsResourceSurface::GetSurface();
    //gImGuiWindow.SurfaceFormat = VkSurfaceFormatKHR { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
    //gImGuiWindow.PresentMode = VK_PRESENT_MODE_FIFO_KHR;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hWnd);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = VulkanGraphicsResourceInstance::GetInstance();
    init_info.PhysicalDevice = VulkanGraphicsResourceDevice::GetPhysicalDevice();
    init_info.Device = VulkanGraphicsResourceDevice::GetLogicalDevice();
    init_info.QueueFamily = VulkanGraphicsResourceDevice::GetGraphicsQueueFamilyIndex();
    init_info.Queue = VulkanGraphicsResourceDevice::GetGraphicsQueue();
    init_info.PipelineCache = VulkanGraphicsResourcePipelineManager::GetPipelineCache();
    init_info.DescriptorPool = descriptorPool;
    init_info.Allocator = NULL;
    init_info.MinImageCount = 2;
    init_info.ImageCount = VulkanGraphicsResourceSwapchain::GetImageViewCount();
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, VulkanGraphicsResourceRenderPassManager::GetRenderPass(gImGuiRenderPassIndex)); // TODO: will we need to make an individual render pass?
    gImGuiFontUpdated = false;
}

void DeinitializeImGui()
{
    auto err = vkDeviceWaitIdle(VulkanGraphicsResourceDevice::GetLogicalDevice());
    check_vk_result(err);
    ImGui_ImplVulkan_Shutdown();
    ImGui::DestroyContext();
    gImGuiFontUpdated = false;
    VulkanGraphicsResourcePipelineManager::DestroyDescriptorPool(gImGuiDescriptorPoolIndex);
    VulkanGraphicsResourceRenderPassManager::DestroyRenderPass(gImGuiRenderPassIndex);
    gImGuiDescriptorPoolIndex = -1;
    gImGuiRenderPassIndex = -1;
}

bool show_demo_window = true;
bool show_another_window = false;

void DrawImGuiFrame()
{
    // Upload Fonts
    if (!gImGuiFontUpdated)
    {
        gImGuiFontUpdated = true;

        // Use any command queue
        auto command_buffer = VulkanGraphicsResourceCommandBufferManager::AllocateAdditionalCommandBuffer();
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        auto err = vkBeginCommandBuffer(command_buffer, &begin_info);
        check_vk_result(err);
        ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

        VkSubmitInfo end_info = {};
        end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        end_info.commandBufferCount = 1;
        end_info.pCommandBuffers = &command_buffer;
        err = vkEndCommandBuffer(command_buffer);
        check_vk_result(err);
        err = vkQueueSubmit(VulkanGraphicsResourceDevice::GetGraphicsQueue(), 1, &end_info, VK_NULL_HANDLE);
        check_vk_result(err);

        err = vkDeviceWaitIdle(VulkanGraphicsResourceDevice::GetLogicalDevice());
        check_vk_result(err);
        ImGui_ImplVulkan_DestroyFontUploadObjects();
        VulkanGraphicsResourceCommandBufferManager::ClearAdditionalCommandBuffers();
    }

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }

    // Rendering
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

    if (!is_minimized)
    {
        auto command_buffer = VulkanGraphicsResourceCommandBufferManager::AllocateAdditionalCommandBuffer();
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags |= (VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);

        auto err = vkBeginCommandBuffer(command_buffer, &begin_info);
        check_vk_result(err);
        gGraphicsPtr->BeginRenderPass(command_buffer, gImGuiRenderPassIndex);

        // Record dear imgui primitives into command buffer
        ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer);

        gGraphicsPtr->EndRenderPass(command_buffer);
        err = vkEndCommandBuffer(command_buffer);
        check_vk_result(err);
    }
}

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
            gGraphicsPtr->InitializeFrame();
            gGraphicsPtr->SubmitPrimary();
            DrawImGuiFrame();
            gGraphicsPtr->SubmitAdditional();
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
   InitializeImGui(hWnd);

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
        DeinitializeImGui();

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
