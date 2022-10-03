<h1> Lora Rain Sensor </h1>

<p>Lorain is an affordable LoraWAN rain sensor utilizing the RAK 3172 Lora Module that can be programmed using Arduino environment. </p>

  
<h2> Hardware</h2>

<p>There are multiple ways to acquire the hardware for the LoRain. The Aqua-Scope Webshop at <a href="https://shop.aqua-scope.com">shop.aqua-scope.com</a> offers different options depending on your willingness to spent own effort in exchange for money:
<ul>
<li>Complete product tested and ready to use</li>
<li>Kit with all needed components and need to solder some of the components and flash the firmware. 
The kit includes a PCBA almost fully populated</li>
<li>The plain enclosure of the device. You need to order the PCB / <a href="img/lorain_pcba.jpg">PCBA</a>, purchase and solder all components and flash the firmware. The 'How-To' below assumes this option.</li>
</ul>
<p> The folder /hardware contains the gerber data and Bill of material of the PCBs. You can order the PCB from one of the usual suspects for fast and cheap PCB manufacturing like jlcpcb or pcbway. Ordering own PCB and soldering all components requires a certain level of hardware knowledge and soldering skills. Hence, we don’t provide additional guidance beside offering the files for the PCB.
</p>

<h2>How to complete the LoRain Kit</h2>

<p>The LoRain Kit consists of the following components (see <a href="img/lorain_kit.png">image</a>):
<ul>
<li>enclosure including screws for mounting the enclosure to a flat surface, PCBA compartment and battery compartment</li>
<li>one screw to hold the battery compartment</li>
<li>2 * AA Batteries</li>
<li>CBA SMD complete</li>
<li>1 * 8.6 mm whip antenna</li>
<li>2 * wires to connect PCBA with battery compartment</li>
<li>Button</li>
<li>Reed contact</li>
</uL>
<p>The PCBA needs some soldering as shown in this (<a href="img/lorain_pcb_solder.png">image</a>). Please note that all soldering are done on the B-Sie that does not have any components. No soldering needed on A side with the components.
</p>
<ul>
<li>Connect the Battery + contact if the PCBA to the battery + contact of the battery compartment using the red wires</li>
<li>Connect the Battery - contact if the PCBA to the battery contact of the battery compartment using the black wires (<a href="img/lorain_pcb_solder.png">image</a>)</li>
<li>Insert the button into the hole of the PCB and solder it on the four pins</li>
<li>Solder the reed contact on the B-side as shown on the silk screen (<a href="img/lorain_pcb_back.png">image</a>)</li>
<li>Solder the whip antenna to the antenna tru-hole</li>
</ul> 

<h2> Firmware </h2>
  
<p>The firmware is written as Arduino Sketch using the RAK Wireless Arduino Environment. You need to install the Arduino IDE and follow the instructions of <a href="https://docs.rakwireless.com/Product-Categories/WisDuo/RAK3172-Module/Quickstart/#rak3172-as-a-stand-alone-device-using-rui3"> RAK wireless </a> to install the RAK board support package. </p>
  
<p>To flash a new firmware you need a simple USB-TTL-UART connector (<a href="https://www.amazon.de/AZDelivery-Konverter-kompatibel-Arduino-inklusive/dp/B089QJZ51Z/ref=asc_df_B089QJZ51Z"> e.g. this </a>). The firmware update mode on the RAK module is enabled using the AT command 'AT+BOOT' on the serial console of the Arduino IDE. </p>
