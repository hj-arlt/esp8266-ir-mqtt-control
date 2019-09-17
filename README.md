# IR Controller

ESP8266 Compatible IR Receiver that can support mqtt protocol.
Smarthome client device.
An example is used in the smart home controller.

Adapted to other esp8266..

     Sonoff SV (ESP8266)
     GPIO_KEY1,        // GPIO00 Button
     GPIO_USER,        // GPIO01 Serial RXD and Optional sensor
     0,                // GPIO02 gong sensor, pullUP 1K8
     GPIO_USER,        // GPIO03 Serial TXD and Optional sensor
     GPIO_USER,        // GPIO04 Optional sensor
     GPIO_USER,        // GPIO05 Optional sensor
     0, 0, 0, 0, 0, 0, // Flash connection
     GPIO_REL1,        // GPIO12 Red Led and Relay (0 = Off, 1 = On)
     GPIO_LED1_INV,    // GPIO13 Green Led (0 = On, 1 = Off)
     GPIO_USER,        // GPIO14 Optional sensor
     0, 0,
     GPIO_ADC0         // ADC0 Analog input

