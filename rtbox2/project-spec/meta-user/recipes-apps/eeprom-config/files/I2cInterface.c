/** \file I2cInterface.c
 * \brief Implementation file for I2C interface functions.
 * \author Garry Jeromson
 * \date 18.06.15
 */

//-------------------------------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------------------------------

#include "I2cInterface.h"
#include "UtilityFunctions.h"
#include "ErrorCodes.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>


//-------------------------------------------------------------------------------------------------
// Constants
//-------------------------------------------------------------------------------------------------



//-------------------------------------------------------------------------------------------------
// Global variable definitions
//-------------------------------------------------------------------------------------------------

static int fd = 0;

//-------------------------------------------------------------------------------------------------
// Function definitions
//-------------------------------------------------------------------------------------------------



EN_RESULT InitialiseI2cInterface()
{
   static const char* devname = "/dev/i2c-0";
   fd = open(devname, O_RDWR);
   if (fd < 0)
   {
      fprintf(stderr, "Cannot open %s: %s\n", devname, strerror(errno));
      return EN_ERROR_FAILED_TO_INITIALISE_I2C_CONTROLLER;
   }
   return EN_SUCCESS;
}


EN_RESULT I2cWrite_NoSubAddress(uint8_t deviceAddress, const uint8_t* pWriteBuffer, uint32_t numberOfBytesToWrite)
{
    if (pWriteBuffer == NULL)
    {
        return EN_ERROR_NULL_POINTER;
    }

    if (numberOfBytesToWrite == 0)
    {
        return EN_ERROR_INVALID_ARGUMENT;
    }

   if (fd < 0)
      return EN_ERROR_NULL_POINTER;

   int ret = 0;

   if (ioctl(fd, I2C_SLAVE, deviceAddress) < 0) 
   {
      // fprintf(stderr, "ioctl failed: %s\n", strerror(errno));
      return EN_ERROR_FAILED_TO_SET_I2C_DEVICE_ADDRESS;
   }
   if (write(fd, pWriteBuffer, numberOfBytesToWrite) != numberOfBytesToWrite)
      return EN_ERROR_I2C_WRITE_FAILED;
   return EN_SUCCESS;
}

EN_RESULT I2cWrite_ByteSubAddress(uint8_t deviceAddress,
                                  uint8_t subAddress,
                                  const uint8_t* pWriteBuffer,
                                  uint32_t numberOfBytesToWrite)
{
    // Create a new array, to contain both the subaddress and the write data.
    uint8_t transferSizeBytes = numberOfBytesToWrite + 1;
    uint8_t transferData[transferSizeBytes];
    transferData[0] = subAddress;

    unsigned int dataByteIndex = 0;
    for (dataByteIndex = 0; dataByteIndex < numberOfBytesToWrite; dataByteIndex++)
    {
        transferData[dataByteIndex + 1] = pWriteBuffer[dataByteIndex];
    }

    EN_RETURN_IF_FAILED(I2cWrite_NoSubAddress(deviceAddress, (uint8_t*)&transferData, transferSizeBytes));

    return EN_SUCCESS;
}

EN_RESULT I2cWrite_TwoByteSubAddress(uint8_t deviceAddress,
                                     uint16_t subAddress,
                                     const uint8_t* pWriteBuffer,
                                     uint32_t numberOfBytesToWrite)
{
    // Create a new array, to contain both the subaddress and the write data.
    uint8_t transferSizeBytes = numberOfBytesToWrite + 2;
    uint8_t transferData[transferSizeBytes];
    transferData[0] = GetUpperByte(subAddress);
    transferData[1] = GetLowerByte(subAddress);

    unsigned int dataByteIndex = 0;
    for (dataByteIndex = 0; dataByteIndex < numberOfBytesToWrite; dataByteIndex++)
    {
        transferData[dataByteIndex + 2] = pWriteBuffer[dataByteIndex];
    }

    EN_RETURN_IF_FAILED(I2cWrite_NoSubAddress(deviceAddress, (uint8_t*)&transferData, transferSizeBytes));

    return EN_SUCCESS;
}

