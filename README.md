   <h1>Lichtwecker mit Touchdisplay und ESP32</h1>
Auf der Suche nach dem passenden Lichtwecker, bin ich der Meinung das es nichts gutes auf dem Markt gibt. Darum selber machen!
Es ist noch nicht perfekt aber aufgrund der begrenzten Zeit erstmal gut genug für mich.
Das ganze ist Opensource und damit viel Spaß damit!

<br />
<br />
<div align="center">
   <img src="Datensammlung/Bilder/20261016_132110_neu.jpg" width="400">
</div>
<br />


## Features Version 1
-   WIFI
    -   NTP-Zeitabgleich mit Sommerzeitumstellung
    -   Webserver für Weckeinstellungen, Statusanzeige, Lichtfunkiton und Einstellung der Radiostream Quellen 

-   MQTT
    -   Status und Logausgabe
    -   Einstellen der Weckfunktionen

-   Touchdisplay
    -   Anzeige der Uhrzeit und Datum
    -   Menüführung Wecker, Radiostream, Licht und Statusinfo   

-   Audiostream
    -   I2S 3W Audiokonverter für Webradio

-   RGB LED-Streifenansterung WS2812B
    -   Für den Sonnenaufgang 30 Minuten vor Weckzeit


Als Herzstück setze ich den ESP32 D1 Mini ein, der alle Arbeit übernimmt.
Dazu ein 3,5" TFT Touchdisplay mit 320x480 Punkten. Für die Audioausgabe ein Modul von Adafruit "I2S 3W Class D Amplifier - MAX98357A" das man zu hauf beim großen "A" finden kann. Ich wollte das Ganze ziemlich einfach halten um nicht an der Komplexität die auch kleine Projekte haben können zu scheitern. Den Lautsprecher habe ich aus altem Spielzeug der Kinder ausgebaut. Und damit gings dann los!

## Schaltplan
<br />
<br />
<div align="center">
   <img src="Datensammlung/Bilder/2026-10-16 141332.png" width="400">
</div>
<br />

Auch am Schaltplan kann man sehen das ich alles sehr einfach gehalten habe. Das einzige was ich mir als Elektroniker nicht verbeißen konnte ist ein 1000F Kondensator, der die Leistung aus der USB-Schnittstelle unterstützen soll. Alles andere ist sehr schlicht und fliegend verdrahtet.  

Als Spannungsversorgung habe ich eine USB-C Buchse verbaut. Ich bnutze an dieser Buchse nur die 5V Spannnungsversorgung und missbrauche die Weiße Leitung für die Ansteuerung des LED Streifens. Damit kann ich mit einem USB Kabel den Wecker mit Strom versorgen aber auch den LED-Streifen ansteuern ohne ein weiteres Kabel zu benötigen. Damit macht es den WAF (woman acceptance factor) um einiges besser. 

## Software
Ich habe das Projekt in Visual Studio Code mit Platform IO aufgesetzt, da es gut mit den Arduino LIB zusammen arbeitet. 

#Verwendete LIBS
-   AduinoJsoon
-   AsyncTCP
-   ESP32-audioI2S
-   ESPAsyncWebServer
-   PubSubClient
-   Ringbuffer
-   SolarCalculator
-   TFT_eSPI

Das Ziel bei der Software war es übersichtlich zu bleiben. Alleine Anhand der verwendeten Libs sieht man das sich das Projekt dann doch etwas erweitert hat.
Json wird für MQTT und auch für den Datenaustausch zwischen Webservice und Applikation verwendet. Auch das speichern der Daten im interen LittleFS verwendet Json.
Die Daten werden persistent im Flash gespeichert um bei einem Neustart nicht wieder eingegen werden zu müssen. 
Das gilt auch für die gespeicherten Radiostrem Adressen. Der RIngbuffer macht es einfacher die Daten ohne eine möglichen Speicherüberlauf zu verwalten.
Den SolarCalculator benötige ich für Sommer und Winter. Im Sommer benötige ich keine Sonnenaufgang, da ich keine Roladen im Zimmer habe. Anhand dees Sonnenaufgangs bleibt diese Funktion dann aus.
Am Schluss noch TFT_eSPI. Ich wollte einfach nicht die ganze TFT Ansteuerung selbst schreiben, das war mir doch zuviel.

Wie man damit schon erkennen kann, war das Projekt nun doch etwas umfangreicher als gedacht. 
Ich habe lange mit der Speicherverwaltung gekämpft, da der Audiostream nicht vernachlässigt werden will und das RAM doch sehr begrenzt ist. Auch heute gibt es beim Radio hören und gleichzeitigen Einstellen der Weckzeiten per Website Probleme. Aber das mache ich dann halt nicht zur gleichen Zeit.

Dann viel Spaß damit.
Über Rückmeldungen würde ich mich freuen!





<!-- 

<div align="center">
    <img src="wiki/DB_ESP32_NOW_Illistration.png" alt="DroneBridge with connectionless ESP-NOW protocol support for increased range of 1km or more.">
    <div>DroneBridge for ESP32 can be used to control drone swarms at a low cost.</div>
