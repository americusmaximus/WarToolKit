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

#include "State.hxx"

#include <stdio.h>
#include <zlib.h>

// 0x004014a0
bool OpenArchive(const char* path)
{
    int indx = -1;

    for (int i = 0; i < MAX_ARCHIVE_COUNT; i++)
    {
        if (!State.Archives[i].IsActive) { indx = i; break; }
    }

    if (indx == -1) { return false; }

    File file;
    ARCHIVEHEADER header;

    file.Open(path, FILEOPENOPTIONS_READ);
    file.Read(&header, sizeof(ARCHIVEHEADER));

    if (header.Magic != ARCHIVE_MAGIC)
    {
        file.Close();

        return false;
    }

    file.SetPosition(header.Offset, FILE_CURRENT);

    unsigned count = 0;
    ARCHIVEITEMDESCRIPTORPTR files = (ARCHIVEITEMDESCRIPTORPTR)ReadArchiveDetails(&file, &count);
    char* names = (char*)ReadArchiveDetails(&file, NULL);
    unsigned* offsets = (unsigned*)ReadArchiveDetails(&file, NULL);

    for (unsigned i = 0; i < count; i++) { AcquireArchiveItem(&files[i], indx, names); }

    free(files);

    strcpy(State.Archives[indx].Path, path);

    State.Archives[indx].IsActive = true;
    State.Archives[indx].File.Handle = INVALID_HANDLE_VALUE;
    State.Archives[indx].Offsets = offsets;
    State.Archives[indx].Names = names;

    return true;
}

// 0x004015f0
void* ReadArchiveDetails(File* file, unsigned* count)
{
    ARCHIVEDESCRIPTOR desc;

    file->Read(&desc, sizeof(ARCHIVEDESCRIPTOR));

    Bytef* src = (Bytef*)malloc(desc.Size);

    unsigned length = desc.Length * desc.Count;
    void* dst = malloc(length);

    file->Read(src, desc.Size);

    uncompress((Bytef*)dst, (uLong*)&length, src, desc.Size);

    if (count != NULL) { *count = desc.Count; }

    free(src);

    return dst;
}

// 0x00401670
void AcquireArchiveItem(ARCHIVEITEMDESCRIPTORPTR item, const unsigned archive, const char* names)
{
    const char* name = (const char*)((size_t)names + (size_t)item->Name);

    unsigned indx = crc32_z(0, (byte*)name, strlen(name) + 1) & ARCHIVE_ITEM_INDEX_MASK;

    for (int i = 0; i < MAX_ARCHIVE_ITEM_COUNT; i++)
    {
        if (State.Items[indx].Type != ARCHIVEITEMTYPE_NONE) { indx = (indx + 1) % MAX_ARCHIVE_ITEM_COUNT; }
    }

    State.Items[indx].Name = name;
    State.Items[indx].Type = item->Type;
    State.Items[indx].Archive = archive;

    // NOTE: Don't ask me why...
    State.Items[indx].File.Handle = (HANDLE)item->Offset;
    State.Items[indx].Size = item->Size;
    State.Items[indx].Chunk = item->Chunk;
    State.Items[indx].Offset = 0;
}

// 0x00401810
int AcquireArchiveItemIndex(const char* name)
{
    int indx = crc32_z(0, (byte*)name, strlen(name) + 1) & ARCHIVE_ITEM_INDEX_MASK;

    for (int i = indx; i < MAX_ARCHIVE_ITEM_COUNT; i++)
    {
        if (State.Items[indx].Type == ARCHIVEITEMTYPE_NONE || _strcmpi(State.Items[indx].Name, name) == 0) { return indx; }

        indx = (indx + 1) % MAX_ARCHIVE_ITEM_COUNT;
    }

    return INVALID_ARCHIVE_ITEM_INDEX;
}

