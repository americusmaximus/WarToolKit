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
#include "Content.hxx"

// 0x00401720
bool CLASSCALL Content::Open(const char* name)
{
    this->Index = AcquireArchiveItemIndex(name);

    if (this->Index == INVALID_ARCHIVE_ITEM_INDEX) { return false; }

    return OpenArchiveItem(this->Index);
}

// 0x00401ce0
unsigned CLASSCALL Content::Size()
{
    if (!this->IsAvailable()) { return 0; }

    return ArchiveItemSize(this->Index);
}

// 0x00401890
bool CLASSCALL Content::IsAvailable()
{
    return IsArchiveItemAvailable(this->Index);
}

// 0x00401950
unsigned CLASSCALL Content::Read(void* content, unsigned size)
{
    if (!this->IsAvailable()) { return false; }

    return ReadArchiveItem(this->Index, content, size);
}

// 0x00401900
void CLASSCALL Content::Close()
{
    if (this->IsAvailable())
    {
        CloseArchiveItem(this->Index);
    }
}