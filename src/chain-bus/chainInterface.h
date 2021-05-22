#include<Arduino.h>
#include "buffer.h"


class Chain;

class Interface {
protected:
    Chain* m_chain = nullptr;
    virtual Chain* chain(Chain* next = nullptr)
    {
        if (next != nullptr)
            m_chain->setNext(next);
        return m_chain;
    };
    virtual void in(Chain* next) { chain(next); };
    virtual Chain* out() { return chain(); };

public:
    Interface() {
        m_chain = new Chain(*this);
    };
    ~Interface() {
        delete m_chain;
    }

    // the following function will be called by the chain class
    virtual void  triggerReadAvailable(size_t size) {   }
    virtual size_t readAvailable() {}
    virtual size_t readData(uint8_t* data, size_t size) { return 0; }
    
};

class Chain {
private:
    Interface& m_interface;
    Chain* m_prev = nullptr;
    Chain* m_next = nullptr;
protected:

public:
    Chain(Interface& interface) : m_interface(interface) {}
    ~Chain() {
        // delete m_interface;
        setNext(nullptr);
        setPrev(nullptr);
    }

    void setPrev(Chain* prev) {
        if (m_prev == prev) return;
        if (m_prev) {
            m_prev->setNext(nullptr);
        }
        m_prev = prev;
        if (m_prev) {
            m_prev->setNext(this);
        }
    }

    void setNext(Chain* next) {
        if (m_next == next) return;
        if (m_next) {
            m_next->setPrev(nullptr);
        }
        m_next = next;
        if (m_next) {
            m_next->setPrev(this);
        }
    }

    Interface getImpl() { return m_interface; }
    // the following function will be called by the chain class
    virtual void  triggerDataAvailable(size_t size) {
        if (m_next) {
            m_next->getImpl().triggerDataAvailable(size);
        }
    }
    virtual size_t dataAvailable() {
        if (m_prev) {
            return m_prev->getImpl().dataAvailable();
        }
        return 0;
    }
    virtual size_t readData(uint8_t* data, size_t size) {
        if (m_prev) {
            return m_prev->getImpl().readData(data, size);
        }
        return 0;
    }

    
};

class Impl : public Interface
{
private:
    char* m_name = "";

public:
    Impl(char* name) {
        m_name = name;
    }
    virtual void callback() {
        Serial.printf("%8s", m_name);
    }
    void display() {
        m_chain->displayAll();
    }
    
};