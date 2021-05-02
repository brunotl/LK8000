/* Copyright (c) 2021, Bruno de Lacheisserie
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - Neither the name of nor the names of its contributors may be used to
 *   endorse or promote products derived from this software without specific
 *   prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _UTILS_FDISTREAM_H_
#define _UTILS_FDISTREAM_H_

#include <iostream>
#include <unistd.h>
#include <streambuf>
#include <cassert>

class fdbuf : public std::streambuf {
protected:
	int _file_descriptor;
	char_type _buffer[1024];

public:
	explicit fdbuf (int fd) : _file_descriptor(fd) { }

protected:
	int_type underflow () override {
		assert(_file_descriptor);
		assert(gptr() == egptr());

		// read next chunk
		ssize_t read_size = ::read(_file_descriptor, _buffer, std::size(_buffer));
		if (read_size <= 0) {
			return traits_type::eof();
		} else {
			setg(_buffer, _buffer, std::next(_buffer, read_size));
		}
		return traits_type::to_int_type(*gptr());
	}
};

class fdistream : public std::istream {
public:
	explicit fdistream (int fd) : std::istream(&_streambuf), _streambuf(fd) { }

protected:
	fdbuf _streambuf;
};

#endif // _UTILS_FDISTREAM_H_
