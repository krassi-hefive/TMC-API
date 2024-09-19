/*******************************************************************************
* Copyright © 2019 TRINAMIC Motion Control GmbH & Co. KG
* (now owned by Analog Devices Inc.),
*
* Copyright © 2024 Analog Devices Inc. All Rights Reserved.
* This software is proprietary to Analog Devices, Inc. and its licensors.
*******************************************************************************/


#include "TMC2300.h"

#ifdef TMC_API_EXTERNAL_CRC_TABLE
extern const uint8_t tmcCRCTable_Poly7Reflected[256];
#else
const uint8_t tmcCRCTable_Poly7Reflected[256] = {
        0x00, 0x91, 0xE3, 0x72, 0x07, 0x96, 0xE4, 0x75, 0x0E, 0x9F, 0xED, 0x7C, 0x09, 0x98, 0xEA, 0x7B,
        0x1C, 0x8D, 0xFF, 0x6E, 0x1B, 0x8A, 0xF8, 0x69, 0x12, 0x83, 0xF1, 0x60, 0x15, 0x84, 0xF6, 0x67,
        0x38, 0xA9, 0xDB, 0x4A, 0x3F, 0xAE, 0xDC, 0x4D, 0x36, 0xA7, 0xD5, 0x44, 0x31, 0xA0, 0xD2, 0x43,
        0x24, 0xB5, 0xC7, 0x56, 0x23, 0xB2, 0xC0, 0x51, 0x2A, 0xBB, 0xC9, 0x58, 0x2D, 0xBC, 0xCE, 0x5F,
        0x70, 0xE1, 0x93, 0x02, 0x77, 0xE6, 0x94, 0x05, 0x7E, 0xEF, 0x9D, 0x0C, 0x79, 0xE8, 0x9A, 0x0B,
        0x6C, 0xFD, 0x8F, 0x1E, 0x6B, 0xFA, 0x88, 0x19, 0x62, 0xF3, 0x81, 0x10, 0x65, 0xF4, 0x86, 0x17,
        0x48, 0xD9, 0xAB, 0x3A, 0x4F, 0xDE, 0xAC, 0x3D, 0x46, 0xD7, 0xA5, 0x34, 0x41, 0xD0, 0xA2, 0x33,
        0x54, 0xC5, 0xB7, 0x26, 0x53, 0xC2, 0xB0, 0x21, 0x5A, 0xCB, 0xB9, 0x28, 0x5D, 0xCC, 0xBE, 0x2F,
        0xE0, 0x71, 0x03, 0x92, 0xE7, 0x76, 0x04, 0x95, 0xEE, 0x7F, 0x0D, 0x9C, 0xE9, 0x78, 0x0A, 0x9B,
        0xFC, 0x6D, 0x1F, 0x8E, 0xFB, 0x6A, 0x18, 0x89, 0xF2, 0x63, 0x11, 0x80, 0xF5, 0x64, 0x16, 0x87,
        0xD8, 0x49, 0x3B, 0xAA, 0xDF, 0x4E, 0x3C, 0xAD, 0xD6, 0x47, 0x35, 0xA4, 0xD1, 0x40, 0x32, 0xA3,
        0xC4, 0x55, 0x27, 0xB6, 0xC3, 0x52, 0x20, 0xB1, 0xCA, 0x5B, 0x29, 0xB8, 0xCD, 0x5C, 0x2E, 0xBF,
        0x90, 0x01, 0x73, 0xE2, 0x97, 0x06, 0x74, 0xE5, 0x9E, 0x0F, 0x7D, 0xEC, 0x99, 0x08, 0x7A, 0xEB,
        0x8C, 0x1D, 0x6F, 0xFE, 0x8B, 0x1A, 0x68, 0xF9, 0x82, 0x13, 0x61, 0xF0, 0x85, 0x14, 0x66, 0xF7,
        0xA8, 0x39, 0x4B, 0xDA, 0xAF, 0x3E, 0x4C, 0xDD, 0xA6, 0x37, 0x45, 0xD4, 0xA1, 0x30, 0x42, 0xD3,
        0xB4, 0x25, 0x57, 0xC6, 0xB3, 0x22, 0x50, 0xC1, 0xBA, 0x2B, 0x59, 0xC8, 0xBD, 0x2C, 0x5E, 0xCF,
};
#endif
/**************************************************************** Cache Implementation *************************************************************************/

