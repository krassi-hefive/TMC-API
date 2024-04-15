/*******************************************************************************
* Copyright © 2017 TRINAMIC Motion Control GmbH & Co. KG
* (now owned by Analog Devices Inc.),
*
* Copyright © 2023 Analog Devices Inc. All Rights Reserved.
* This software is proprietary to Analog Devices, Inc. and its licensors.
*******************************************************************************/


#include "TMC5160.h"


static int32_t readRegisterSPI(uint16_t icID, uint8_t address);
static void writeRegisterSPI(uint16_t icID, uint8_t address, int32_t value);
static int32_t readRegisterUART(uint16_t icID, uint8_t registerAddress);
static void writeRegisterUART(uint16_t icID, uint8_t registerAddress, int32_t value);

int32_t tmc5160_readRegister(uint16_t icID, uint8_t address)
{
    TMC5160BusType bus = tmc5160_getBusType(icID);

    if(bus == IC_BUS_SPI)
    {
        return readRegisterSPI(icID, address);
    }
   else if (bus == IC_BUS_UART)
   {
       return readRegisterUART(icID, address);
   }

    //ToDo: Error handling
    return -1;
}

void tmc5160_writeRegister(uint16_t icID, uint8_t address, int32_t value)
{
    TMC5160BusType bus = tmc5160_getBusType(icID);

    if(bus == IC_BUS_SPI)
    {
        writeRegisterSPI(icID, address, value);
    }
    else if(bus == IC_BUS_UART)
    {
        writeRegisterUART(icID, address, value);

    }
}

int32_t readRegisterSPI(uint16_t icID, uint8_t address)
{
    address = TMC_ADDRESS(address);


    // register not readable -> shadow register copy
    if(!TMC_IS_READABLE(TMC5160.registerAccess[address]))
        return TMC5160.config->shadowRegister[address];

    uint8_t data[5] = { 0 };

    // clear write bit
    data[0] = address;

    // Send the read request
    tmc5160_readWriteSPI(icID, &data[0], sizeof(data));

    // Rewrite address and clear write bit
    data[0] = address;

    // Send another request to receive the read reply
    tmc5160_readWriteSPI(icID, &data[0], sizeof(data));

    return ((uint32_t)data[1] << 24) | ((uint32_t) data[2] << 16) | ( data[3] <<  8) | ( data[4]);
}

void writeRegisterSPI(uint16_t icID, uint8_t address, int32_t value)
{
    uint8_t data[5] = { 0 };

    data[0] = address | TMC5160_WRITE_BIT;
    data[1] = 0xFF & (value>>24);
    data[2] = 0xFF & (value>>16);
    data[3] = 0xFF & (value>>8);
    data[4] = 0xFF & (value>>0);

    // Send the write request
    tmc5160_readWriteSPI(icID, &data[0], sizeof(data));

    // Write to the shadow register and mark the register dirty
    address = TMC_ADDRESS(address);
    TMC5160.config->shadowRegister[address] = value;
    TMC5160.registerAccess[address] |= TMC_ACCESS_DIRTY;
}

int32_t readRegisterUART(uint16_t icID, uint8_t address)
{
    uint8_t data[8] = { 0 };

    address = address & TMC_ADDRESS_MASK;

    if (!TMC_IS_READABLE(TMC5160.registerAccess[address]))
        return TMC5160.config->shadowRegister[address];

    data[0] = 0x05;
    data[1] = tmc5160_getNodeAddress(icID); //targetAddressUart;
    data[2] = address;
    data[3] = tmc_CRC8(data, 3, 1);

   if (!tmc5160_readWriteUART(icID, &data[0], 4, 8))
       return 0;

   // Byte 0: Sync nibble correct?
   if (data[0] != 0x05)
       return 0;

   // Byte 1: Master address correct?
   if (data[1] != 0xFF)
       return 0;

   // Byte 2: Address correct?
   if (data[2] != address)
       return 0;

   // Byte 7: CRC correct?
   if (data[7] != tmc_CRC8(data, 7, 1))
       return 0;

    return ((uint32_t)data[3] << 24) | ((uint32_t)data[4] << 16) | (data[5] << 8) | data[6];
}

void writeRegisterUART(uint16_t icID, uint8_t address, int32_t value)
{
    uint8_t data[8];

    data[0] = 0x05;
    data[1] = (uint8_t)tmc5160_getNodeAddress(icID); //targetAddressUart;
    data[2] = address | TMC5160_WRITE_BIT;
    data[3] = (value >> 24) & 0xFF;
    data[4] = (value >> 16) & 0xFF;
    data[5] = (value >> 8 ) & 0xFF;
    data[6] = (value      ) & 0xFF;
    data[7] = tmc_CRC8(data, 7, 1);

    tmc5160_readWriteUART(icID, &data[0], 8, 0);
    // Write to the shadow register and mark the register dirty
    address = TMC_ADDRESS(address);
    TMC5160.config->shadowRegister[address] = value;
    TMC5160.registerAccess[address] |= TMC_ACCESS_DIRTY;
}

