/*
 * the following sketch is a proto of a chainable class system.
 * with on function in the Impl class you can hook that class
 * to his predecessor class.
 * This system will be used to create a new I2S library where
 * you can link (chain) different proccesses together to receive
 * sound from different sources decode/encode them end send the
 * result out again.
 * This example shows also a way to interact with each other with
 * the use of an interface class.
 */
#include <Arduino.h>
#include "dataChain.h"
#include <esp_now.h>

class serialInOut : public InputOutputInterface {
private:
    HardwareSerial& m_serial;
    boolean isRunning = false;
public:
    serialInOut() :m_serial(Serial) {

    }

  //  virtual void onTriggerBegin() {}
    virtual void onTriggerNewData(dataChain * orgin, size_t availbleSize) {
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

    void loop() {
        uint8_t data[1];
        while (m_serial.available() > 0) {
            if (m_input->availableForWrite() >= 1) {

                data[0] = (uint8_t)m_serial.read();
                m_input->writeData(data, 1);
            }
        }
    }
};

serialInOut test;

void setup()
{
    Serial.begin(115200);
    test.input(test.output());
    test.begin();

}

void loop()
{
    test.loop();
    // put your main code here, to run repeatedly:
}