#if TMC2300_CACHE == 0
static inline bool tmc2300_cache(uint16_t icID, TMC2300CacheOp operation, uint8_t address, uint32_t *value)
{
    UNUSED(icID);
    UNUSED(address);
    UNUSED(operation);
    return false;
}
#else
#if TMC2300_ENABLE_TMC_CACHE == 1
// => UART wrapper
extern void tmc2300_readWriteArray(uint8_t channel, uint8_t *data, size_t writeLength, size_t readLength);
// <= UART wrapper

uint8_t tmc2300_dirtyBits[TMC2300_IC_CACHE_COUNT][TMC2300_REGISTER_COUNT/8]= {0};
int32_t tmc2300_shadowRegister[TMC2300_IC_CACHE_COUNT][TMC2300_REGISTER_COUNT];
// => CRC wrapper
extern uint8_t tmc2300_CRC8(uint8_t *data, size_t length);
// <= CRC wrapper

void tmc2300_setDirtyBit(uint16_t icID, uint8_t index, bool value)
void tmc2300_writeInt(TMC2300TypeDef *tmc2300, uint8_t address, int32_t value)
{
    if(index >= TMC2300_REGISTER_COUNT)
        return;
    // When we are in standby or in the reset procedure we do not actually write
    // to the IC - we only update the shadow registers. After exiting standby or
    // completing a reset we transition into a restore, which pushes the shadow
    // register contents into the chip.
    if (!tmc2300->standbyEnabled || tmc2300->config->state != CONFIG_RESET)
    {
        uint8_t data[8];

        data[0] = 0x05;
        data[1] = tmc2300->slaveAddress;
        data[2] = address | TMC_WRITE_BIT;
        data[3] = (value >> 24) & 0xFF;
        data[4] = (value >> 16) & 0xFF;
        data[5] = (value >> 8 ) & 0xFF;
        data[6] = (value      ) & 0xFF;
        data[7] = tmc2300_CRC8(data, 7);

        tmc2300_readWriteArray(tmc2300->config->channel, &data[0], 8, 0);
    }

    uint8_t *tmp = &tmc2300_dirtyBits[icID][index / 8];
    uint8_t shift = (index % 8);
    uint8_t mask = 1 << shift;
    *tmp = (((*tmp) & (~(mask))) | (((value) << (shift)) & (mask)));
    // Write to the shadow register and mark the register dirty
    address = TMC_ADDRESS(address);
    tmc2300->config->shadowRegister[address] = value;
    tmc2300->registerAccess[address] |= TMC_ACCESS_DIRTY;
}

bool tmc2300_getDirtyBit(uint16_t icID, uint8_t index)
int32_t tmc2300_readInt(TMC2300TypeDef *tmc2300, uint8_t address)
{
    if(index >= TMC2300_REGISTER_COUNT)
        return false;
    uint8_t data[8] = { 0 };

    address = TMC_ADDRESS(address);

    // When the chip is in standby or when accessing a write-only register
    // use the shadow register content instead.
    if (tmc2300->standbyEnabled || !TMC_IS_READABLE(tmc2300->registerAccess[address]))
        return tmc2300->config->shadowRegister[address];

    data[0] = 0x05;
    data[1] = tmc2300->slaveAddress;
    data[2] = address;
    data[3] = tmc2300_CRC8(data, 3);

    tmc2300_readWriteArray(tmc2300->config->channel, data, 4, 8);

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
    if (data[7] != tmc2300_CRC8(data, 7))
        return 0;

    uint8_t *tmp = &tmc2300_dirtyBits[icID][index / 8];
    uint8_t shift = (index % 8);
    return ((*tmp) >> shift) & 1;
    return ((uint32_t)data[3] << 24) | ((uint32_t)data[4] << 16) | (data[5] << 8) | data[6];
}
/*
 * This function is used to cache the value written to the Write-Only registers in the form of shadow array.
 * The shadow copy is then used to read these kinds of registers.
 */

