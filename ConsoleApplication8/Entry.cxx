#include <chrono>
#include <thread>
#include <string>

#include <d3d11.h>
#include <dxgi.h>
#include <dwmapi.h>
#include <tchar.h>

#include "Driver.hxx"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx11.h"
#include "menu.hxx"
#include "esp.hxx"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static ID3D11Device* g_d3d_device = nullptr;
static ID3D11DeviceContext* g_d3d_context = nullptr;
static IDXGISwapChain* g_swapchain = nullptr;
static ID3D11RenderTargetView* g_rtv = nullptr;
static ID3D11BlendState* g_blend = nullptr;

static HWND g_game = nullptr;

static bool rect_valid(const RECT& r) {
	const int w = r.right - r.left;
	const int h = r.bottom - r.top;
	return w >= 200 && h >= 200 && r.left > -9000 && r.top > -9000;
}

struct pick_ctx {
	HWND best = nullptr;
	long long best_score = -1;
};

static BOOL CALLBACK enum_pick(HWND hwnd, LPARAM lp) {
	auto* ctx = reinterpret_cast<pick_ctx*>(lp);
	if (!::IsWindow(hwnd) || !::IsWindowVisible(hwnd) || ::IsIconic(hwnd))
		return TRUE;
	RECT r{};
	if (!::GetWindowRect(hwnd, &r) || !rect_valid(r))
		return TRUE;
	const long long area = (long long)(r.right - r.left) * (r.bottom - r.top);

	wchar_t cls[128] = {}, title[256] = {};
	::GetClassNameW(hwnd, cls, 128);
	::GetWindowTextW(hwnd, title, 256);
	std::wstring c = cls, t = title;

	long long score = area;
	if (c == L"UnrealWindow")
		score += 1000000LL;
	if (t.find(L"Fortnite") != std::wstring::npos)
		score += 2000000LL;
	if (hwnd == ::GetForegroundWindow())
		score += 5000000LL;

	if (score > ctx->best_score) {
		ctx->best_score = score;
		ctx->best = hwnd;
	}
	return TRUE;
}

static HWND find_game_window() {
	pick_ctx ctx{};
	::EnumWindows(enum_pick, reinterpret_cast<LPARAM>(&ctx));
	return ctx.best;
}

static bool create_device(HWND hwnd) {
	DXGI_SWAP_CHAIN_DESC sd{};
	sd.BufferCount = 2;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hwnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT create_flags = 0;
	D3D_FEATURE_LEVEL feature_level;
	const D3D_FEATURE_LEVEL levels[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
	if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, create_flags,
		levels, 2, D3D11_SDK_VERSION, &sd, &g_swapchain,
		&g_d3d_device, &feature_level, &g_d3d_context) != S_OK)
		return false;

	ID3D11Texture2D* back_buffer = nullptr;
	g_swapchain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
	if (back_buffer) {
		g_d3d_device->CreateRenderTargetView(back_buffer, nullptr, &g_rtv);
		back_buffer->Release();
	}
	if (!g_rtv)
		return false;

	D3D11_BLEND_DESC bd{};
	bd.RenderTarget[0].BlendEnable = TRUE;
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	g_d3d_device->CreateBlendState(&bd, &g_blend);

	return true;
}

static void create_rtv() {
	if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
	ID3D11Texture2D* back_buffer = nullptr;
	if (g_swapchain) {
		g_swapchain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
		if (back_buffer) {
			g_d3d_device->CreateRenderTargetView(back_buffer, nullptr, &g_rtv);
			back_buffer->Release();
		}
	}
}

static void cleanup_device() {
	if (g_blend) { g_blend->Release(); g_blend = nullptr; }
	if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
	if (g_swapchain) { g_swapchain->Release(); g_swapchain = nullptr; }
	if (g_d3d_context) { g_d3d_context->Release(); g_d3d_context = nullptr; }
	if (g_d3d_device) { g_d3d_device->Release(); g_d3d_device = nullptr; }
}

static void set_click_through(HWND hwnd, bool through) {
	LONG ex = ::GetWindowLongW(hwnd, GWL_EXSTYLE);
	if (through)
		ex |= WS_EX_TRANSPARENT;
	else
		ex &= ~WS_EX_TRANSPARENT;
	::SetWindowLongW(hwnd, GWL_EXSTYLE, ex);
}