EN_RESULT I2cRead_NoSubAddress(uint8_t deviceAddress, uint8_t* pReadBuffer, uint32_t numberOfBytesToRead)
{

    if (pReadBuffer == NULL)
    {
        return EN_ERROR_NULL_POINTER;
    }

    if (numberOfBytesToRead == 0)
    {
        return EN_ERROR_INVALID_ARGUMENT;
    }

   if (fd < 0)
      return EN_ERROR_NULL_POINTER;

   int ret = 0;

   if (ioctl(fd, I2C_SLAVE, deviceAddress) < 0) 
   {
      // fprintf(stderr, "ioctl failed: %s\n", strerror(errno));
      return EN_ERROR_FAILED_TO_SET_I2C_DEVICE_ADDRESS;
   }
   if (read(fd, pReadBuffer, numberOfBytesToRead) != numberOfBytesToRead)
      return EN_ERROR_I2C_READ_FAILED;
   return EN_SUCCESS;
}

EN_RESULT I2cRead_ByteSubAddress(uint8_t deviceAddress,
                                 uint8_t subAddress,
                                 uint8_t* pReadBuffer,
                                 uint32_t numberOfBytesToRead)
{
    // Write the subaddress, with start condition asserted but stop condition not.
    EN_RETURN_IF_FAILED(I2cWrite_NoSubAddress(deviceAddress, (uint8_t*)&subAddress, 1));

    // Perform the read.
    EN_RETURN_IF_FAILED(I2cRead_NoSubAddress(deviceAddress, pReadBuffer, numberOfBytesToRead));

    return EN_SUCCESS;
}

EN_RESULT I2cRead_WordSubAddress(uint8_t deviceAddress,
                                 uint16_t subAddress,
                                 uint8_t* pReadBuffer,
                                 uint32_t numberOfBytesToRead)
{
    // Write the subaddress, with start condition asserted but stop condition not.
    EN_RETURN_IF_FAILED(I2cWrite_NoSubAddress(deviceAddress, (uint8_t*)&subAddress, 2));

    // Perform the read.
    EN_RETURN_IF_FAILED(I2cRead_NoSubAddress(deviceAddress, pReadBuffer, numberOfBytesToRead));

    return EN_SUCCESS;
}

EN_RESULT I2cRead(uint8_t deviceAddress,
                  uint16_t subAddress,
                  EI2cSubAddressMode_t subAddressMode,
                  uint32_t numberOfBytesToRead,
                  uint8_t* pReadBuffer)
{

    switch (subAddressMode)
    {
    case EI2cSubAddressMode_None:
    {
        EN_RETURN_IF_FAILED(I2cRead_NoSubAddress(deviceAddress, pReadBuffer, numberOfBytesToRead));
        break;
    }
    case EI2cSubAddressMode_OneByte:
    {
        EN_RETURN_IF_FAILED(
            I2cRead_ByteSubAddress(deviceAddress, (uint8_t)subAddress, pReadBuffer, numberOfBytesToRead));
        break;
    }
    case EI2cSubAddressMode_TwoBytes:
    {
        EN_RETURN_IF_FAILED(
            I2cRead_WordSubAddress(deviceAddress, (uint16_t)subAddress, pReadBuffer, numberOfBytesToRead));
    }
    default:
        break;
    }

    return EN_SUCCESS;
}

EN_RESULT I2cWrite(uint8_t deviceAddress,
                   uint16_t subAddress,
                   EI2cSubAddressMode_t subAddressMode,
                   const uint8_t* pWriteBuffer,
                   uint32_t numberOfBytesToWrite)
{

    switch (subAddressMode)
    {
    case EI2cSubAddressMode_None:
    {
        EN_RETURN_IF_FAILED(I2cWrite_NoSubAddress(deviceAddress, pWriteBuffer, numberOfBytesToWrite));
        break;
    }
    case EI2cSubAddressMode_OneByte:
    {
        EN_RETURN_IF_FAILED(
            I2cWrite_ByteSubAddress(deviceAddress, (uint8_t)subAddress, pWriteBuffer, numberOfBytesToWrite));
        break;
    }
    case EI2cSubAddressMode_TwoBytes:
    {
        EN_RETURN_IF_FAILED(I2cWrite_TwoByteSubAddress(deviceAddress, subAddress, pWriteBuffer, numberOfBytesToWrite));
        break;
    }
    default:
        break;
    }

    return EN_SUCCESS;
}

EN_RESULT I2cClose()
{
   if (fd > 0)
      close(fd);
   fd = 0;
   return EN_SUCCESS;
}

