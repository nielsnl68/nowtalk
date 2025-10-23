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


#include "Arduino.h"
#include "dataChain.h"

dataChain::dataChain(Interface& interface) : m_interface(interface) {}
dataChain::~dataChain() {
    // delete m_interface;
    setNext(nullptr);
    setPrev(nullptr);
}

void dataChain::begin() {
    m_interface.onTriggerBegin();
    if (m_next) {
        if (m_triggerSize > m_bufferSize) { m_bufferSize = m_triggerSize * 2; }
        m_buffer = new dataBuffer(m_bufferSize);
        m_next->begin();
    }
}

void dataChain::setTriggerSize(size_t size) {
    m_triggerSize = size;
}

void dataChain::callTrigger(dataChain* prevChain, size_t size) {

    m_interface.onTriggerNewData(prevChain, size);
}

void dataChain::setPrev(dataChain* prev) {
    if (m_prev == prev) return;
    if (m_prev) {
        m_prev->setNext(nullptr);
    }
    m_prev = prev;
    if (m_prev) {
        m_prev->setNext(this);
    }
}

void dataChain::setNext(dataChain* next) {
    if (m_next == next) return;
    if (m_next) {
        m_next->setPrev(nullptr);
    }
    m_next = next;
    if (m_next) {
        m_next->setPrev(this);
    }
}

void dataChain::setBufferSize(size_t size) {
    m_bufferSize = size;
}

size_t dataChain::availableForWrite() {
    if (m_buffer != nullptr) {
        return m_buffer->availableToWrite();
    }
    return 0;
}

size_t dataChain::writeData(uint8_t* data, size_t size) {
    size_t  writen = 0;
    if (m_buffer != nullptr) {
        writen = m_buffer->write(data, size);
        size_t newSize = m_buffer->availableToRead();

        if ((newSize >= m_triggerSize) && (m_next != nullptr)) {
            m_next->callTrigger(this, newSize);
        }
    }
    return writen;
}

size_t dataChain::availableForRead() {
    if (m_buffer != nullptr) {
        return m_buffer->availableToRead();
    }
    return 0;
}

size_t dataChain::readData(uint8_t* data, size_t size) {
    if (m_buffer != nullptr) {
        return m_buffer->read(data, size);
    }
    return 0;
}

/***********************************************************
 * Default base Interfaces for in-/output and conversion tasks
 ***********************************************************/

InputInterface::InputInterface() {
    m_input = new dataChain(*this);
}

InputInterface ::~InputInterface() {
    delete m_input;
}

void InputInterface::input(dataChain* next)
{
    m_input->setNext(next);
}

void InputInterface::begin() {
    m_input->begin();
}

/***********************************************************/

OutputInterface::OutputInterface() {
    m_output = new dataChain(*this);
}

OutputInterface::~OutputInterface() {
    delete m_output;
}

dataChain* OutputInterface::output() {
    return m_output;
}

/***********************************************************/

InputOutputInterface::InputOutputInterface() {
    m_input = new dataChain(*this);
    m_output = new dataChain(*this);
}

InputOutputInterface::~InputOutputInterface() {
    delete m_input;
    delete m_output;
}

void InputOutputInterface::begin() {
    m_input->begin();
}

void InputOutputInterface::input(dataChain* next)
{
    m_input->setNext(next);
}

dataChain* InputOutputInterface::output()
{
    return m_output;
}

/***********************************************************/

CoverterInterface::CoverterInterface() {
    m_input = new dataChain(*this);
    m_output = new dataChain(*this);
}

CoverterInterface::~CoverterInterface() {
    delete m_input;
    delete m_output;
}

dataChain* CoverterInterface::encode(dataChain* next) {
    if (next != nullptr)
        m_input->setNext(next);
    return m_input;
}

dataChain* CoverterInterface::decode(dataChain* next) {
    if (next != nullptr)
        m_output->setNext(next);
    return m_output;
}