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

#include "File.hxx"

#define ARCHIVE_MAGIC               0x53465A46 /* FZFS */

#define INVALID_ARCHIVE_ITEM_INDEX  (-1)

typedef struct ArchiveHeader
{
    unsigned                    Magic;
    unsigned                    Offset;
} ARCHIVEHEADER, * ARCHIVEHEADERPTR;

typedef struct ArchiveDescriptor
{
    unsigned                    Size;
    unsigned                    Count;
    unsigned                    Length;
} ARCHIVEDESCRIPTOR, * ARCHIVEDESCRIPTORPTR;

typedef enum ArchiveItemType
{
    ARCHIVEITEMTYPE_NONE        = 0,
    ARCHIVEITEMTYPE_FILE        = 1, // Straight unpacked file
    ARCHIVEITEMTYPE_PACKED      = 2, // Packaged into an archive without compression
    ARCHIVEITEMTYPE_COMPRESSED  = 8, // Packaged into an archive with zlib compression
    ARCHIVEITEMTYPE_FORCE_DWORD = 0x7FFFFFFF
} ARCHIVEITEMTYPE, * ARCHIVEITEMTYPEPTR;

typedef struct ArchiveItemDescriptor
{
    unsigned                    Name;
    ARCHIVEITEMTYPE             Type;
    unsigned                    Offset; // Offset to the content within a file.
    unsigned                    Size;
    unsigned                    Chunk;
} ARCHIVEITEMDESCRIPTOR, * ARCHIVEITEMDESCRIPTORPTR;

#define MAX_ARCHIVE_COUNT               16
#define MAX_ARCHIVE_PATH_LENGTH         256

#define MAX_ARCHIVE_ITEM_COUNT          4096
#define ARCHIVE_ITEM_INDEX_MASK         0xFFF

#define MAX_ARCHIVE_ITEM_CHUNK_COUNT    8

typedef struct ArchiveItemChunk
{
    int             Index;
    int             Chunk;
    int             Size;
    void*           Content;
} ARCHIVEITEMCHUNK, * ARCHIVEITEMCHUNKPTR;

typedef struct ArchiveItem
{
    const char*                 Name;
    ARCHIVEITEMTYPE             Type;
    unsigned                    Archive;

    // NOTE:
    // This is used either as a file, or an offset to the file within an archive.
    File                        File;

    unsigned                    Size;
    unsigned                    Chunk;

    // NOTE:
    // An offset of the actual content when reading it from the archive.
    unsigned                    Offset;

    unsigned                    IsActive;
} ARCHIVEITEM, * ARCHIVEITEMPTR;

typedef struct Archive
{
    bool                        IsActive;
    char                        Path[MAX_ARCHIVE_PATH_LENGTH];
    unsigned*                   Offsets;
    char*                       Names;
    File                        File;
} ARCHIVE, * ARCHIVEPTR;

bool OpenArchive(const char* path);
int AcquireArchiveItemIndex(const char* name);
void AcquireArchiveItem(ARCHIVEITEMDESCRIPTORPTR item, const unsigned archive, const char* names);
void* ReadArchiveDetails(File* file, unsigned* count);
bool OpenArchiveItem(const int indx);
unsigned ArchiveItemSize(const int indx);
bool IsArchiveItemAvailable(const int indx);
void CloseArchiveItem(const int indx);
unsigned ReadPackedArchiveItem(void* content, const int indx, const unsigned offset, const unsigned size);
unsigned ReadCompressedArchiveItem(void* content, const int indx, const unsigned offset, const unsigned size);
unsigned ReadArchiveItem(const int indx, void* content, const unsigned size);
void* InitializeArchiveItemChunk(const int indx, const unsigned chunk, const unsigned size);
unsigned AcquireArchiveItemChunkLength(const int indx, const int size);
void* ReadArchiveItemChunk(const int indx, const int chunk);
void* AcquireArchiveItemChunk(const int indx, const int chunk);