static LRESULT WINAPI wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
		return true;

	switch (msg) {
	case WM_SIZE:
		if (g_d3d_device && wparam != SIZE_MINIMIZED)
			create_rtv();
		return 0;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProcW(hwnd, msg, wparam, lparam);
}

int main() {
	core::init();

	std::uintptr_t image_base = 0;
	if (core::inited) {
		for (int i = 0; i < 600 && !core::pid; ++i) {
			core::find_proc(L"FortniteClient-Win64-Shipping.exe");
			if (!core::pid)
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		if (core::pid) {
			image_base = core::get_base();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			core::DecryptCr3();
		}
	}

	g_game = find_game_window();
	RECT gr{ 100, 100, 900, 700 };
	RECT tmp{};
	if (g_game && ::GetWindowRect(g_game, &tmp) && rect_valid(tmp)) {
		gr = tmp;
	} else {
		g_game = nullptr;
	}
	const int gw = gr.right - gr.left;
	const int gh = gr.bottom - gr.top;

	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.style = CS_CLASSDC;
	wc.lpfnWndProc = wnd_proc;
	wc.hInstance = ::GetModuleHandleW(nullptr);
	wc.lpszClassName = L"OverlayFN";
	::RegisterClassExW(&wc);

	HWND hwnd = ::CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
		wc.lpszClassName, L"",
		WS_POPUP,
		gr.left, gr.top, gw, gh,
		nullptr, nullptr, wc.hInstance, nullptr);

	if (!hwnd)
		return 1;

	::SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
	MARGINS m{ -1, -1, -1, -1 };
	::DwmExtendFrameIntoClientArea(hwnd, &m);

	if (!create_device(hwnd)) {
		cleanup_device();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);
	::SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	menu::setup_style();

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_d3d_device, g_d3d_context);

	bool menu_was_open = true;
	menu::open = true;
	set_click_through(hwnd, !menu_was_open);

	bool done = false;
	while (!done) {
		MSG msg;
		while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
			::TranslateMessage(&msg);
			::DispatchMessageW(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}
		if (done)
			break;

		if (::GetAsyncKeyState(VK_INSERT) & 1) {
			menu::open = !menu::open;
			std::this_thread::sleep_for(std::chrono::milliseconds(150));  
		}
		if (menu::open != menu_was_open) {
			menu_was_open = menu::open;
			set_click_through(hwnd, !menu_was_open);
		}

		HWND fg = find_game_window();
		if (fg)
			g_game = fg;
		if (g_game && ::IsWindow(g_game) && !::IsIconic(g_game)) {
			RECT r{};
			if (::GetWindowRect(g_game, &r) && rect_valid(r)) {
				const int w = r.right - r.left;
				const int h = r.bottom - r.top;
				RECT cur{};
				::GetWindowRect(hwnd, &cur);
				const int cw = cur.right - cur.left;
				const int ch = cur.bottom - cur.top;
				if (cur.left != r.left || cur.top != r.top || cw != w || ch != h) {
					::SetWindowPos(hwnd, HWND_TOPMOST, r.left, r.top, w, h,
						SWP_NOACTIVATE | SWP_SHOWWINDOW);
					if ((w != cw || h != ch) && g_swapchain) {
						if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
						g_swapchain->ResizeBuffers(0, (UINT)w, (UINT)h, DXGI_FORMAT_UNKNOWN, 0);
						create_rtv();
					}
				} else {
					::SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
						SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
				}
			}
		} else {
			::SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
		}

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		menu::render();

		settings::set_resolution((int)io.DisplaySize.x, (int)io.DisplaySize.y);
		if (image_base && core::inited) {
			static int cache_tick = 0;
			if ((cache_tick++ % 3) == 0)
				cache::update(image_base);
			esp::render(ImGui::GetForegroundDrawList());
		}

		ImGui::Render();
		const float clear_col[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		const float blend[4] = { 0, 0, 0, 0 };
		g_d3d_context->OMSetBlendState(g_blend, blend, 0xFFFFFFFF);
		g_d3d_context->OMSetRenderTargets(1, &g_rtv, nullptr);
		g_d3d_context->ClearRenderTargetView(g_rtv, clear_col);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		g_swapchain->Present(1, 0);
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	cleanup_device();
	::DestroyWindow(hwnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return 0;
}
