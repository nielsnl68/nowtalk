# nowTalk - An intercom system over ESP-NOW
This would be my approach of building a commbadge system. using esp32 and some i2s modules. 

## Insperation
Inspire by @atomic14 i started to build my own version of your walkytalky concept. I always love to build a commbadge like in startrek/B5 etc. 
I bought a couple of i2s devices like the VS1052, the stgl5000 and WM8960  to see which one the best performs.  I like the VS1052 because it has a couple of useful codec build into it. but we will see.
I have been thinking to do this for yours already but did not found the right way to implement it. Tried bluetooth or real internet but none where as simple i wanted. Until you shown us the ESP-NOW solution, and i loving it a lot. 

The last couple of days i ordered a lot of ESP32 devices to setup a network around esp-now and i love it, so easy to setup. The main idea is to have a master switch-board that that can listen to the user and has a Voice control future  to direct the caller to an other commbadge within the same network or outside depending where the called person is. 

At this moment i'm working on a lite with protocol that allows me to setup the switchboard via serial and webserver page. In addition i have setup a espnow protocol that alows to ping the switchboard, getting some network info, allowing to do OTA over esp-now (Not tested yet but the idea is there) and start and except call's. 

The use of the commbadge needs to be as easy as possible just by touching the commbadge sould go active and talk to the switch board. After done a single press sould be enough to close the connection (no push to talk btw).

## Infrastucture NowTalk
The nowTalk ego system exist of s switchboard (master) and up to 20 commbadges. Each ego system can talk to other ego systems when a guest commbadge enters that ego system. The switchboard will then working as a serving hatch between those two ego systems. As soon a commbadge enters an other ego system that swichboard will notify the owners swichboard of his pressens. This is useful because when someone needs to talk to that commbasge the owners swichboard know who to reach etc.
In the phonebook you can store the local commbadges,  commbadges from friends and commbadges that you do not want to give access to the network.
By just pressing the commbadge a audio link is opened to the local switchboard and say the name of the other commbadge or give the switchboard some instructions in home automation etc. When the swichboard find the name in the phonebook it will contact the other commbadge to notify the request. 
The calling commbadge user just need to press his commbadge and say "reject", "okey"  when okey the connection is setup between the two combadges until one off them pressing his commbadge again and say "done".

### The commbadge
also named "client" 
#### Hardware
The commbadge is build around a ESP32 with a I2S codec onboard (Like the ESP32-A1S) or an ESP32 with externel I2S devices like the SGTL5000, wm8960 or  VS1053 etc. or an other setup we did not have thought about up yet or tested.
The badge will be activated by using capisit touch. So no real buttons needed. Optional it could have a small oled display or RGB led to tell the status. 
For audio communication the device will have his own mic and speaker. But can also be used with a audio headset, the button will then also function of start the communication. 

### The Switchboard
Also named "Master"
#### Hardware
The switchboard existes of  a ESP32 that will function as bridge between the ESP-NOW protocal  and a Raspberry PI.
The PI will work as the communication manager and runs on nodejs as server setup. As said above it will hold the  "phonebook" with all known commbadges within his own ego system and will also manage commbadges from other ego system.

## contribute
I would love to work together on this project when you are interested, let me know. In the main time i have setup this github where i placed everything i have so far.

## esp-now protocol codes
The ESP-NOW message exist about minimal 2 bytes. the first gives the size of the message the second the instuction code.
Some instructions will include also extra information. The maximal size of a message cant be above 200 tokens incl. size and code byte.

Code | Discription                              | Transfer                    | Remark
---- | ---------------------------------------- | --------------------------- | ---------
0x01 | Ping the server                          | client to server/broadcast |
0x02 | Pong from server                         | server to client |
0x03 | Reguest client details (new in network)  | server to client |
0x04 | Guest use request                        | client to server | incl. name and IP
0x05 | New device                               | client to server |
---- |   |   |
0x07 | Accept new client                        | server to client | incl. new name and IP
---- |   |   |
0x0d | Update username                          | server to client | old and new username
0x0e | Update owner IP (used in other network)  | server to client | old and new IP
---- |   |   |
0x10 | Ack update                               | client to server | 
0x11 | Nack update                              | client to server | 
---- |   |   |
0x30 | Start call request                       | client to server | 
0x31 | Ack start speaking                       | server to client | 
0x32 | System busy wait a moment                | server to client | 
0x33 | Deliver other mac address                | server to client | mac from other client
0x34 | Other client not available               | server to client |
---- |   |   |
0x37 | Request call                             | client to client |
0x38 | Receive call                             | client to client |
0x39 | Close call/call rejected                 | client to client | 
---- |   |   |
0x3f | Voice package send / received.           | server/client to client/server |
---- |   |   |
0x40 | Client peered on extern network          | network to server |
0x4f | Client left from extern network          | network to server |
---- |   |   |
0xe0 | Start update firmware                    | server to broadcast | size of file,checksum of file
0xe1 | Upload update firmware                   | server to broadcast | package counter, fileblock 
0xe2 | Ack update now                           | client to erver |
0xe3 | No Ack file was not correct  | client to erver |
---- |   |    |
0xff | Alarm help needed.                       | client to broadcast |

### Alarm protocol
The Alarm function will be started when the user hold the commbadge for longer the 3 second until he will release it again.  All commbadges around hem will be notified. 
