# DS2438 Arduino OneWire Library

Changes to the original library to handle current measurement, calibration, and to remove the delays so it runs faster. Based on other forks of this code; see source for acknowledgements.

This code "works for me", but as I don't have much interest in using the DS2438 in future projects, I have not spent much time on QA or code cleanliness. I may come back to this in the future with improvements.

The DS2438 is fine for voltage and temperature measurement (albeit quite slow), but on the hardware I tested with, the current accumulation functionality seems broken. In hindsight, I would use a different temp/voltage/current chip, and I would go with my own dedicated ucontroller to integrate the current.

## Usage

Thae same as the old library, except you need to call update() 4 times to get the latest of all measurements, with a delay of at least 10ms between. this is because it manages state internally, and will do alternating ADC triggers and fetches without blocking. You can safely call update() in your loop() repeatedly.

It does things this way so it can start the ADC conversions on the hardware, then returning to let you do something else, before fetching the results in the next loop(), without needing to block execution in a delay().


## Installation

To install, download and rename the arduino-onewire-DS2438 folder to DS2438 and copy the DS2438 folder structure to your Arduino libraries folder.

Requires Arduino 1.0 or greater and OneWire Arduino library (see http://playground.arduino.cc/Learning/OneWire).

For additional information see http://projects.bechter.com

