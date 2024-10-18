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

#include "Content.hxx"
#include "State.hxx"

#include <direct.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_CONTENT_CHUNK_SIZE  4096

APPSTATE State;

// 0x004013e0
void InitializeArchiveItemChunks(void)
{
    State.ChunkCount = 0;

    for (unsigned i = 0; i < MAX_ARCHIVE_ITEM_CHUNK_COUNT; i++) { State.Chunks[i].Content = NULL; }
}

// 0x00401470
void ReleaseArchiveItemChunks(void)
{
    for (unsigned i = 0; i < MAX_ARCHIVE_ITEM_CHUNK_COUNT; i++)
    {
        if (State.Chunks[i].Content != NULL) { free(State.Chunks[i].Content); }
    }

    InitializeArchiveItemChunks();
}

// 0x00401d30
void Initialize(void)
{
    for (unsigned i = 0; i < MAX_ARCHIVE_ITEM_COUNT; i++) { State.Items[i].Type = ARCHIVEITEMTYPE_NONE; }
    for (unsigned i = 0; i < MAX_ARCHIVE_COUNT; i++) { State.Archives[i].IsActive = false; }

    InitializeArchiveItemChunks();
}

// 0x00401000
int main(int argc, char* argv[])
{
    int x = 1;

    {
        while (x < argc)
        {
            const char* param = argv[x];

            if (param[0] != '-') { break; }
            else if (param[1] == 'q') { State.IsSilent = true; }
            else { x = x - 1; }

            x = x + 1;
        }

        if (argc - x < 1)
        {
            printf("Syntax: %s [switches] file.sue [outdir]\n-q         Quiet (no shell output)\n", argv[0]);

            exit(EXIT_FAILURE);
        }
    }

    Initialize();

    if (!OpenArchive(argv[x]))
    {
        fprintf(stderr, "Could not open resource file: %s\n", argv[x]);

        exit(EXIT_FAILURE);
    }

    char root[MAX_PATH];

    if (argc - x < 2)
    {
        // No output folder provided.
        // Use file name, prepended by underscores, as the output folder.
        
        char path[MAX_PATH];
        strcpy(path, argv[x]);

        {
            char* end = strrchr(path, '\\');

            if (end != NULL) { strcpy(path, end + 1); }
        }

        {
            char* end = strrchr(path, '/');

            if (end != NULL) { strcpy(path, end + 1); }
        }

        sprintf(root, "__%s", path);
    }
    else
    {
        // The output folder provided.

        strcpy(root, argv[x + 1]);
    }

    mkdir(root);

    for (int i = 0; i < MAX_ARCHIVE_ITEM_COUNT; i++)
    {
        if (State.Items[i].Type == ARCHIVEITEMTYPE_NONE) { continue; }

        if (!State.IsSilent) { printf("%d %s ", State.Items[i].Type, State.Items[i].Name); }
        
        Content content;
        content.Open(State.Items[i].Name);

        unsigned size = content.Size();
        if (!State.IsSilent) { printf("%d\n", size); }

        char path[MAX_PATH];
        sprintf(path, "%s\\%s", root, State.Items[i].Name);

        const size_t len = strlen(path);

        for (size_t k = 0; k < len; k++)
        {
            if (path[k] == '/') { path[k] = '\\'; }
            if (path[k] == '\\')
            {
                path[k] = NULL;
                mkdir(path);
                path[k] = '\\';
            }
        }

        File file;
        if (!file.Open(path, (FILEOPENOPTIONS)(FILEOPENOPTIONS_CREATE | FILEOPENOPTIONS_WRITE)))
        {
            fprintf(stderr, "Cannot write %s\n", path);

            ReleaseArchiveItemChunks();

            exit(EXIT_FAILURE);
        }

        {
            unsigned len = 0;
            for (; size != 0; size = size - len)
            {
                byte data[MAX_CONTENT_CHUNK_SIZE];
                len = content.Read(data, MAX_CONTENT_CHUNK_SIZE);
                file.Write(data, len);
            }
        }

        file.Close();
        content.Close();
    }

    ReleaseArchiveItemChunks();

    return EXIT_SUCCESS;
}