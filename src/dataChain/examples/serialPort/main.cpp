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

#include <esp_now.h>
#include "..\..\dataChain.h"
#include "..\..\src\SerialInOut.h"


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