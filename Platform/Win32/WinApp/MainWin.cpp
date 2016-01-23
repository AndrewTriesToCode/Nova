#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d2d1.h>
#pragma comment(lib, "d2d1")

#include "../../../Nova/nova_render.h"
#include "../../../Nova/nova_utility.h"

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Direct 2D vars.
ID2D1Factory *pFactory = NULL;
ID2D1HwndRenderTarget *pRenderTarget = NULL;
ID2D1Bitmap *bm = NULL;
HRESULT CreateD2D(HWND hWnd);
void DestroyD2D();

// My vars.
Mesh *mesh;
Matrix pos, persp;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR pCmdLine, int nCmdShow)
{
	SetProcessDPIAware();

	// Register the window class.
	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"Nova Window";
	RegisterClass(&wc);

	// Create the window.
	HWND hWnd = CreateWindowEx(0, wc.lpszClassName, L"Nova", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 500, 500, NULL, NULL, hInstance, NULL);
	if (hWnd == NULL)
		return -1;
	ShowWindow(hWnd, nCmdShow);

	if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory)))
		return -1;

	// Set up my stuff.
	mesh = CreateMeshFromFile("../../../models/f16/f16.obj");
	//mesh = CreateMeshFromFile("../../../models/test_tri.obj");

	if (!mesh)
		MessageBox(NULL, L"Unable to load mesh file!", L"ERROR", MB_OK);

	// Run the message loo0
	MSG msg = { 0 };
	DWORD start = GetTickCount();
	DWORD now;
	int frame = 0;
	wchar_t d[20];
	while (msg.message != WM_QUIT)
	{
		static float ang = 0.0f;

		Matrix rot, rot2, trans, pos1, pos2;

		MatSetRotY(&rot, ang);
		MatSetRotX(&rot2, ang / 2.0f);
		ang -= 0.002f;
		MatSetTranslate(&trans, 0.0f, 0.0f, -3.f);
		MatMul(&rot2, &rot, &pos1);
		MatMul(&trans, &pos1, &pos);

		InvalidateRect(hWnd, NULL, FALSE);

		frame++;
		now = GetTickCount();
		if (now - start > 2000)
		{
			start = now;
			_itow_s(frame / 2, d, 20, 10);
			SetWindowText(hWnd, d);
			frame = 0;
		}

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	if (pFactory)
	{
		pFactory->Release();
		pFactory = NULL;
	}

	DestroyD2D();
	DestroyMesh(mesh);

	return 0;
}

HRESULT CreateD2D(HWND hWnd)
{
	HRESULT hr = S_OK;

	if (pRenderTarget == NULL)
	{
		RECT rc;
		GetClientRect(hWnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

		hr = pFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWnd, size, D2D1_PRESENT_OPTIONS_IMMEDIATELY), &pRenderTarget);

		if (SUCCEEDED(hr))
		{
			D2D1_PIXEL_FORMAT pf = pRenderTarget->GetPixelFormat();
			float dpix, dpiy;
			pFactory->GetDesktopDpi(&dpix, &dpiy);
			D2D1_BITMAP_PROPERTIES bmp = D2D1::BitmapProperties(pf, dpix, dpiy);
			pRenderTarget->CreateBitmap(D2D1::SizeU(rc.right, rc.bottom), bmp, &bm);
		}
	}

	return hr;
}

void DestroyD2D()
{
	if (pRenderTarget != NULL)
	{
		pRenderTarget->Release();
		pRenderTarget = NULL;
	}
	if (bm != NULL)
	{
		bm->Release();
		bm = NULL;
	}
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:

		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);

		return 0;

	case WM_PAINT:
	{
		if (pRenderTarget == NULL)
		{
			CreateD2D(hWnd);
		}

		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		pRenderTarget->BeginDraw();

		clear_pixel_buffer();
		clear_depth_buffer();

		if (mesh != NULL)
			render_mesh_bary(mesh, &pos);

		RECT rc;
		GetClientRect(hWnd, &rc);
		uint32_t *src = get_pixel_buffer();
		HRESULT hr = bm->CopyFromMemory(NULL, src, rc.right * BYTES_PER_PIXEL);

		pRenderTarget->DrawBitmap(bm);

		hr = pRenderTarget->EndDraw();

		if (hr == D2DERR_RECREATE_TARGET)
			DestroyD2D();

		EndPaint(hWnd, &ps);
	}
	return 0;

	case WM_SIZE:
		RECT rc;
		GetClientRect(hWnd, &rc);

		DestroyD2D();

		set_screen_size(rc.right, rc.bottom);
		set_hfov(60.0f);

		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
