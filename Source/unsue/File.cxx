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

#include "File.hxx"

// 0x00401d80
bool CLASSCALL File::Open(const char* path, const FILEOPENOPTIONS options)
{
    unsigned access = GENERIC_READ;
    unsigned mode = FILE_SHARE_READ;

    if (mode & FILEOPENOPTIONS_WRITE)
    {
        access = access | GENERIC_WRITE;
        mode = mode | FILE_SHARE_WRITE;
    }

    this->Handle = CreateFileA(path, access, mode, NULL,
        (options & FILEOPENOPTIONS_CREATE) ? CREATE_ALWAYS : OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    return this->Handle != INVALID_HANDLE_VALUE;
}

// 0x00401de0
void CLASSCALL File::Close(void)
{
    CloseHandle(this->Handle);

    this->Handle = INVALID_HANDLE_VALUE;
}

// 0x00401e00
unsigned CLASSCALL File::Read(void* content, const unsigned size)
{
    unsigned result = size;

    ReadFile(this->Handle, content, size, (DWORD*)&result, NULL);

    return result;
}

// 0x00401e30
unsigned CLASSCALL File::Write(void* content, const unsigned size)
{
    unsigned written = size;
    WriteFile(this->Handle, content, size, (DWORD*)&written, NULL);
    return size;
}

// 0x00401e60
void CLASSCALL File::SetPosition(const int offset, const int mode)
{
    SetFilePointer(this->Handle, offset, NULL, mode);
}

// 0x00401e80
unsigned CLASSCALL File::Size()
{
    return GetFileSize(this->Handle, NULL);
}