void tmc5160_rotateMotor(uint16_t icID, uint8_t motor, int32_t velocity)
{
  if(motor >= TMC5160_MOTORS)
        return;
  StepDir_rotate(motor, velocity);
}


/***************** The following code is TMC-EvalSystem specific and needs to be commented out when working with other MCUs e.g Arduino*****************************/
// Initialize a TMC5160 IC.
// This function requires:
//     - tmc5160: The pointer to a TMC5160TypeDef struct, which represents one IC
//     - channel: The channel index, which will be sent back in the SPI callback
//     - config: A ConfigurationTypeDef struct, which will be used by the IC
//     - registerResetState: An int32_t array with 128 elements. This holds the values to be used for a reset.
void tmc5160_init(TMC5160TypeDef *tmc5160, uint8_t channel, ConfigurationTypeDef *config, const int32_t *registerResetState)
{
	tmc5160->velocity  = 0;
	tmc5160->oldTick   = 0;
	tmc5160->oldX      = 0;

	tmc5160->config               = config;
	tmc5160->config->callback     = NULL;
	tmc5160->config->channel      = channel;
	tmc5160->config->configIndex  = 0;
	tmc5160->config->state        = CONFIG_READY;

	size_t i;
	for(i = 0; i < TMC5160_REGISTER_COUNT; i++)
	{
		tmc5160->registerAccess[i]      = tmc5160_defaultRegisterAccess[i];
		tmc5160->registerResetState[i]  = registerResetState[i];
	}
}

// Fill the shadow registers of hardware preset non-readable registers
// Only needed if you want to 'read' those registers e.g to display the value
// in the TMCL IDE register browser
void tmc5160_fillShadowRegisters(TMC5160TypeDef *tmc5160)
{
	// Check if we have constants defined
	if(ARRAY_SIZE(tmc5160_RegisterConstants) == 0)
		return;

	size_t i, j;
	for(i = 0, j = 0; i < TMC5160_REGISTER_COUNT; i++)
	{
		// We only need to worry about hardware preset, write-only registers
		// that have not yet been written (no dirty bit) here.
		if(tmc5160->registerAccess[i] != TMC_ACCESS_W_PRESET)
			continue;

		// Search the constant list for the current address. With the constant
		// list being sorted in ascended order, we can walk through the list
		// until the entry with an address equal or greater than i
		while(j < ARRAY_SIZE(tmc5160_RegisterConstants) && (tmc5160_RegisterConstants[j].address < i))
			j++;

		// Abort when we reach the end of the constant list
		if (j == ARRAY_SIZE(tmc5160_RegisterConstants))
			break;

		// If we have an entry for our current address, write the constant
		if(tmc5160_RegisterConstants[j].address == i)
		{
			tmc5160->config->shadowRegister[i] = tmc5160_RegisterConstants[j].value;
		}
	}
}

// Reset the TMC5160.
uint8_t tmc5160_reset(TMC5160TypeDef *tmc5160)
{
	if(tmc5160->config->state != CONFIG_READY)
		return false;

	// Reset the dirty bits and wipe the shadow registers
	size_t i;
	for(i = 0; i < TMC5160_REGISTER_COUNT; i++)
	{
		tmc5160->registerAccess[i] &= ~TMC_ACCESS_DIRTY;
		tmc5160->config->shadowRegister[i] = 0;
	}

	tmc5160->config->state        = CONFIG_RESET;
	tmc5160->config->configIndex  = 0;

	return true;
}

// Restore the TMC5160 to the state stored in the shadow registers.
// This can be used to recover the IC configuration after a VM power loss.
uint8_t tmc5160_restore(TMC5160TypeDef *tmc5160)
{
	if(tmc5160->config->state != CONFIG_READY)
		return false;

	tmc5160->config->state        = CONFIG_RESTORE;
	tmc5160->config->configIndex  = 0;

	return true;
}

// Change the values the IC will be configured with when performing a reset.
void tmc5160_setRegisterResetState(TMC5160TypeDef *tmc5160, const int32_t *resetState)
{
	size_t i;
	for(i = 0; i < TMC5160_REGISTER_COUNT; i++)
	{
		tmc5160->registerResetState[i] = resetState[i];
	}
}

// Register a function to be called after completion of the configuration mechanism
void tmc5160_setCallback(TMC5160TypeDef *tmc5160, tmc5160_callback callback)
{
	tmc5160->config->callback = (tmc_callback_config) callback;
}

