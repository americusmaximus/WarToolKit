/*
Copyright (c) 2024 Americus Maximus

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "Image.hxx"
#include "DirectDraw.hxx"

LPDIRECTDRAW7 dd;
LPDIRECTDRAWSURFACE7 dds;

HRESULT InitDirectDraw(HWND hWnd, LPBOOL colorkeys)
{
    HRESULT result = DirectDrawCreateEx(NULL, (LPVOID*)&dd, IID_IDirectDraw7, NULL);

    if (FAILED(result)) { return result; }

    result = dd->SetCooperativeLevel(hWnd, DDSCL_NORMAL);

    if (FAILED(result)) { RELEASE(dd); return result; }

    DDSURFACEDESC2 desc;
    ZeroMemory(&desc, sizeof(DDSURFACEDESC2));

    desc.dwSize = sizeof(DDSURFACEDESC2);
    desc.dwFlags = DDSD_CAPS;
    desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    result = dd->CreateSurface(&desc, &dds, NULL);

    if (FAILED(result)) { RELEASE(dd); return result; }

    LPDIRECTDRAWCLIPPER clipper = NULL;
    result = dd->CreateClipper(0, &clipper, NULL);

    if (FAILED(result)) { RELEASE(dds); RELEASE(dd); return result; }

    result = clipper->SetHWnd(0, hWnd);

    if (FAILED(result)) { RELEASE(dds); RELEASE(dd); return result; }

    result = dds->SetClipper(clipper);
    if (FAILED(result)) { RELEASE(clipper); RELEASE(dds); RELEASE(dd); return result; }

    RELEASE(clipper);

    DDCAPS caps;
    ZeroMemory(&caps, sizeof(DDCAPS));

    caps.dwSize = sizeof(DDCAPS);

    dd->GetCaps(&caps, NULL);

    // Check if the hardware supports color keying.
    *colorkeys = caps.dwCKeyCaps & DDCKEYCAPS_SRCBLT;

    return result;
}

BOOL CreateDirectDrawSurface(LPDIRECTDRAWSURFACE7* surface, CONST UINT width, CONST UINT height)
{
    DDSURFACEDESC2 desc;
    ZeroMemory(&desc, sizeof(DDSURFACEDESC2));

    desc.dwSize = sizeof(DDSURFACEDESC2);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

    desc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    desc.ddpfPixelFormat.dwFlags = DDPF_RGB;

    // RGB565
    desc.ddpfPixelFormat.dwRGBBitCount = 16;

    desc.ddpfPixelFormat.dwRGBAlphaBitMask = 0x00000000;
    desc.ddpfPixelFormat.dwRBitMask = 0x0000F800;
    desc.ddpfPixelFormat.dwGBitMask = 0x000007E0;
    desc.ddpfPixelFormat.dwBBitMask = 0x0000001F;

    desc.dwWidth = width;
    desc.dwHeight = height;

    return SUCCEEDED(dd->CreateSurface(&desc, surface, NULL));
}

VOID DrawImage(HWND hWnd, LPDIRECTDRAWSURFACE7 surface, CONST BOOL transparent, CONST BOOL scaled)
{
    RECT rect;
    GetClientRect(hWnd, &rect);

    POINT point = { 0, 0 };
    ClientToScreen(hWnd, &point);
    OffsetRect(&rect, point.x, point.y);

    if (!scaled)
    {
        DDSURFACEDESC2 desc;
        ZeroMemory(&desc, sizeof(DDSURFACEDESC2));

        desc.dwSize = sizeof(DDSURFACEDESC2);

        surface->GetSurfaceDesc(&desc);

        if (desc.dwWidth < (UINT)(rect.right - rect.left))
        {
            CONST UINT overage = ((rect.right - rect.left) - desc.dwWidth) / 2;

            rect.left = rect.left + overage;
            rect.right = rect.right - overage;
        }

        if (desc.dwHeight < (UINT)(rect.bottom - rect.top))
        {
            CONST UINT overage = ((rect.bottom - rect.top) - desc.dwHeight) / 2;

            rect.top = rect.top + overage;
            rect.bottom = rect.bottom - overage;
        }

        DDBLTFX fx;
        ZeroMemory(&fx, sizeof(DDBLTFX));

        fx.dwSize = sizeof(DDBLTFX);
        fx.dwFillColor = 0x0000;

        dds->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &fx);
    }

    // Blt
    {
        DDBLTFX fx;
        ZeroMemory(&fx, sizeof(DDBLTFX));

        fx.dwSize = sizeof(DDBLTFX);

        DWORD options = DDBLT_WAIT;

        if (transparent)
        {
            options = options | DDBLT_KEYSRCOVERRIDE;

            fx.ddckSrcColorkey.dwColorSpaceLowValue = 0xF81F;
            fx.ddckSrcColorkey.dwColorSpaceHighValue = 0xF81F;
        }

        dds->Blt(&rect, surface, NULL, options, &fx);
    }
}

VOID ReleaseDirectDraw(VOID)
{
    RELEASE(dds);
    RELEASE(dd);
}

BOOL ReleaseDirectDrawSurfaceContent(LPDIRECTDRAWSURFACE7 surface)
{
    return SUCCEEDED(surface->Unlock(NULL));
}

BOOL AcquireDirectDrawSurfaceContent(LPDIRECTDRAWSURFACE7 surface, LPVOID* content, UINT* width, UINT* height, UINT* stride)
{
    DDSURFACEDESC2 desc;
    ZeroMemory(&desc, sizeof(DDSURFACEDESC2));

    desc.dwSize = sizeof(DDSURFACEDESC2);

    HRESULT result = surface->Lock(NULL, &desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);

    *content = desc.lpSurface;
    *width = desc.dwWidth;
    *height = desc.dwHeight;
    *stride = desc.lPitch;

    return SUCCEEDED(result);
}