bool tmc2300_cache(uint16_t icID, TMC2300CacheOp operation, uint8_t address, uint32_t *value)
void tmc2300_init(TMC2300TypeDef *tmc2300, uint8_t channel, ConfigurationTypeDef *tmc2300_config, const int32_t *registerResetState)
{
    if (operation == TMC2300_CACHE_READ)
    {
        // Check if the value should come from cache
    tmc2300->config               = tmc2300_config;
    tmc2300->config->callback     = NULL;
    tmc2300->config->channel      = channel;
    tmc2300->config->configIndex  = 0;
    tmc2300->config->state        = CONFIG_READY;

        // Only supported chips have a cache
        if (icID >= TMC2300_IC_CACHE_COUNT)
            return false;
    // Default slave address: 0
    tmc2300->slaveAddress = 0;

        // Only non-readable registers care about caching
        // Note: This could also be used to cache i.e. RW config registers to reduce bus accesses
        if (TMC2300_IS_READABLE(tmc2300_registerAccess[address]))
            return false;
    // Start in standby
    tmc2300->standbyEnabled = 1;

        // Grab the value from the cache
        *value = tmc2300_shadowRegister[icID][address];
        return true;
    }
    else if (operation == TMC2300_CACHE_WRITE || operation == TMC2300_CACHE_FILL_DEFAULT)
    int32_t i;
    for(i = 0; i < TMC2300_REGISTER_COUNT; i++)
    {
        // Fill the cache

        // only supported chips have a cache
        if (icID >= TMC2300_IC_CACHE_COUNT)
            return false;

        // Write to the shadow register.
        tmc2300_shadowRegister[icID][address] = *value;
        // For write operations, mark the register dirty
        if (operation == TMC2300_CACHE_WRITE)
        {
            tmc2300_setDirtyBit(icID, address, true);
        }

        return true;
        tmc2300->registerAccess[i]      = tmc2300_defaultRegisterAccess[i];
        tmc2300->registerResetState[i]  = registerResetState[i];
    }
    return false;
}

void tmc2300_initCache()
// Fill the shadow registers of hardware preset registers
// Only needed if you want to read out those registers to display the value
// (e.g. for the TMCL IDE register browser)
static void fillShadowRegisters(TMC2300TypeDef *tmc2300)
{
    // Check if we have constants defined
    if(ARRAY_SIZE(tmc2300_RegisterConstants) == 0)
        return;

    size_t i, j, id, motor;

    for(i = 0, j = 0; i < TMC2300_REGISTER_COUNT; i++)
    size_t i, j = 0;
    for(i = 0; i < TMC2300_REGISTER_COUNT; i++)
    {
        // We only need to worry about hardware preset, write-only registers
        // We only need to worry about hardware preset registers
        // that have not yet been written (no dirty bit) here.
        if(tmc2300_registerAccess[i] != TMC2300_ACCESS_W_PRESET && tmc2300_registerAccess[i] != TMC2300_ACCESS_RW_PRESET)
        if(!TMC_IS_PRESET(tmc2300->registerAccess[i]) || TMC_IS_DIRTY(tmc2300->registerAccess[i]))
            continue;

        // Search the constant list for the current address. With the constant
        // list being sorted in ascended order, we can walk through the list
        // until the entry with an address equal or greater than i
        while(j < ARRAY_SIZE(tmc2300_RegisterConstants) && (tmc2300_RegisterConstants[j].address < i))
        while (j < ARRAY_SIZE(tmc2300_RegisterConstants) && (tmc2300_RegisterConstants[j].address < i))
        {
            j++;
        }

        // Abort when we reach the end of the constant list
        if (j == ARRAY_SIZE(tmc2300_RegisterConstants))
            break;

        // If we have an entry for our current address, write the constant
        if(tmc2300_RegisterConstants[j].address == i)
        if (tmc2300_RegisterConstants[j].address == i)
        {
            for (id = 0; id < TMC2300_IC_CACHE_COUNT; id++)
            {
                tmc2300_cache(id, TMC2300_CACHE_FILL_DEFAULT, i, &tmc2300_RegisterConstants[j].value);
            }
            tmc2300->config->shadowRegister[i] = tmc2300_RegisterConstants[j].value;
        }
    }
}
#else
// User must implement their own cache
extern bool tmc2300_cache(uint16_t icID, TMC2300CacheOp operation, uint8_t address, uint32_t *value);
#endif
#endif

