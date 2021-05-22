#pragma once
#include <Arduino.h>

typedef void (*I2S_buffer_cb_t)(const String orgin, const size_t  request_size, size_t availbleSize);
/**
 * @brief Destination for audio data
 *
 */
class buffer
{
private:
    // double buffer so we can be capturing samples while sending data
    uint8_t* m_buffer;
    uint32_t m_readPosition = 0;
    uint32_t m_writePosition = 0;
    uint32_t m_bufferSize = 0;
    bool m_locked = false;
    String m_bufferName = "";
    I2S_buffer_cb_t m_buffer_cb;

    void call_callback(String Orgin, size_t x, size_t y) {
        if (m_buffer_cb != nullptr) {
            m_buffer_cb(m_bufferName + "_" + Orgin, x, y);
        }
    }

public:
    buffer(size_t buffer_size, String bufferName = "") {
        m_locked = true;
        m_bufferName = bufferName;
        m_bufferSize = buffer_size;
        m_buffer = (uint8_t*)malloc(m_bufferSize);
        memset(m_buffer, 0x30, m_bufferSize);
        m_locked = false;
    }

    int32_t get_bufferSize() {
        return m_bufferSize;
    }

    int32_t get_readPos() {
        return m_readPosition;
    }

    int32_t get_writePos() {
        return m_writePosition;
    }

    uint8_t* debug() {
        return m_buffer;
    }

    uint32_t availableToWrite() {
        if (m_readPosition <= m_writePosition) {
            return (m_readPosition + m_bufferSize) - m_writePosition;
        }
        else {
            return  (m_readPosition - m_writePosition);
        }
    }
    uint32_t availableToRead()
    {
        if (m_writePosition < m_readPosition) {
            return (m_writePosition + m_bufferSize) - m_readPosition;
        }
        else {
            return  (m_writePosition - m_readPosition);
        }
    }

    size_t write(uint8_t* data, size_t size)
    {
        while (m_locked) {};
        m_locked = true;
        if (size > availableToWrite()) {
            call_callback("write", size, availableToWrite());
            size = availableToWrite();
        }
        if (m_writePosition == m_bufferSize) m_writePosition = 0;
        uint32_t oldPosition = m_writePosition;
        m_writePosition = (m_writePosition + size) % (m_bufferSize + 1);

        uint32_t topsize = 0;
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
    };

    size_t read(uint8_t* data, size_t size)
    {

        while (m_locked) {};
        m_locked = true;

        if (size > availableToRead()) {
            call_callback("read", size, availableToRead());
            size = availableToRead();
        }
        if (m_readPosition == m_bufferSize) m_readPosition = 0;
        uint32_t oldPosition = m_readPosition;
        m_readPosition = (m_readPosition + size) % (m_bufferSize + 1);

        uint32_t topsize = 0;
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
    void set_buffer_cb(I2S_buffer_cb_t cb) {
        m_buffer_cb = cb;
    }
};
