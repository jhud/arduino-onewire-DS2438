/*
 *   DS2438.h
 *
 *   by Joe Bechter
 *
 *   (C) 2012, bechter.com
 *
 *   All files, software, schematics and designs are provided as-is with no warranty.
 *   All files, software, schematics and designs are for experimental/hobby use.
 *   Under no circumstances should any part be used for critical systems where safety,
 *   life or property depends upon it. You are responsible for all use.
 *   You are free to use, modify, derive or otherwise extend for your own non-commercial purposes provided
 *       1. No part of this software or design may be used to cause injury or death to humans or animals.
 *       2. Use is non-commercial.
 *       3. Credit is given to the author (i.e. portions © bechter.com), and provide a link to the original source.
 *
 */

#ifndef DS2438_h
#define DS2438_h

#include <Arduino.h>
#include <OneWire.h>

#define DS2438_TEMPERATURE_CONVERSION_COMMAND 0x44
#define DS2438_VOLTAGE_CONVERSION_COMMAND 0xb4
#define DS2438_WRITE_SCRATCHPAD_COMMAND 0x4e
#define DS2438_COPY_SCRATCHPAD_COMMAND 0x48
#define DS2438_READ_SCRATCHPAD_COMMAND 0xbe
#define DS2438_RECALL_MEMORY_COMMAND 0xb8
#define DS2438_PAGE_0 0x00

#define PAGE_ZERO_CONFIG_REGISTER_ENABLE_IAD_BIT 0x01 // James - this enables current integration sensing
#define PAGE_ZERO_CONFIG_REGISTER_ENABLE_CA_BIT 0x02 // James - this enables current accumulation register
#define PAGE_ZERO_CONFIG_REGISTER_SHADOW_TO_EEPROM 0x04

#define DS2438_CHA 0
#define DS2438_CHB 1

#define DS2438_MODE_CHA 0x01
#define DS2438_MODE_CHB 0x02
#define DS2438_MODE_TEMPERATURE 0x04
#define DS2438_MODE_CURRENT 0x08

#define DS2438_VOLTAGE_CONVERSION_DELAY 16

class DS2438 {
    public:
        DS2438(OneWire *ow);
        DS2438(OneWire *ow, uint8_t *address);
        bool begin(uint8_t mode=(DS2438_MODE_CHA | DS2438_MODE_CHB | DS2438_MODE_TEMPERATURE | DS2438_MODE_CURRENT));
        void getAddress(uint8_t *addr);
        int update();
        double getTemperature();
        float getVoltage(int channel=DS2438_CHA);
		
		/**
		 * Get instantaneous current in amps.
		 * @param shunt shunt resistor (RSENSE) value in ohms, ie 0.01 Ohms
		 */
        float getCurrent(float shunt);
        boolean isError();
        unsigned long getTimestamp();
        boolean startConversion(boolean doTemperature);
		
		/**
		 * Write the offset value to calibrate the current read.
		*/
		bool writeCurrentOffset(int amount);
    private:		
		uint8_t _state;
        OneWire *_ow;
        uint8_t *_address;
        uint8_t _mode;
        double _temperature;
        float _voltageA;
        float _voltageB;
        long _current;
		unsigned long _currentAccumulated;
        unsigned long _timestamp;
        boolean _error;
        boolean selectChannel(int channel);
        void writePage(uint8_t page, uint8_t *data);
		bool writeConfigurationByte(uint8_t data);
        boolean readPage(uint8_t page, uint8_t *data);
        double calculateTemperature(uint8_t *data);
        double calculateVoltage(uint8_t *data);
		
		/**
		 * Since I could not get this working on my hardware, it just debug prints the raw register values.
		 */
		int getAccumulatedChargeData();
};

#endif
