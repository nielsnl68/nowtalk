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


#include<Arduino.h>
#include "dataBuffer.h"

class Interface;

class dataChain {
public:
    dataChain(Interface& interface) ;
    ~dataChain();
    
    void begin();

    void setTriggerSize(size_t size);
    void callTrigger(dataChain* prevChain, size_t size);

    void setPrev(dataChain* prev);
    void setNext(dataChain* next);
    
    void setBufferSize(size_t size);
    
    size_t availableForWrite();
    size_t writeData(uint8_t* data, size_t size);
    
    size_t availableForRead();
    size_t readData(uint8_t* data, size_t size);

private:
    Interface& m_interface;
    dataChain* m_prev = nullptr;
    dataChain* m_next = nullptr;
    dataBuffer* m_buffer = nullptr;
    size_t  m_triggerSize = 1;
    size_t m_bufferSize = 100;
};


/*
*   Below are some example base Interfaces to make the real   
*   classes easy to interface with each other.
*/

class Interface {
public:
    virtual void onTriggerNewData(dataChain* orgin, size_t availbleSize) { };
    virtual void onTriggerBegin() {};
};

class InputInterface : public  Interface {
public:
    InputInterface();
    ~InputInterface();

    void input(dataChain* next);
    virtual void begin();

protected:
    dataChain* m_input = nullptr;
};

class OutputInterface : public  Interface {
public:
    OutputInterface( );
    ~OutputInterface();

    dataChain* output();
protected:
    dataChain* m_output = nullptr;
};

class InputOutputInterface : public  Interface {
public:
    InputOutputInterface();
    ~InputOutputInterface();
    virtual void begin();

    void input(dataChain* next);
    dataChain* output();
protected:
    dataChain* m_output = nullptr;
    dataChain* m_input = nullptr;
};

class CoverterInterface : public  Interface {
public:
    CoverterInterface();
    ~CoverterInterface();

    dataChain* encode(dataChain* next);
    dataChain* decode(dataChain* next);
protected:
    dataChain* m_output = nullptr;
    dataChain* m_input = nullptr;
};