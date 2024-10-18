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

#define MAX_MESSAGE_LENGTH  576

typedef struct AppState
{
    unsigned            IsSilent;                                   // 0x0060f194
    unsigned            ChunkCount;                                 // 0x0060f198

    ARCHIVEITEMCHUNK    Chunks[MAX_ARCHIVE_ITEM_CHUNK_COUNT];       // 0x0060f1a0
    ARCHIVE             Archives[MAX_ARCHIVE_COUNT];                // 0x0060f220
    ARCHIVEITEM         Items[MAX_ARCHIVE_ITEM_COUNT];              // 0x00610320
} APPSTATE, * APPSTATEPTR;

extern APPSTATE State;