// 0x00401750
bool OpenArchiveItem(const int indx)
{
    if (indx == INVALID_ARCHIVE_ITEM_INDEX) { return false; }

    switch (State.Items[indx].Type)
    {
    case ARCHIVEITEMTYPE_FILE:
    {
        char path[MAX_ARCHIVE_PATH_LENGTH];

        sprintf(path, "%s%s", State.Archives[State.Items[indx].Archive].Path, State.Items[indx].Name);

        return State.Items[indx].File.Open(path, FILEOPENOPTIONS_READ);
    }
    case ARCHIVEITEMTYPE_PACKED:
    case ARCHIVEITEMTYPE_COMPRESSED:
    {
        if (State.Items[indx].IsActive) { return false; }

        State.Items[indx].Offset = 0;
        State.Items[indx].IsActive = true;

        return true;
    }
    }

    return false;
}

// 0x00401d00
unsigned ArchiveItemSize(const int indx)
{
    switch (State.Items[indx].Type)
    {
    case ARCHIVEITEMTYPE_FILE: { return State.Items[indx].File.Size(); }
    case ARCHIVEITEMTYPE_PACKED:
    case ARCHIVEITEMTYPE_COMPRESSED: { return State.Items[indx].Size; }
    }

    return 0;
}

// 0x004018a0
bool IsArchiveItemAvailable(const int indx)
{
    if (indx == INVALID_ARCHIVE_ITEM_INDEX) { return false; }

    switch (State.Items[indx].Type)
    {
    case ARCHIVEITEMTYPE_FILE: { return State.Items[indx].File.Handle != INVALID_HANDLE_VALUE; }
    case ARCHIVEITEMTYPE_PACKED:
    case ARCHIVEITEMTYPE_COMPRESSED: { return State.Items[indx].IsActive; }
    }

    return false;
}

// 0x00401980
unsigned ReadArchiveItem(const int indx, void* content, unsigned size)
{
    switch (State.Items[indx].Type)
    {
    case ARCHIVEITEMTYPE_FILE:
    {
        if (State.Items[indx].File.Handle == INVALID_HANDLE_VALUE) { return 0; }

        return State.Items[indx].File.Read(content, size);
    }
    case ARCHIVEITEMTYPE_PACKED:
    {
        unsigned result = ReadPackedArchiveItem(content, indx, State.Items[indx].Offset, size);

        State.Items[indx].Offset = State.Items[indx].Offset + result;

        return result;
    }
    case ARCHIVEITEMTYPE_COMPRESSED:
    {
        unsigned result = ReadCompressedArchiveItem(content, indx, State.Items[indx].Offset, size);

        State.Items[indx].Offset = State.Items[indx].Offset + result;

        return result;
    }
    }

    return 0;
}

// 0x00401920
void CloseArchiveItem(const int indx)
{
    const ARCHIVEITEMTYPE type = State.Items[indx].Type;

    if (type == ARCHIVEITEMTYPE_FILE)
    {
        State.Items[indx].File.Close();
    }
    else if (type == ARCHIVEITEMTYPE_PACKED || type == ARCHIVEITEMTYPE_COMPRESSED)
    {
        State.Items[indx].IsActive = false;
    }
}

// 0x00401c60
unsigned ReadPackedArchiveItem(void* content, const int indx, const unsigned offset, const unsigned size)
{
    const unsigned archive = State.Items[indx].Archive;

    if (State.Archives[archive].File.Handle == INVALID_HANDLE_VALUE)
    {
        State.Archives[archive].File.Open(State.Archives[archive].Path, FILEOPENOPTIONS_READ);
    }

    unsigned result = State.Items[indx].Size < offset + size
        ? State.Items[indx].Size - offset : size;

    if (result != 0)
    {
        // NOTE: Don't ask me why...
        const unsigned start = (unsigned)State.Items[indx].File.Handle;

        State.Archives[archive].File.SetPosition(start + offset, FILE_BEGIN);
        State.Archives[archive].File.Read(content, result);
    }

    return result;
}

