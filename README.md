# aj_ivr_freeswitch_using-esl
Freeswitch IVR development using esl

I have given 2 solutions here in this repo for achieving the objective

1st :-> There will be xml dialplan which will connect user to freeswitch(freeswitch answers the call) , then a lua script will be called from the same xml dialplan and in that lua script I am connecting to esl and willing to use it to generate custom events and then I have created an IVR which will be played to the user. 
Files for this method : 
a: aj_esl_connect_ivr_first_method.lua
b: aj_ivr_dialplan_first_method.xml

2nd :-> There will be a xml dialplan which will connect user to freeswitch(freeswitch answers the call), then an application is being called which will go into the custom freeswitch module which I have created, which will connect to esl and generate custom events and then IVR will be played to the user and so on.

Note: Both the files are being named differently, like first and second methods respectively, in order to get easily understand.
Note: Will add some comments to give you a proper understanding, what I have tried to achieve.
Note: As I don't have the freeswitch installed right now with me, So, I have created based on my experience.
Note: Taken some help from freeswitch confluence pages, links below 
https://developer.signalwire.com/freeswitch/FreeSWITCH-Explained/Client-and-Developer-Interfaces/Event-Socket-Library/
https://developer.signalwire.com/freeswitch/FreeSWITCH-Explained/Client-and-Developer-Interfaces/Lua-API-Reference/