</div>
<br />
<div>
DroneBridge for ESP32 supports ESP-NOW LR, enabling ranges of more than 1km with external receiving antennas.<br />The number of drones is only limited by the channel capacity and the ESP32s processing power. All data is encrypted using AES256-GCM.
</div>

## Drone Light Show Edition (DLSE)
![DragonDroneShow](https://github.com/user-attachments/assets/979ba184-3fab-4bdf-a315-61321d533b82)

Special version that is optimised to work for Drone Shows using [Skybrush](https://skybrush.io/)    
Check [drone-bridge.com](https://drone-bridge.com) to get the latest release!

-   Support for Skybrush
-   Remote Power Management (sleep, wakeup) of the show drone
-   Lots of additional checks to improve safety and reliability
-   [Open Source Commercial Support Suite](https://github.com/DroneBridge/DLSECommercialSupportSuite) to help you flash, manage & maintain your drones efficiently. Script your own tools following the given examples using the DLSE API.
-   Over The Air (OTA) Updates for the ESP32
-   WiFi6 (OFDMA) & 5GHz support on supported ESP32 chips
-   ESP32-C3, ESP32-C5 & ESP32-C6

## Hardware

**Officially supported and tested boards:**  
Do the project and yourself a favour and use one of the officially supported and tested boards below.   
These boards are very affordable, have everything you need, and are also very compact. Perfect for use on any drone. 

**[You can find more info on how to get the official boards here!](https://dronebridge.gitbook.io/docs/dronebridge-for-esp32/hardware-and-wiring#officially-supported-boards)**

![Official Board DroneBridge for ESP32 featuring the ESP32C6](wiki/officialboard.webp)

[For further info please check the wiki!](https://dronebridge.gitbook.io/docs/dronebridge-for-esp32/hardware-and-wiring)

## Installation/Flashing using precompiled binaries

[It is recommended that you use the official online flashing tool!](https://drone-bridge.com/flasher/)

In any other case, there are multiple ways how to flash the firmware.  
**[For further info please check the wiki!](https://dronebridge.gitbook.io/docs/dronebridge-for-esp32/installation)**

## Wiring

1.  Connect the UART of the ESP32 to a 3.3V UART of your flight controller. It is not recommended to use the ESP32s pins that are marked with TX & RX since they often are connected to the internal serial ouput. Go for any other pin instead!
2.  Set the flight controller port to the desired protocol.

**Check out the manufacturer datasheet! Only some modules can take more than 3.3V. Follow the recommendations by the ESP32 board manufacturer for powering the device**  
**[For further info please check the wiki!](https://dronebridge.gitbook.io/docs/dronebridge-for-esp32/hardware-and-wiring)**

## Configuration
1.  Connect to the WiFi `DroneBridge ESP32` with password `dronebridge`
2.  In your browser type: `dronebridge.local` (Chrome: `http://dronebridge.local`) or `192.168.2.1` into the address bar.
 **You might need to disable the cellular connection to force the browser to use the WiFi connection**
3.  Configure as you please and hit `save`

![DroneBridge for ESP32 web interface](wiki/dbesp32_webinterface.png)

**[For further info please check the wiki!](https://dronebridge.gitbook.io/docs/dronebridge-for-esp32/configuration)**

## Use with QGroundControl, Mission Planner or any other GCS

![QGroundControl](https://docs.qgroundcontrol.com/master/assets/connected_vehicle.C1qygcZV.jpg)

-   The ESP will auto-send data to all connected devices via UDP to port 14550. QGroundControl should auto-connect using UDP
-   Connect via **TCP on port 5760** or **UDP on port 14550** to the ESP32 to send & receive data with a GCS of your choice. 
-   **In case of a UDP connection the GCS must send at least one packet (e.g. MAVLink heart beat etc.) to the UDP port of the ESP32 to register as an endpoint. Add ESP32 as an UDP target in the GCS**
-   Manually add a UDP target using the web interface

## Further Support

For questions or general chatting regarding DroneBridge for ESP32 please visit the Discord channel  
<div>
<a href="https://discord.gg/pqmHJNArE3">
<img src="wiki/discord-logo-blue.png" width="200px">
</a>
</div>

[contributors-shield]: https://img.shields.io/github/contributors/DroneBridge/ESP32.svg?style=for-the-badge
[contributors-url]: https://github.com/DroneBridge/ESP32/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/DroneBridge/ESP32.svg?style=for-the-badge
[forks-url]: https://github.com/DroneBridge/ESP32/network/members
[stars-shield]: https://img.shields.io/github/stars/DroneBridge/ESP32.svg?style=for-the-badge
[stars-url]: https://github.com/DroneBridge/ESP32/stargazers
[issues-shield]: https://img.shields.io/github/issues/DroneBridge/ESP32.svg?style=for-the-badge
[issues-url]: https://github.com/DroneBridge/ESP32/issues -->
