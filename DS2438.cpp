/*
 *   DS2438.cpp
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
 * 
 *   Updated By BMG
 */

#include "DS2438.h"

// Mofidied by James

String oneWireAddressToString(byte addr[]) {
  String s = "";
  for (byte i = 0; i < 8; i++) {
    if (addr[i] < 16) s += "0";
    s += String(addr[i], HEX);
  }
  return s;
}

DS2438::DS2438(OneWire *ow)
  : _state(0)
  , _ow(ow){
}

DS2438::DS2438(OneWire *ow, uint8_t *address) 
	: _state(0)
    , _ow(ow) {

    _address = address;
};

bool DS2438::begin(uint8_t mode) {
    uint8_t addr[8];
	const int numBytes = sizeof(addr)/sizeof(addr[0]);

    _mode = mode & (DS2438_MODE_CHA | DS2438_MODE_CHB | DS2438_MODE_TEMPERATURE | DS2438_MODE_CURRENT);
    _temperature = 0;
    _voltageA = 0.0;
    _voltageB = 0.0;
    _current = 0.0;
    _error = true;
    _timestamp = 0;
	_state = 0;
	
    while (_ow->search(addr)) {
		int i = 0;
		for (; i<numBytes && (addr[i] == _address[i]); i++) {
		}	
		if (i == numBytes) {
        	return true;
		}
	}
	
	return false;
}


int DS2438::update() {
    uint8_t data[9];

    _error = true;
    _timestamp = millis();

	if (_state == 0 && (_mode&DS2438_MODE_CHA > 0)) {
	    if (!selectChannel(DS2438_CHA))
	        return -5;
		
	    if (!startConversion((_mode & DS2438_MODE_TEMPERATURE) > 0)) {
	        return -1;
	    }
	}
	else if (_state == 1 && (_mode&DS2438_MODE_CHA > 0)) {
		// Results are waiting from CHA
        if (!readPage(0, data)) {
            return -2;
		}
		
		_temperature = (double)(((((int16_t)data[2]) << 8) | (data[1] & 0x0ff)) >> 3) * 0.03125;
		
        _voltageA = (((data[4] << 8) & 0x00300) | (data[3] & 0xff)) / 100.0;
		
		_current = ((((int16_t)(data[6]) << 8) & 0b0000001100000000) | (data[5] & 0x0ff)) ;
		if (data[6] &0b100 > 0) { // Sign bit
						_current = -1 * (0x3ff-_current);
		}
	}
	else if (_state == 2 && (_mode&DS2438_MODE_CHB > 0)) {
	    if (!selectChannel(DS2438_CHB))
	        return -5;
		
	    if (!startConversion((_mode & DS2438_MODE_TEMPERATURE) > 0 && (_mode & DS2438_MODE_CHA) == 0)) {
	        return -1;
	    }		
	}
	else if (_state == 3) {
		// Results are waiting from CHB
		
		if (_mode&DS2438_MODE_CHB > 0) {
		
        	if (!readPage(0, data)) {
            	return -4;
			}
		
			_current = ((((int16_t)(data[6]) << 8) & 0b0000001100000000) | (data[5] & 0xff)) ;
			if (data[6] &0b1000 > 0) { // Sign bit
				_current = -1 * (0x3ff-_current);
			}

			_voltageB = (((data[4] << 8) & 0x00300) | (data[3] & 0x0ff)) / 100.0;
		}
		
		if ((_mode & DS2438_MODE_CURRENT) > 0) {
			int err = getAccumulatedChargeData();
			if (err != 0) {
				return err;
			}
		}
		
		_state = -1;
	}

	_state++;	

    _error = false;
	return 0;
}

int DS2438::getAccumulatedChargeData() {
	uint8_t data[9];
    if (!readPage(1, data)) {
        return -4;
	}
	Serial.print("ICA byte: ");
	Serial.println(data[4]);
	
    if (!readPage(7, data)) {
        return -4;
	}
	
	Serial.print("CCA byte: ");
	const uint16_t cca = (((int16_t)data[5]) << 8) | (data[4]);
	Serial.println(cca);
	
	Serial.print("DCA byte: ");
	const uint16_t dca = (((int16_t)data[7]) << 8) | (data[6]);
	_currentAccumulated = dca;
	Serial.println(dca);
	return 0;
}

double DS2438::getTemperature() {
    return _temperature;
}

float DS2438::getVoltage(int channel) {
    if (channel == DS2438_CHA) {
        return _voltageA;
    } else if (channel == DS2438_CHB) {
        return _voltageB;
    } else {
        return 0.0;
    }
}


float DS2438::getCurrent(float shunt) {
	Serial.print("Now current raw: ");
	Serial.println(_current);
    return float(_current)/(4096.0f*shunt);
}


boolean DS2438::isError() {
    return _error;
}

