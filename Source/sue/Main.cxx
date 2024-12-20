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

#define USAGE_TEXT_MESSAGE \
    "Syntax: %s [switches] file.sue name1 [name2 ...]\n-q         Quiet (no output)\n-m<n>      Compression level=<n>, 0-no compression, 1-fast, 9-best(default)\n-n         Disable pre-decompressing of gzip comressed files\n-b<nnn>    Compression block size=<nnn>, default=16384\n-s         Do not compress subdirectories\n-f         Flatten directory structure\nName can stand for a file or a directory.\nDirectory names should end with a backslash.\n"

APPSTATE State;

void Initialize() {
    State.Compression = Z_BEST_COMPRESSION;

    State.Names.Next = State.Names.Names;
}

// 0x00401340
void Initialize(const char* name) {
    InitializeArchives();
    State.Archive.File = fopen(name, "wb");

    if (State.Archive.File == NULL) {
        fprintf(stderr, "ERROR: could not create output file %s\n", name);
        exit(EXIT_FAILURE);
    }

    if (!State.IsSilent) { printf("Compressing to %s\n\n", name); }

    const unsigned value = ARCHIVE_MAGIC;

    fwrite(&value, 1, sizeof(unsigned), State.Archive.File);
    fwrite(&value, 1, sizeof(unsigned), State.Archive.File);
}

// 0x00401470
void Release() {
    // File length less 8 bytes:
    // 1. 4 bytes for the magic.
    // 2. 4 bytes for the actual content size value.
    const long offset = ftell(State.Archive.File) - (2 * sizeof(unsigned));

    Save(State.Items, State.Archive.Count, sizeof(ARCHIVEITEMDESCRIPTOR));
    Save(State.Names.Names, 1, (unsigned)(State.Names.Next - State.Names.Names));
    Save(State.Offsets, State.Archive.Index, sizeof(unsigned));

    fseek(State.Archive.File, sizeof(unsigned), SEEK_SET);
    fwrite(&offset, 1, sizeof(unsigned), State.Archive.File);

    if (!State.IsSilent) {
        printf("\n");
        printf("Total files:                             %d\n", State.Archive.Count);
    }

    fclose(State.Archive.File);

    State.Archive.Count = 0;
}

// 0x004017f0
int main(int argc, char* argv[]) {
    Initialize();

    int block = DEFAULT_BLOCK_SIZE;
    int flatten = FALSE, subdirs = TRUE;

    int indx = 1;
    for (; indx < argc; indx++) {
        if (argv[indx][0] != '-') { break; }

        switch (argv[indx][1]) {
        case 'b': {
            block = atoi(&argv[indx][2]);

            if (block < 0) { block = 0; }

            break;
        }
        case 'f': { flatten = TRUE; break; }
        case 'm': {
            State.Compression = atoi(&argv[indx][2]);
            State.Compression = min(Z_BEST_COMPRESSION, max(Z_NO_COMPRESSION, State.Compression));

            break;
        }
        case 'n': { State.SkipExtraction = TRUE; break; }
        case 'q': { State.IsSilent = TRUE; break; }
        case 's': { subdirs = 0; break; }
        default: { indx = argc - 1; break; }
        }
    }

    if (argc - indx < 2) {
        printf(USAGE_TEXT_MESSAGE, *argv);
        exit(EXIT_FAILURE);
    }

    Initialize(argv[indx]);

    for (indx = indx + 1; indx < argc; indx++) { ArchivePath(argv[indx], "", "*", block, subdirs, flatten); }

    Release();

    return EXIT_SUCCESS;
}