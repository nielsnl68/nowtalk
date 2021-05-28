/*
  MIT License

  Copyright (c) 2021 Spectronix NL

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
#include <Arduino.h>

/**
 * @brief Destination for audio data
 *
 */
class dataBuffer {
public:
    dataBuffer(size_t buffer_size);  // Constructor

    size_t getBufferSize();
    size_t getReadPos();
    size_t getWritePos();

    uint8_t* debug();

    size_t availableToWrite();
    size_t availableToRead();
    
    size_t write(uint8_t* data, size_t size);

    size_t read(uint8_t* data, size_t size);
private:
    // double buffer so we can be capturing samples while sending data
    uint8_t* m_buffer;
    size_t m_readPosition = 0;
    size_t m_writePosition = 0;
    size_t m_bufferSize = 0;
    bool m_locked = false;
};