/************************************************************* read / write Implementation *********************************************************************/
static void writeConfiguration(TMC2300TypeDef *tmc2300)
{
    uint8_t *ptr = &tmc2300->config->configIndex;
    const int32_t *settings;

static int32_t readRegisterUART(uint16_t icID, uint8_t registerAddress);
static void writeRegisterUART(uint16_t icID, uint8_t registerAddress, int32_t value);
static uint8_t CRC8(uint8_t *data, uint32_t bytes);
    if (tmc2300->config->state == CONFIG_RESET)
    {
        settings = tmc2300->registerResetState;
        // Find the next resettable register
        while((*ptr < TMC2300_REGISTER_COUNT) && !TMC_IS_RESETTABLE(tmc2300->registerAccess[*ptr]))
        {
            (*ptr)++;
        }
    }
    else
    {
        // Do not restore while in standby
        if (tmc2300->standbyEnabled)
            return;

        settings = tmc2300->config->shadowRegister;
        // Find the next restorable register
        while((*ptr < TMC2300_REGISTER_COUNT) && !TMC_IS_RESTORABLE(tmc2300->registerAccess[*ptr]))
        {
            (*ptr)++;
        }
    }

    if(*ptr < TMC2300_REGISTER_COUNT)
    {
        // Reset/restore the found register
        tmc2300_writeInt(tmc2300, *ptr, settings[*ptr]);
        (*ptr)++;
    }
    else
    {
        fillShadowRegisters(tmc2300);

        // Reset/restore complete -> call the callback if set
        if (tmc2300->config->callback)
        {
            ((tmc2300_callback)tmc2300->config->callback)(tmc2300, tmc2300->config->state);
        }

        if (tmc2300->config->state == CONFIG_RESET)
        {
            // Reset done -> Perform a restore
            tmc2300->config->state        = CONFIG_RESTORE;
            tmc2300->config->configIndex  = 0;
        }
        else
        {
            // Restore done -> configuration complete
            tmc2300->config->state = CONFIG_READY;
        }
    }
}

int32_t tmc2300_readRegister(uint16_t icID, uint8_t address)
void tmc2300_setRegisterResetState(TMC2300TypeDef *tmc2300, const int32_t *resetState)
{
    return readRegisterUART(icID, address);
    size_t i;
    for (i = 0; i < TMC2300_REGISTER_COUNT; i++)
    {
        tmc2300->registerResetState[i] = resetState[i];
    }
}
void tmc2300_writeRegister(uint16_t icID, uint8_t address, int32_t value)

void tmc2300_setCallback(TMC2300TypeDef *tmc2300, tmc2300_callback callback)
{
    writeRegisterUART(icID, address, value);
    tmc2300->config->callback = (tmc_callback_config) callback;
}

int32_t readRegisterUART(uint16_t icID, uint8_t registerAddress)
void tmc2300_periodicJob(TMC2300TypeDef *tmc2300, uint32_t tick)
{
    uint32_t value;
    registerAddress = registerAddress & TMC2300_ADDRESS_MASK;
    UNUSED(tick);

    if(tmc2300->config->state != CONFIG_READY)
    {
        writeConfiguration(tmc2300);
        return;
    }
}

    // Read from cache for registers with write-only access
    if (tmc2300_cache(icID, TMC2300_CACHE_READ, registerAddress, &value))
        return value;