unsigned long DS2438::getTimestamp() {
    return _timestamp;
}

boolean DS2438::startConversion(boolean doTemperature) {
    _ow->reset();
    _ow->select(_address);
    if (doTemperature) {
        _ow->write(DS2438_TEMPERATURE_CONVERSION_COMMAND, 0);
        _ow->reset();
        _ow->select(_address);
    }
    _ow->write(DS2438_VOLTAGE_CONVERSION_COMMAND, 0);
    return true;
}

boolean DS2438::selectChannel(int channel) {
    uint8_t data = 0b011;
    if (channel == DS2438_CHB) {
        data = data | 0b1000;
	}
    return writeConfigurationByte(data);
		//
		// if ((_mode & DS2438_MODE_CURRENT) > 0) {
		// 	data[0] = data[0] | PAGE_ZERO_CONFIG_REGISTER_ENABLE_IAD_BIT | PAGE_ZERO_CONFIG_REGISTER_ENABLE_CA_BIT | 0b0111; // Also set this bit so we always get current info
		// 	//data[0] &= ~PAGE_ZERO_CONFIG_REGISTER_SHADOW_TO_EEPROM;
		// }

}

bool DS2438::writeCurrentOffset(int amount) {
/*	4. Disable the current ADC by setting the IAD bit in the Status/Configuration Register to “0”
	5. Change the sign of the previously-read Current Register value by performing the two’s complement
	and write the result to the Offset Register
	6. Enable the current ADC by setting the IAD bit in the Status/Configuration Register to “1”*/
	
	uint8_t data[9];
	
	if (!readPage(1, data) ) {
		Serial.println("Page 1 error");
		return false;
	}
	writeConfigurationByte(PAGE_ZERO_CONFIG_REGISTER_SHADOW_TO_EEPROM); // 4. Disable the current ADC by setting the IAD bit in the Status/Configuration Register to “0”
	
	delay(40); // Wait for ADC to stop

	if (amount >= 512 || amount <= -512) {
		Serial.println("Can only write 9 bits");
		return false;
	}
	
	if (amount < 0) {
		amount = 0x7ff+amount;
	}
	
	amount = amount << 3; // First 3 bits are unused by register
	
	data[5] = amount&0xff;
	data[6] = amount>>8;
	
	writePage(1, data);
}

bool DS2438::writeConfigurationByte(uint8_t data) {
    _ow->reset();
    _ow->select(_address);
    _ow->write(DS2438_WRITE_SCRATCHPAD_COMMAND, 0);
    _ow->write(DS2438_PAGE_0, 0);
    _ow->write(data, 0);
    _ow->reset();
	_ow->select(_address);
    _ow->write(DS2438_READ_SCRATCHPAD_COMMAND, 0);
    _ow->write(DS2438_PAGE_0, 0);
	uint8_t check[9];
    for (int i = 0; i < 9; i++) {
        check[i] = _ow->read();
	}
	if (_ow->crc8(check, 8) != check[8]) {
		Serial.println("CRC failed.");
			Serial.println(oneWireAddressToString(check));
		return false;
	}
	if (check[0] != data) {
		Serial.println("Config not written.");
			Serial.println(oneWireAddressToString(check));
		return false;
	}
    _ow->reset();
    _ow->select(_address);
    _ow->write(DS2438_COPY_SCRATCHPAD_COMMAND, 0);
    _ow->write(DS2438_PAGE_0, 0);
	// while(_ow->read() == 0) {
	// 	delay(1);
	// }
	
	//Serial.println("Set configuration. Zero page is:");
	//Serial.println(oneWireAddressToString(check));
	return true;
}

void DS2438::writePage(uint8_t page, uint8_t *data) {
    _ow->reset();
    _ow->select(_address);
    _ow->write(DS2438_WRITE_SCRATCHPAD_COMMAND, 0);
    _ow->write(page, 0);
    for (int i = 0; i < 8; i++)
        _ow->write(data[i], 0);
    _ow->reset();
    _ow->select(_address);
    _ow->write(DS2438_COPY_SCRATCHPAD_COMMAND, 0);
    _ow->write(page, 0);
	
	Serial.println("Wrote page");
	Serial.println(oneWireAddressToString(data));
}

boolean DS2438::readPage(uint8_t page, uint8_t *data) {
    _ow->reset();
    _ow->select(_address);
    _ow->write(DS2438_RECALL_MEMORY_COMMAND, 0);
    _ow->write(page, 0);
    _ow->reset();
    _ow->select(_address);
    _ow->write(DS2438_READ_SCRATCHPAD_COMMAND, 0);
    _ow->write(page, 0);
    for (int i = 0; i < 9; i++)
        data[i] = _ow->read();
	
	Serial.print("Page dump ");
	Serial.println(page);
	Serial.println(oneWireAddressToString(data));
	return _ow->crc8(data, 8) == data[8];
}


