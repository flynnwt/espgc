# ESP8266 WiFi I/O

## Background Info

This project contains some experimental code for using the ESP8266 to implement IR, control, and sensing over WiFi.  It is now updated to use the SPIFFS filesystem for web pages and persistent configuration/status data.

As a first attempt at programming the ESP8266, I used the Global Cache ITach as a base since I had some devices, its operation was documented, and I have used them with Homeseer and Perl scripts.  But having an HTTP interface makes this device much easier to use and potentially more powerful.  

## Goals

* Global Cache ITach Emulation (UDP, TCP), for use with existing controllers like Homeseer

* HTTP web admin and api for simple app/script/etc. interfacing

* May add local scheduling and event response for independent activity

## Dependencies

* ESP8266 Arduino library

* Other libraries

  * Time (https://github.com/PaulStoffregen/Time)
  
  * IRremoteESP8266 (https://github.com/shirriff/Arduino-IRremote/)
  


## Testing so far

* very limited; need to scope output signal timing/quality; have installed on ESP-01 and ESP-12 devices

* is recognized as ITach by Homeseer plugin (broadcast, tcp, sendir, sensor_notify)

* have tried a small set of IR devices/commands

## Hardware Programming and Operation

0. use Arduino IDE to upload SPIFFS data directory (I don't know if there is a commandline way to do this); I'm using 64K SPIFFS

0. use Arduino IDE or VSMicro to compile and upload code; my setup:
  * /libraries directories in ../Arduino/libraries directory
  * /espgc files in a main project (../espgc) directory
  * /espgc/data is the SPIFFS data directory

0. serial console output for debug/monitoring

0. boots to AP mode w/SSID based on MAC, accessible at 192.168.0.1

0. configure for LAN, apply changes, restart from webpage or hardware reset/power cycle

0. boots to LAN mode as configured

0. module/connector config defaults to ITach WiFI device using ESP GPIO 4,5

0. reconfigure pins to match hardware using config file edit/save from web, or api, or edit and upload config.txt to SPIFFS dir, or change default config in code


## API

* Hardware info/control/sense
  * GET /api/cmd=[gc cmd]

* Configuration, Utilities
  * GET /api/config
  * POST /api/config
  * GET /api/configfile
  * POST /api/configfile
  * GET /api/networks
  * GET /api/compressir
  * GET /api/decompressir

* Administration
  * POST /api/admin/lock
  * POST /api/admin/unlock
  * POST /api/admin/restart
  * DELETE /api/admin/file
  
  
  #### Examples

```
curl 192.168.0.100/api?cmd=sendir,1:1,1,37000,10,1,128,64,16,16,16,49BBBBBBBBBBBCBBBBBBBBBCBBBBBBBBCBCCCCBBCBCCCCBC16,2718
{
"res":"completeir,1:1,1"
}

curl 192.168.0.100/api?cmd=getstate,1:2
{
"res":"state,1:2,0"
}
  
curl 192.168.0.100/api/compressir?code=128,63,16,16,16,48,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,48,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,48,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,48,16,48,16,48,16,48,16,48,16,16,16,16,16,16,16,48,16,48,16,48,16,48,16,48,16,16,16,48,16,2712
{
"code":"128,63,16,16,16,48BBBBBBBBBBBCBBBBBBBBBCBBBBBBBBBCCCCCBBBCCCCCBC16,2712"
}

curl -X POST 192.168.0.100/api/admin/lock
{
"err":"Expected 'pw' parameter"
}
>curl -X POST 192.168.0.100/api/admin/lock?pw=sparty
{
"res":"Locked device"
}
curl -X POST 192.168.0.100/api/admin/lock?pw=sparty
{
"res":"Can't lock device, already locked"
}
curl -X POST 192.168.0.100/api/admin/unlock
{
"err":"Expected 'pw' parameter"
}
curl -X POST 192.168.0.100/api/admin/unlock?pw=wolverines
{
"res":"Can't unlock device, password mismatch"
}
curl -X POST 192.168.0.100/api/config
{
"err":"Device is locked"
}
C:\Projects\node>curl 192.168.0.100/api/config
{
"res":{
"version":"867.5309.01",
"platform":"ESP-12E",
"mac":"18FE34A0D72A",
"updated":1460219129,
"locked":1,
"deviceName":"La Villa StrangIRto",
"tagline":"...we have assumed control...",
"hostname":"Hemispheres",
"wirelessMode":1,
"ssid":"WTF-N",
"passphrase":"(DEVICE LOCKED)",
"dhcp":1,
"staticIp":"0.0.0.0",
"gatewayIp":"0.0.0.0",
"subnetMask":"0.0.0.0",
"discovery":1,
"tcp":1,
"module_0":"WiFi,-1,-1",
"module_1":"IR,-1,-1",
"connector_0":"IR,1,1,5,-1",
"connector_1":"SensorNotify,1,2,4,-1",
"connector_2":"IRBlaster,1,3,5,-1",
"lastBoot":"OK"
}
}
curl -X POST 192.168.0.100/api/admin/restart
{
"err":"Device is locked"
}
curl -X POST 192.168.0.100/api/admin/unlock?pw=sparty
{
"res":"Unlocked device"
}
```
#### Typical Output at Power On
```
*** Hardware Info

ESP Info: 
 VCC: 65535
 Frequency: 80
 Free Heap: 33888
 SDK: 1.5.1(e67da894)
 Boot Version: 31
 Boot Mode: 1
 Flash: 1640EF,524288,40000000,2


*** File System

Dir: /
/reset.txt: 4B
/app.css: 8353B
/status.txt: 2B
/app.js: 17712B
/favicon.ico: 1150B
/nanoajax.min.js: 889B
/index.html: 7596B
/balalaika.min.js: 976B
/config.txt: 423B

Checking reset file: No reset - pin 4 state not 1.


*** Configuration

Checking last boot: status=OK

Current configuration:
version=867.5309.01
platform=ESP-12E
mac=18FE34A0D72A
updated=1460227943
locked=0
devicename=La Villa StrangIRto
tagline=...we have assumed control...
hostname=Hemispheres
wirelessMode=1
ssid=WTF-N
passphrase=magic
dhcp=1
staticIp=0.0.0.0
gatewayIp=0.0.0.0
subnetMask=0.0.0.0
discovery=1
tcp=1
module=0,WiFi
module=1,IR
connector=0,IR,1,1,5,-1
connector=1,SensorNotify,1,2,4,-1
connector=2,IRBlaster,1,3,5,-1

Loaded configuration:
version=867.5309.01
platform=ESP-12E
mac=18FE34A0D72A
updated=1460227943
locked=0
devicename=La Villa StrangIRto
tagline=...we have assumed control...
hostname=Hemispheres
wirelessMode=1
ssid=WTF-N
passphrase=magic
dhcp=1
staticIp=0.0.0.0
gatewayIp=0.0.0.0
subnetMask=0.0.0.0
discovery=0
tcp=0
module=0,WiFi
module=1,IR
connector=0,IR,1,1,5,-1
connector=1,SensorNotify,1,2,4,-1
connector=2,IRBlaster,1,3,5,-1

{
"version":"867.5309.01",
"platform":"ESP-12E",
"mac":"18FE34A0D72A",
"updated":1460227943,
"locked":0,
"deviceName":"La Villa StrangIRto",
"tagline":"...we have assumed control...",
"hostname":"Hemispheres",
"wirelessMode":1,
"ssid":"WTF-N",
"passphrase":"magic",
"dhcp":1,
"staticIp":"0.0.0.0",
"gatewayIp":"0.0.0.0",
"subnetMask":"0.0.0.0",
"discovery":0,
"tcp":0,
"module_0":"WiFi,-1,-1",
"module_1":"IR,-1,-1",
"connector_0":"IR,1,1,5,-1",
"connector_1":"SensorNotify,1,2,4,-1",
"connector_2":"IRBlaster,1,3,5,-1",
"lastBoot":"OK"
}

Checking lock: unlocked



*** Hardware Configuration

Adding module 0, type=0
Adding module 1, type=2
Adding connector 1:1, type=0, fPin=5, sPin=-1
Adding connector 1:2, type=3, fPin=4, sPin=-1
Adding connector 1:3, type=1, fPin=5, sPin=-1

Device Tree:
Module 0, 0 WIFI
Module 1, 3 IR
  Connector 1:1, IR, fPin=5, sPin=-1
  Connector 1:2, SENSOR_NOTIFY, fPin=4, sPin=-1
  Connector 1:3, IR_BLASTER, fPin=5, sPin=-1


*** Initialization

MAC: 18FE34A0D72A
Hostname: Hemispheres
Connecting to network...
 SSID: WTF-N
 IP:   192.168.0.100
Discovery beacon active on 9131.
TCP server started on 4998.
Getting time...
2016-04-09 22:52:49 UTC
Boot complete.
HTTP server started.


*** Network Scan

 0: [1][1A:FE:34:F2:03:73] NONE ESP_F20373 (-87)
 1: [1][80:1F:02:60:47:68] AUTO WTF-N (-50)
 2: [6][00:21:29:71:4C:A3] AUTO WTF (-63)


*** Setup Complete...Thunderbirds are GO!


discovery beacon:
AMXB<-UUID=GlobalCache_18fe34a0d72a><-SDKClass=Utility><-Make=GlobalCache><-Model=iTachWF2IR><-Revision=710-1001-05><-Pkg_Level=GCPK001><-Config-URL=http://192.168.0.100><-PCB_PN=025-0026-06><-Status=Ready><CR>

*** 2016-04-09 22:52:51 UTC free:29280 lat:0us (boot: 2016-04-09 22:52:51 UTC)

GET /api/networks
200
{
"res":"{\"networks\":[\n{\"channel\":1,\"bssid\":\"80:1f:02:60:47:68\",\"encryption\":\"AUTO\",\"ssid\":\"WTF-N\",\"rssi\":-47},\n{\"channel\":1,\"bssid\":\"1a:fe:34:f2:03:73\",\"encryption\":\"NONE\",\"ssid\":\"ESP_F20373\",\"rssi\":-88},\n{\"channel\":6,\"bssid\":\"00:21:29:71:4c:a3\",\"encryption\":\"AUTO\",\"ssid\":\"WTF\",\"rssi\":-65}\n]}"
}
sensornotify,1:2,0

*** 2016-04-09 22:53:51 UTC free:29304 lat:102us (boot: 2016-04-09 22:52:51 UTC)

TCP: inserting client 0
TCP: inserting client 1
TCP: inserting client 2
TCP: deleting disconnected client 1
TCP: deleting disconnected client 2
TCP request(0): getversion<CR><LF>
TCP response: 710-1001-05<CR>
sensornotify,1:2,0

*** 2016-04-09 22:54:51 UTC free:29056 lat:102us (boot: 2016-04-09 22:52:51 UTC)

discovery beacon:
AMXB<-UUID=GlobalCache_18fe34a0d72a><-SDKClass=Utility><-Make=GlobalCache><-Model=iTachWF2IR><-Revision=710-1001-05><-Pkg_Level=GCPK001><-Config-URL=http://192.168.0.100><-PCB_PN=025-0026-06><-Status=Ready><CR>
TCP request(0): getdevices<CR><LF>
TCP response: device,0,0 WIFI<CR>device,1,3 IR<CR>endlistdevices<CR>
TCP request(0): getversion<CR><LF>
TCP response: 710-1001-05<CR>
TCP request(0): get_NET,0:1<CR><LF>
TCP response: NET,0:1,UNLOCKED,DHCP,192.168.0.100,255.255.255.0,192.168.0.1<CR>
TCP request(0): get_IR,1:1<CR><LF>
TCP response: IR,1:1,IR<CR>
TCP request(0): get_IR,1:2<CR><LF>
TCP response: IR,1:2,SENSOR_NOTIFY<CR>
TCP request(0): get_IR,1:3<CR><LF>
TCP response: IR,1:3,IR_BLASTER<CR>
TCP request(0): getstate,1:2<CR><LF>
TCP response: state,1:2,0<CR>
sensornotify,1:2,0
TCP request(0): getversion<CR><LF>
TCP response: 710-1001-05<CR>
TCP request(0): getversion<CR><LF>
TCP response: 710-1001-05<CR>

*** 2016-04-09 22:55:51 UTC free:29240 lat:108us (boot: 2016-04-09 22:52:51 UTC)

discovery beacon:
AMXB<-UUID=GlobalCache_18fe34a0d72a><-SDKClass=Utility><-Make=GlobalCache><-Model=iTachWF2IR><-Revision=710-1001-05><-Pkg_Level=GCPK001><-Config-URL=http://192.168.0.100><-PCB_PN=025-0026-06><-Status=Ready><CR>
sensornotify,1:2,0
TCP request(0): getversion<CR><LF>
TCP response: 710-1001-05<CR>
TCP request(0): getversion<CR><LF>
TCP response: 710-1001-05<CR>
...
```