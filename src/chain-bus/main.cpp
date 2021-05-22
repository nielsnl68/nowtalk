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
#include "chainInterface.h"

void setup()
{

    Serial.begin(115200);
    delay(1000);
    Serial.println("_---_");
    Impl a("Anton");
    a.display();
    Serial.println("add Bernard:");

    Impl b("Bernard");
    a.testing(b.testing());
    a.display();
    b.display();

    Serial.println("add Coranel:");
    Impl c("Coranel");
    b.testing(c.testing());
    a.display();
    b.display();
    c.display();

    Serial.println("clear test:");
    c.testing()->clear();
    a.display();
    b.display();
    c.display();

    Serial.println("replace Daniel with Bernard:");
    Impl d("Daniel");
    a.testing(d.testing());
    a.display();
    b.display();
    c.display();
    d.display();
}

void loop()
{
    // put your main code here, to run repeatedly:
}