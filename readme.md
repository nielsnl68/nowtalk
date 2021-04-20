# espnow-commbadge
This would be my approach of building a commbadge system. using esp32 and some i2s modules. 


## Insperation
Inspire by @atomic14 i started to build my own version of your walkytalky concept. I always love to build a commbadge like in startrek/B5 etc. 
I bought a couple of i2s devices like the VS1052, the stgl5000 and WM8960  to see which one the best performs.  I like the VS1052 because it has a couple of useful codec build into it. but we will see.
I have been thinking to do this for yours already but did not found the right way to implement it. Tried bluetooth or real internet but none where as simple i wanted. Until you shown us the ESP-NOW solution, and i loving it a lot. 

The last couple of days i ordered a lot of ESP32 devices to setup a network around esp-now and i love it, so easy to setup. The main idea is to have a master switch-board that that can listen to the user and has a Voice control future  to direct the caller to an other commbadge within the same network or outside depending where the called person is. 

At this moment i'm working on a lite with protocol that allows me to setup the switchboard via serial and webserver page. In addition i have setup a espnow protocol that alows to ping the switchboard, getting some network info, allowing to do OTA over esp-now (Not tested yet but the idea is there) and start and except call's. 

The use of the commbadge needs to be as easy as possible just by touching the commbadge sould go active and talk to the switch board. After done a single press sould be enough to close the connection (no push to talk btw).

## esp-now protocol codes

Request message codes:

Code | Discription                              | Transfer
---- | ---------------------------------------- | -----------------------------
0x01 | Ping Pong Master returns with 0x01       | client to server/broadcast
0x02 | Pong Master notification                 | server to client
0x03 | Reguest client details (new in network)  | server to client
0x04 | send orginal network IP and username     | client to server
0x05 | New device no info available             | client to server
---- | ---------------------------------------- | -----------------------------
0x08 | reject new client                        | server to client
0x09 | Accept new client                        | server to client
---- | ---------------------------------------- | -----------------------------
0x0d | update username                          | server to client
0x0e | update owner IP (used in other network)  | server to client
0x0f | Ack update owner IP                      | client to server
---- | ---------------------------------------- | -----------------------------
0x10 | start reconnect to other client          | client to server
0x11 | deliver other mac address                | server to client
0x12 | other client not available               | server to client
---- | ---------------------------------------- | -----------------------------
0x17 | Request call                             | client to client
0x18 | Receive call                             | client to client
0x19 | close call/call rejected                 | client to client
---- | ---------------------------------------- | -----------------------------
0x20 | voice package send / received.           | server/client to client/server
---- | ---------------------------------------- | -----------------------------
0xe0 | start update firmware                    | server to broadcast
0xe1 | upload update firmware                   | server to broadcast
0xe2 | Ack update now                           | client to erver
---- | ---------------------------------------- | -----------------------------
0xff | Alarm help needed.                       | client to broadcast



## contribute
I would love to work together on this project when you are interested, let me know. In the main time i have setup this github where i placed everything i have so far.
