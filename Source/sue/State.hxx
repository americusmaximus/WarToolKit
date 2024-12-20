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

#pragma once

#include "Archive.hxx"

#include <stdio.h>

#define DEFAULT_BLOCK_SIZE      16384

#define MAX_FILE_NAME_LENGTH    256

#define MAX_FILE_COUNT          2048

#define MAX_CONTENT_IN_SIZE     0x40000
#define MAX_CONTENT_OUT_SIZE    0x50000

#define MAX_ARCHIVE_ITEM_COUNT  4096

typedef struct AppState {
    int                     Compression;                                    // 0x00411030

    int                     IsSilent;                                       // 0x00739138
    int                     SkipExtraction;                                 // 0x0073913c

    struct {
        FILE*               File;                                           // 0x00415130

        unsigned            Index;                                          // 0x00739140
        unsigned            Count;                                          // 0x00739144
        unsigned            Size;                                           // 0x0073914c
    } Archive;

    struct {
        byte                In[MAX_CONTENT_IN_SIZE];                        // 0x00465134
        byte                Out[MAX_CONTENT_OUT_SIZE];                      // 0x00415134
    } Content;

    struct {
        char*               Next;                                           // 0x00411034
        char                Names[MAX_FILE_COUNT * MAX_FILE_NAME_LENGTH];   // 0x006a5134
    } Names;

    ARCHIVEITEMDESCRIPTOR   Items[MAX_ARCHIVE_ITEM_COUNT];                  // 0x00725138

    long                    Offsets[MAX_FILE_COUNT * MAX_FILE_NAME_LENGTH]; // 0x004a5134
} APPSTATE, * APPSTATEPTR;

extern APPSTATE State;