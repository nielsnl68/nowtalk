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
#include "SerialInOut.h"

serialInOut::serialInOut() : m_serial(Serial) {
    m_input->setBufferSize(200);
}

//  virtual void onTriggerBegin() {}
void serialInOut::onTriggerNewData(dataChain* orgin, size_t availbleSize) {
    if (isRunning) return;
    isRunning = true;
    size_t bytes = 0;
    uint8_t buffer[100];
    while (orgin->availableForRead() > 0) {
        bytes = min((size_t)100, orgin->availableForRead());
        bytes = min(bytes, (size_t)m_serial.availableForWrite());

        bytes = orgin->readData(buffer, bytes);
        m_serial.write(buffer, bytes);
    }
    isRunning = false;
}

void serialInOut::loop() {
    uint8_t data[1];
    while (m_serial.available() > 0) {
        if (m_input->availableForWrite() >= 1) {

            data[0] = (uint8_t)m_serial.read();
            m_input->writeData(data, 1);
        }
    }
}