uint8_t tmc2300_reset(TMC2300TypeDef *tmc2300)
{
    // A reset can always happen - even during another reset or restore

    uint8_t data[8] = { 0 };
    // Reset the dirty bits and wipe the shadow registers
    size_t i;
    for(i = 0; i < TMC2300_REGISTER_COUNT; i++)
    {
        tmc2300->registerAccess[i] &= ~TMC_ACCESS_DIRTY;
        tmc2300->config->shadowRegister[i] = 0;
    }

    // Activate the reset config mechanism
    tmc2300->config->state        = CONFIG_RESET;
    tmc2300->config->configIndex  = 0;

    data[0] = 0x05;
    data[1] = tmc2300_getNodeAddress(icID);
    data[2] = registerAddress;
    data[3] = CRC8(data, 3);
    return 1;
}

    // When the chip is in standby or use the shadow register content instead
    if (!tmc2300_readWriteUART(icID, &data[0], 4, 8))
        return tmc2300_shadowRegister[0][registerAddress];

    // Byte 0: Sync nibble correct?
    if (data[0] != 0x05)
uint8_t tmc2300_restore(TMC2300TypeDef *tmc2300)
{
    // Do not interrupt a reset
    // A reset will transition into a restore anyways
    if(tmc2300->config->state == CONFIG_RESET)
        return 0;

    // Byte 1: Master address correct?
    if (data[1] != 0xFF)
        return 0;
    tmc2300->config->state        = CONFIG_RESTORE;
    tmc2300->config->configIndex  = 0;

    // Byte 2: Address correct?
    if (data[2] != registerAddress)
        return 0;
    return 1;
}

    // Byte 7: CRC correct?
    if (data[7] != CRC8(data, 7))
        return 0;
uint8_t tmc2300_getSlaveAddress(TMC2300TypeDef *tmc2300)
{
    return tmc2300->slaveAddress;
}

    return ((uint32_t)data[3] << 24) | ((uint32_t)data[4] << 16) | (data[5] << 8) | data[6];
void tmc2300_setSlaveAddress(TMC2300TypeDef *tmc2300, uint8_t slaveAddress)
{
    tmc2300->slaveAddress = slaveAddress;
}

void writeRegisterUART(uint16_t icID, uint8_t registerAddress, int32_t value)
uint8_t tmc2300_getStandby(TMC2300TypeDef *tmc2300)
{
    uint8_t data[8];

    data[0] = 0x05;
    data[1] = tmc2300_getNodeAddress(icID);
    data[2] = registerAddress | TMC2300_WRITE_BIT;
    data[3] = (value >> 24) & 0xFF;
    data[4] = (value >> 16) & 0xFF;
    data[5] = (value >> 8 ) & 0xFF;
    data[6] = (value      ) & 0xFF;
    data[7] = CRC8(data, 7);

    tmc2300_readWriteUART(icID, &data[0], 8, 0);

    //Cache the registers with write-only access
    tmc2300_cache(icID, TMC2300_CACHE_WRITE, registerAddress, &value);

    return tmc2300->standbyEnabled;
}

static uint8_t CRC8(uint8_t *data, uint32_t bytes)
void tmc2300_setStandby(TMC2300TypeDef *tmc2300, uint8_t standbyState)
{
    uint8_t result = 0;

    while(bytes--)
        result = tmcCRCTable_Poly7Reflected[result ^ *data++];

    // Flip the result around
    // swap odd and even bits
    result = ((result >> 1) & 0x55) | ((result & 0x55) << 1);
    // swap consecutive pairs
    result = ((result >> 2) & 0x33) | ((result & 0x33) << 2);
    // swap nibbles ...
    result = ((result >> 4) & 0x0F) | ((result & 0x0F) << 4);

    return result;
    if (tmc2300->standbyEnabled && !standbyState)
    {
        // Just exited standby -> call the restore
        tmc2300_restore(tmc2300);
    }
    tmc2300->standbyEnabled = standbyState;
}
