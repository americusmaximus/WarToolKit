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

#include "Archive.hxx"
#include "State.hxx"

#include <io.h>
#include <zlib.h>

#define SAVE_SIZE_MODIFIER  0x8000

// 0x00401000
void InitializeArchives(void) {
    State.Names.Next = State.Names.Names;

    State.Archive.Index = 0;
    State.Archive.Count = 0;
}

// 0x00401020
void ArchiveFile(const char* path, const char* name, int block) {
    void* file = State.SkipExtraction
        ? (void*)fopen(path, "rb") : (void*)gzopen(path, "rb");

    if (file == NULL) {
        fprintf(stderr, "ERROR: could not open %s\n", path);
        return;
    }

    size_t read = 0, write = 0;

    if (block == 0 || State.Compression == Z_NO_COMPRESSION) {
        State.Items[State.Archive.Count].Type = ARCHIVEITEMTYPE_PACKED;
        State.Items[State.Archive.Count].Offset = ftell(State.Archive.File);

        while (true) {
            const int end = State.SkipExtraction
                ? feof((FILE*)file) : gzeof((gzFile)file);

            if (end) { break; }

            const size_t size = State.SkipExtraction
                ? fread(State.Content.In, 1, MAX_CONTENT_IN_SIZE, (FILE*)file)
                : gzread((gzFile)file, State.Content.In, MAX_CONTENT_IN_SIZE);

            fwrite(State.Content.In, 1, size, State.Archive.File);

            read = read + size;
        }

        write = read;
    }
    else {
        State.Items[State.Archive.Count].Type = ARCHIVEITEMTYPE_COMPRESSED;
        State.Items[State.Archive.Count].Offset = State.Archive.Index;

        while (true) {
            const int end = State.SkipExtraction
                ? feof((FILE*)file) : gzeof((gzFile)file);

            if (end) { break; }

            const size_t size = State.SkipExtraction
                ? fread(State.Content.In, 1, block, (FILE*)file)
                : gzread((gzFile)file, State.Content.In, block);

            read = read + size;

            uLong length = MAX_CONTENT_OUT_SIZE;
            compress2(State.Content.Out, &length, State.Content.In, (uLong)size, State.Compression);

            State.Offsets[State.Archive.Index] = ftell(State.Archive.File);
            State.Archive.Index = State.Archive.Index + 1;

            fwrite(State.Content.Out, 1, length, State.Archive.File);

            write = write + length;
        }

        State.Offsets[State.Archive.Index] = ftell(State.Archive.File);
        State.Archive.Index = State.Archive.Index + 1;
    }

    if (State.SkipExtraction) { fclose((FILE*)file); }
    else { gzclose((gzFile)file); }

    State.Items[State.Archive.Count].Chunk = block;
    State.Items[State.Archive.Count].Name = (unsigned)(State.Names.Next - State.Names.Names);

    const size_t len = strlen(name);
    strcpy(State.Names.Next, name);

    State.Names.Next = State.Names.Next + len + 1;
    State.Items[State.Archive.Count].Size = (unsigned)read;

    State.Archive.Count = State.Archive.Count + 1;
    State.Archive.Size = State.Archive.Size + (unsigned)write;

    if (!State.IsSilent) { printf("%s->%s %d->%d\n", path, name, (unsigned)read, (unsigned)write); }
}

// 0x004013d0
void Save(const void* data, const unsigned count, const unsigned size) {
    uLongf length = count * size + SAVE_SIZE_MODIFIER;

    Bytef* content = (Bytef*)malloc(length);
    compress(content, &length, (Bytef*)data, count * size);

    fwrite(&length, 1, sizeof(uLongf), State.Archive.File);
    fwrite(&count, 1, sizeof(unsigned), State.Archive.File);
    fwrite(&size, 1, sizeof(unsigned), State.Archive.File);
    fwrite(content, 1, length, State.Archive.File);

    free(content);
}

// 0x00401530
void ArchivePath(const char* path, const char* name, const char* pattern, const int block, const int subdirs, const int flatten) {
    if (!State.IsSilent) { printf("Adding %s; blocksize=%d\n", path, block); }

    // File
    {
        const size_t length = strlen(path);

        if (path[length - 1] != '\\') {
            if (path[length - 1] != '/') {
                size_t end = length - 2;

                while (path[end] != '\\' && path[end] != '/') { end = end - 1; }

                ArchiveFile(path, &path[end + 1], block);

                return;
            }
        }
    }

    // Directory
    {
        char file[MAX_FILE_NAME_LENGTH];
        sprintf(file, "%s%s", path, pattern);

        _finddata_t context;
        intptr_t handle = _findfirst(file, &context);

        if (handle == -1) {
            if (errno == ENOENT) {
                fprintf(stderr, "Warning: no files in %s\n", path);
                return;
            }

            fprintf(stderr, "Error: bad path %s\n", path);
            exit(EXIT_FAILURE);
        }

        char dir[MAX_FILE_NAME_LENGTH];
        char tag[MAX_FILE_NAME_LENGTH];

        do {
            if (!(context.attrib & _A_SUBDIR)) {

                sprintf(dir, "%s%s", path, context.name);
                sprintf(tag, "%s%s", name, context.name);

                ArchiveFile(dir, tag, block);
            }
            else {
                if (strcmp(context.name, ".") != 0) {
                    if (strcmp(context.name, "..") != 0 && subdirs) {
                        sprintf(dir, "%s%s\\", path, context.name);

                        if (!flatten) { sprintf(tag, "%s%s\\", name, context.name); }
                        else { strcpy(tag, name); }

                        ArchivePath(dir, tag, pattern, block, subdirs, flatten);
                    }
                }
            }
        } while (_findnext(handle, &context) == 0);

        _findclose(handle);
    }
}