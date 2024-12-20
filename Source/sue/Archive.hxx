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

#include "Base.hxx"

#define ARCHIVE_MAGIC               0x53465A46 /* FZFS */

typedef enum ArchiveItemType
{
    ARCHIVEITEMTYPE_NONE            = 0,
    ARCHIVEITEMTYPE_FILE            = 1, // Straight unpacked file
    ARCHIVEITEMTYPE_PACKED          = 2, // Packaged into an archive without compression
    ARCHIVEITEMTYPE_COMPRESSED      = 8, // Packaged into an archive with zlib compression
    ARCHIVEITEMTYPE_FORCE_DWORD     = 0x7FFFFFFF
} ARCHIVEITEMTYPE, * ARCHIVEITEMTYPEPTR;

typedef struct ArchiveItemDescriptor
{
    unsigned                    Name;
    ARCHIVEITEMTYPE             Type;
    unsigned                    Offset; // Offset to the content within a file.
    unsigned                    Size;
    unsigned                    Chunk;
} ARCHIVEITEMDESCRIPTOR, * ARCHIVEITEMDESCRIPTORPTR;

void InitializeArchives();

void Save(const void* data, const unsigned count, const unsigned size);

void ArchiveFile(const char* path, const char* name, const int block);
void ArchivePath(const char* path, const char* name, const char* pattern, const int block, const int subdirs, const int flatten);