#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d2d1.h>
#pragma comment(lib, "d2d1")

#include "../../../Nova/nova_geometry.h"
#include "../../../Nova/nova_render.h"

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
	// Register the window class.
	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"Nova Window";
	RegisterClass(&wc);

	// Create the window.
	HWND hWnd = CreateWindowEx(0, wc.lpszClassName, L"Nova", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 600, 600, NULL, NULL, hInstance, NULL);
	if (hWnd == NULL)
		return -1;
	ShowWindow(hWnd, nCmdShow);

	if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory)))
		return -1;

	// Set up my stuff.
	mesh = CreateMeshFromFile("../../../models/bunny-1500.obj");
	if (!mesh)
		MessageBox(NULL, L"ERROR", L"Unable to load mesh file!", MB_OK);

	MatSetTranslate(&pos, 0.0f, -0.1f, -0.3f);

	// Run the message loop.
	MSG msg = { 0 };
	DWORD start = GetTickCount();
	DWORD now;
	int frame = 0;
	wchar_t d[20];
	while (msg.message != WM_QUIT)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

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
			D2D1_BITMAP_PROPERTIES bmp = D2D1::BitmapProperties(pf);
			hr = pRenderTarget->CreateBitmap(pRenderTarget->GetPixelSize(), bmp, &bm);
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
			
			D2D1_PIXEL_FORMAT pf = pRenderTarget->GetPixelFormat();
			float dpix, dpiy;
			pRenderTarget->GetDpi(&dpix, &dpiy);
			D2D1_BITMAP_PROPERTIES bmp = D2D1::BitmapProperties(pf, dpix, dpiy);
			RECT rc;
			GetClientRect(hWnd, &rc);
			pRenderTarget->CreateBitmap(D2D1::SizeU(rc.right, rc.bottom), bmp, &bm);
		}

		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		pRenderTarget->BeginDraw();

		clear_pixel_buffer();
		clear_depth_buffer();

		if (mesh != NULL)
			RenderMesh(mesh, &pos);

		RECT rc;
		GetClientRect(hWnd, &rc);
		D2D1_RECT_U rect = D2D1::RectU(0, 0, rc.right, rc.bottom);
		uint32_t *src = get_pixel_buffer();
		HRESULT hr = bm->CopyFromMemory(NULL, src, rc.right * sizeof(uint32_t));
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
		set_fov(60.0f);

		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
