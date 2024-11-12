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

#include "DirectDraw.hxx"
#include "Image.hxx"

#include <stdlib.h>

#pragma pack(push, 1)
typedef struct ImageHeader
{
    UINT Offset;
} IMAGEHEADER, * IMAGEHEADERPTR;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct PackedPixel
{
    BYTE Length;
    USHORT Pixel;
} PACKEDPIXEL, * PACKEDPIXELPTR;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct ImageFrame
{
    SHORT X;
    SHORT Y;
    SHORT Width;
    SHORT Height;
    BYTE Colors; // ???
    USHORT Next;
    PACKEDPIXEL Pixels[1];
} IMAGEFRAME, * IMAGEFRAMEPTR;
#pragma pack(pop)

UINT GetImageFrameCount(LPCVOID image)
{
    return ((IMAGEHEADERPTR)image)->Offset / sizeof(IMAGEHEADER);
}

UINT GetImageFrameOffset(LPCVOID image, CONST UINT index)
{
    return ((IMAGEHEADERPTR)((DWORD_PTR)image + (DWORD_PTR)(index * sizeof(IMAGEHEADER))))->Offset;
}

IMAGEFRAMEPTR GetImageFrame(LPCVOID image, CONST UINT index)
{
    return (IMAGEFRAMEPTR)((DWORD_PTR)image + (DWORD_PTR)GetImageFrameOffset(image, index));
}

PACKEDPIXELPTR GetImageFramePixels(IMAGEFRAMEPTR frame, CONST UINT index)
{
    if (index == 0) { return frame->Pixels; }

    PACKEDPIXELPTR pixels = (PACKEDPIXELPTR)((DWORD_PTR)frame + (DWORD_PTR)frame->Next + 9 + 2);

    for (UINT x = 0; x < index - 1; x++)
    {
        pixels = (PACKEDPIXELPTR)((DWORD_PTR)pixels + (DWORD_PTR)(*(USHORT*)pixels + 2));
    }

    return (PACKEDPIXELPTR)((DWORD_PTR)pixels + (DWORD_PTR)2);
}

BOOL OpenImage(HANDLE hFile, IMAGECONTAINERPTR image)
{
    CONST UINT size = GetFileSize(hFile, NULL);
    if (size == NULL) { return FALSE; }

    LPVOID content = malloc(size);
    if (content == NULL) { return FALSE; }

    DWORD read = 0;
    if (!ReadFile(hFile, content, size, &read, NULL) || size != read) { free(content); return FALSE; }

    image->Frames = GetImageFrameCount(content);
    if (image->Frames == 0) { free(content); return FALSE; }

    image->Surfaces = (LPDIRECTDRAWSURFACE7*)malloc(image->Frames * sizeof(LPDIRECTDRAWSURFACE7));
    if (image->Surfaces == NULL) { free(content); return FALSE; }
    ZeroMemory(image->Surfaces, image->Frames * sizeof(LPDIRECTDRAWSURFACE7));

    BOOL fail = FALSE;
    for (UINT i = 0; i < image->Frames; i++)
    {
        IMAGEFRAMEPTR frame = GetImageFrame(content, i);

        if (!CreateDirectDrawSurface(&image->Surfaces[i], frame->Width, frame->Height)) { fail = TRUE; break; }
    }

    if (fail)
    {
        for (UINT i = 0; i < image->Frames; i++) { RELEASE(image->Surfaces[i]); }

        free(image->Surfaces);
        free(content);

        return FALSE;
    }

    fail = FALSE;
    for (UINT i = 0; i < image->Frames; i++)
    {
        DDSURFACEDESC2 desc;
        ZeroMemory(&desc, sizeof(DDSURFACEDESC2));

        desc.dwSize = sizeof(DDSURFACEDESC2);

        if (FAILED(image->Surfaces[i]->Lock(NULL, &desc, DDLOCK_NOSYSLOCK | DDLOCK_WAIT, NULL))) { fail = TRUE; break; }

        IMAGEFRAMEPTR frame = GetImageFrame(content, i);

        for (UINT x = 0; x < frame->Height; x++)
        {
            UINT filled = 0;
            PACKEDPIXELPTR pixels = GetImageFramePixels(frame, x);
            DWORD_PTR finish = (DWORD_PTR)GetImageFramePixels(frame, x + 1) - 2;

            while (filled < frame->Width)
            {
                BYTE length = pixels->Length;
                BYTE count = length & 0x7F;

                USHORT* row = (USHORT*)((DWORD_PTR)desc.lpSurface + (DWORD_PTR)(desc.lPitch * x));

                if (length & 0x80)
                {
                    USHORT pixel = pixels->Pixel;

                    for (UINT xx = 0; xx < count; xx++)
                    {
                        if (filled + xx < frame->Width)
                        {
                            row[filled + xx] = pixel;
                        }
                    }

                    pixels = PACKEDPIXELPTR((DWORD_PTR)pixels + (DWORD_PTR)sizeof(PACKEDPIXEL));
                }
                else
                {
                    // Individual colors
                    USHORT* individual = &pixels->Pixel;

                    for (UINT xx = 0; xx < count; xx++)
                    {
                        if (filled + xx < frame->Width)
                        {
                            row[filled + xx] = individual[xx];
                        }
                    }

                    pixels = PACKEDPIXELPTR((DWORD_PTR)pixels + (DWORD_PTR)(sizeof(PACKEDPIXEL) + (count - 1) * sizeof(USHORT)));
                }

                filled = filled + count;

                if (finish <= (DWORD_PTR)pixels) { break; }
            }
        }

        if (FAILED(image->Surfaces[i]->Unlock(NULL))) { fail = TRUE; break; }
    }

    if (fail)
    {
        for (UINT i = 0; i < image->Frames; i++) { RELEASE(image->Surfaces[i]); }

        free(image->Surfaces);
        free(content);

        return FALSE;
    }

    free(content);

    return TRUE;
}