// Helper function: Configure the next register.
static void writeConfiguration(TMC5160TypeDef *tmc5160, uint8_t motor)
{
	uint8_t *ptr = &tmc5160->config->configIndex;
	const int32_t *settings;

	if(tmc5160->config->state == CONFIG_RESTORE)
	{
		settings = tmc5160->config->shadowRegister;
		// Find the next restorable register
		while((*ptr < TMC5160_REGISTER_COUNT) && !TMC_IS_RESTORABLE(tmc5160->registerAccess[*ptr]))
		{
			(*ptr)++;
		}
	}
	else
	{
		settings = tmc5160->registerResetState;
		// Find the next resettable register
		while((*ptr < TMC5160_REGISTER_COUNT) && !TMC_IS_RESETTABLE(tmc5160->registerAccess[*ptr]))
		{
			(*ptr)++;
		}
	}


    if(*ptr < TMC5160_REGISTER_COUNT)
    {
        if(*ptr == TMC5160_FACTORY_CONF){

            // Reading reset default value for FCLKTRIM (otp0.0 to otp0.4)
            int32_t otpFclkTrim = tmc5160_readRegister(motor, TMC5160_OTP_READ) & TMC5160_OTP_FCLKTRIM_MASK;
            // Writing the reset default value to FCLKTRIM
            tmc5160_writeRegister(motor, *ptr, otpFclkTrim);

        }else{
            tmc5160_writeRegister(motor, *ptr, settings[*ptr]);
        }
        (*ptr)++;
    }
	else // Finished configuration
	{
		if(tmc5160->config->callback)
		{
			((tmc5160_callback)tmc5160->config->callback)(tmc5160, tmc5160->config->state);
		}

		tmc5160->config->state = CONFIG_READY;
	}
}

// Call this periodically
void tmc5160_periodicJob(TMC5160TypeDef *tmc5160, uint8_t motor, uint32_t tick)
{
	if(tmc5160->config->state != CONFIG_READY)
	{
		writeConfiguration(tmc5160, motor);
		return;
	}

	int32_t XActual;
	uint32_t tickDiff;

	// Calculate velocity v = dx/dt
	if((tickDiff = tick - tmc5160->oldTick) >= 5)
	{
		XActual = tmc5160_readRegister(motor, TMC5160_XACTUAL);
		// ToDo CHECK 2: API Compatibility - write alternative algorithm w/o floating point? (LH)
		tmc5160->velocity = (uint32_t) ((float32_t) ((XActual - tmc5160->oldX) / (float32_t) tickDiff) * (float32_t) 1048.576);

		tmc5160->oldX     = XActual;
		tmc5160->oldTick  = tick;
	}
}

// Rotate with a given velocity (to the right)
void tmc5160_rotate(TMC5160TypeDef *tmc5160, uint8_t motor, int32_t velocity)
{
	// Set absolute velocity
	tmc5160_writeRegister(motor, TMC5160_VMAX, abs(velocity));
	// Set direction
	tmc5160_writeRegister(motor, TMC5160_RAMPMODE, (velocity >= 0) ? TMC5160_MODE_VELPOS : TMC5160_MODE_VELNEG);
}

// Rotate to the right
void tmc5160_right(TMC5160TypeDef *tmc5160, uint8_t motor, uint32_t velocity)
{
	tmc5160_rotate(tmc5160, motor, velocity);
}

// Rotate to the left
void tmc5160_left(TMC5160TypeDef *tmc5160, uint8_t motor, uint32_t velocity)
{
	tmc5160_rotate(tmc5160, motor, -velocity);
}

// Stop moving
void tmc5160_stop(TMC5160TypeDef *tmc5160, uint8_t motor)
{
	tmc5160_rotate(tmc5160, motor, 0);
}

// Move to a specified position with a given velocity
void tmc5160_moveTo(TMC5160TypeDef *tmc5160, uint8_t motor, int32_t position, uint32_t velocityMax)
{
	tmc5160_writeRegister(motor, TMC5160_RAMPMODE, TMC5160_MODE_POSITION);

	// VMAX also holds the target velocity in velocity mode.
	// Re-write the position mode maximum velocity here.
	tmc5160_writeRegister(motor, TMC5160_VMAX, velocityMax);

	tmc5160_writeRegister(motor, TMC5160_XTARGET, position);
}

// Move by a given amount with a given velocity
// This function will write the absolute target position to *ticks
void tmc5160_moveBy(TMC5160TypeDef *tmc5160, uint8_t motor, int32_t *ticks, uint32_t velocityMax)
{
	// determine actual position and add numbers of ticks to move
	*ticks += tmc5160_readRegister(motor, TMC5160_XACTUAL);

	tmc5160_moveTo(tmc5160, motor, *ticks, velocityMax);
}

uint8_t tmc5160_consistencyCheck(TMC5160TypeDef *tmc5160, uint8_t motor)
{
	// Config has not yet been written -> it cant be consistent
	if(tmc5160->config->state != CONFIG_READY)
		return 0;

	// Check constant shadow registers consistent with actual registers
	for(size_t i = 0; i < TMC5160_REGISTER_COUNT; i++)
		if(tmc5160->config->shadowRegister[i] != tmc5160_readRegister(motor, i))
			return 1;

	// No inconsistency detected
	return 0;
}
