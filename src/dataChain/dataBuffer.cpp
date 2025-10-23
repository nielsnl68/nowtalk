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

#include <Arduino.h>
#include "dataBuffer.h"
  /**
   * @brief Destination for audio data
   *
   */
dataBuffer::dataBuffer(size_t buffer_size) {
    m_locked = true;
    m_bufferSize = buffer_size;
    m_buffer = (uint8_t*)malloc(m_bufferSize);
    memset(m_buffer, 0x30, m_bufferSize);
    m_locked = false;
}

size_t dataBuffer::getBufferSize() {
    return m_bufferSize;
}

size_t dataBuffer::getReadPos() {
    return m_readPosition;
}

size_t dataBuffer::getWritePos() {
    return m_writePosition;
}

uint8_t* dataBuffer::debug() {
    return m_buffer;
}

size_t dataBuffer::availableToWrite() {
    if (m_readPosition <= m_writePosition) {
        return (m_readPosition + m_bufferSize) - m_writePosition;
    }
    else {
        return  (m_readPosition - m_writePosition);
    }
}

size_t dataBuffer::availableToRead()
{
    if (m_writePosition < m_readPosition) {
        return (m_writePosition + m_bufferSize) - m_readPosition;
    }
    else {
        return  (m_writePosition - m_readPosition);
    }
}

size_t dataBuffer::write(uint8_t* data, size_t size)
{
    while (m_locked) {}
    m_locked = true;

    if (m_writePosition == m_bufferSize) m_writePosition = 0;
    size_t oldPosition = m_writePosition;
    m_writePosition = (m_writePosition + size) % (m_bufferSize + 1);

    size_t topsize = 0;
    size_t oldSize = size;

    if (size + oldPosition >= m_bufferSize)
    {
        topsize = (size + oldPosition) - m_bufferSize;
        size -= topsize;
    }
    memcpy(&m_buffer[oldPosition], &data[0], size);
    if (topsize > 0)
    {
        memcpy(&m_buffer[0], &data[size], topsize);
    }
    m_locked = false;
    return oldSize;
}

size_t dataBuffer::read(uint8_t* data, size_t size)
{
    while (m_locked) {}
    m_locked = true;

    if (m_readPosition == m_bufferSize) m_readPosition = 0;
    size_t oldPosition = m_readPosition;
    m_readPosition = (m_readPosition + size) % (m_bufferSize + 1);

    size_t topsize = 0;
    size_t oldSize = size;

    if (size + oldPosition >= m_bufferSize)
    {
        topsize = (size + oldPosition) - m_bufferSize;
        size -= topsize;
    }
    memcpy(&data[0], &m_buffer[oldPosition], size);
    if (topsize > 0)
    {
        memcpy(&data[size], &m_buffer[0], topsize);
    }

    m_locked = false;
    return oldSize;
}