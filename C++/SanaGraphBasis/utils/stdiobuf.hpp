// This software is part of github.com/waynebhayes/libwayne, and is Copyright(C) Wayne B. Hayes 2025, under the GNU LGPL 3.0
// (GNU Lesser General Public License, version 3, 2007), a copy of which is contained at the top of the repo.
#ifndef STDIOBUF_HPP
#define STDIOBUF_HPP

#include <streambuf>

#define BUFFER_SIZE 10240
// This class is needed to convert a C-style FILE* to a C++-style stream to make
// popen compatible with the large portions of SANA that use C++ to read files
//*** NEW COMMENT***
// now in libwayne for use with other projects such as BLANT [Akhil]
class stdiobuf : public std::streambuf {
private:
    FILE* file;
    char buffer[BUFFER_SIZE];
    bool isPiped = false;
public:
    stdiobuf(FILE* file, bool piped): file(file), isPiped(piped) {}
    ~stdiobuf() {
        if (file) {
            if (isPiped) pclose(file);
            else fclose(file); 
        }
    }
    stdiobuf(const stdiobuf& other) {
        if (&other != this) {
            this->file = other.file;
            this->isPiped = other.isPiped;
            for(int i = 0; i < BUFFER_SIZE; i++)
                this->buffer[i] = other.buffer[i];
        }
    }

    int underflow() {
        if (gptr() == egptr() and file) {
            size_t size = fread(buffer, 1, BUFFER_SIZE, file);
            if (0 < size) setg(buffer, buffer, buffer + size);
        }
        return (gptr() == egptr() ? traits_type::eof()
                                  : traits_type::to_int_type(*gptr()));
    }
};

#endif /* STDIOBUF_HPP */