// 0x00401a20
unsigned ReadCompressedArchiveItem(void* content, const int indx, const unsigned offset, const unsigned size)
{
    if (State.Items[indx].Size <= offset) { return 0; }

    unsigned chunk = offset / State.Items[indx].Chunk;

    // Offset within current chunk, 0 for non-initial chunk
    unsigned start = offset - chunk * State.Items[indx].Chunk;

    unsigned completed = 0;
    unsigned left = size;

    unsigned actual = size;
    if (State.Items[indx].Size < offset + actual)
    {
        left = State.Items[indx].Size - offset;
        actual = left;
    }

    while (left != 0)
    {
        const unsigned length = left < State.Items[indx].Size - start
            ? left : State.Items[indx].Size - start;

        void* value = ReadArchiveItemChunk(indx, chunk);

        if (value == NULL) { break; }

        memcpy((void*)((byte*)content + completed), (void*)((byte*)value + start), length);

        completed = completed + length;
        left = left - length;

        start = 0; // Start reading from the beginning of chunk for non-first chunk.
        chunk = chunk + 1;
    }

    return actual;
}

// 0x00401bf0
void* AcquireArchiveItemChunk(const int indx, const int chunk)
{
    for (unsigned x = 0; x < MAX_ARCHIVE_ITEM_CHUNK_COUNT; x++)
    {
        if (State.Chunks[x].Index == indx && State.Chunks[x].Chunk == chunk && State.Chunks[x].Content != NULL)
        {
            return State.Chunks[x].Content;
        }
    }

    return NULL;
}

// 0x00401c30
unsigned AcquireArchiveItemChunkLength(const int indx, const int size)
{
    unsigned result = State.Items[indx].Chunk;

    if (State.Items[indx].Size < result + size) { result = State.Items[indx].Size - size; }

    return result;
}

// 0x00401400
void* InitializeArchiveItemChunk(const int indx, const unsigned chunk, const unsigned size)
{
    if (State.Chunks[State.ChunkCount].Content != NULL) { free(State.Chunks[State.ChunkCount].Content); }

    State.Chunks[State.ChunkCount].Index = indx;
    State.Chunks[State.ChunkCount].Chunk = chunk;
    State.Chunks[State.ChunkCount].Size = size;

    State.Chunks[State.ChunkCount].Content = malloc(size);

    void* result = State.Chunks[State.ChunkCount].Content;

    // Wrap around...
    State.ChunkCount = (State.ChunkCount + 1) & (MAX_ARCHIVE_ITEM_CHUNK_COUNT - 1);

    return result;
}

// 0x00401af0
void* ReadArchiveItemChunk(const int indx, const int chunk)
{
    void* result = AcquireArchiveItemChunk(indx, chunk);

    if (result == NULL)
    {
        const unsigned archive = State.Items[indx].Archive;

        if (State.Archives[archive].File.Handle == INVALID_HANDLE_VALUE)
        {
            State.Archives[archive].File.Open(State.Archives[archive].Path, FILEOPENOPTIONS_READ);
        }

        // NOTE: Don't ask me why...
        const unsigned index = (unsigned)State.Items[indx].File.Handle + chunk;

        State.Archives[archive].File.SetPosition(State.Archives[archive].Offsets[index], FILEOPENOPTIONS_READ);
        
        const unsigned size = State.Archives[archive].Offsets[index + 1] - State.Archives[archive].Offsets[index];

        unsigned length = AcquireArchiveItemChunkLength(indx, State.Items[indx].Chunk * chunk);
        
        Bytef* content = (Bytef*)malloc(size);

        result = InitializeArchiveItemChunk(indx, chunk, length);

        if (State.Archives[archive].File.Read(content, size) != size) { return NULL; }

        uncompress((Bytef*)result, (uLongf*)&length, content, size);

        free(content);
    }

    return result;
}