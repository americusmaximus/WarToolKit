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

typedef enum FileOpenOptions
{
    FILEOPENOPTIONS_READ        = 0,
    FILEOPENOPTIONS_WRITE       = 1,
    FILEOPENOPTIONS_CREATE      = 2,
    FILEOPENOPTIONS_FORCE_DWORD = 0x7FFFFFF
} FILEOPENOPTIONS, * FILEOPENOPTIONSPTR;

class File
{
public:
    File() { Handle = INVALID_HANDLE_VALUE; }
    bool CLASSCALL Open(const char* path, const FILEOPENOPTIONS options);
    void CLASSCALL Close(void);
    unsigned CLASSCALL Read(void* content, const unsigned size);
    unsigned CLASSCALL Write(void* content, const unsigned size);
    void CLASSCALL SetPosition(const int offset, const int mode);
    unsigned CLASSCALL Size();
public:
    HANDLE Handle;
};