
#include "I2cInterface.h"
#include "ModuleEeprom.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

EN_RESULT I2cClose();

int main(int argc, char* argv[])
{
   int ret = 0;
   InitialiseI2cInterface();
   ret = (Eeprom_Initialise() != EN_SUCCESS);
   if (ret)
   {
      fprintf(stderr, "EEprom initialization failed\n");
      return 1;
   }
   ret |= ret || (Eeprom_Read() != EN_SUCCESS);
   if (ret)
   {
      fprintf(stderr, "Error while reading EEprom\n");
      return 1;
   }
   if ((argc > 1) && strcmp(argv[1], "-s") == 0)
   {
      uint32_t serialNumber;
      ProductNumberInfo_t productNumber;
      uint64_t macAddress;
      ret = (Eeprom_GetModuleInfo(&serialNumber, &productNumber, &macAddress) != EN_SUCCESS);
      if (!ret)
         printf("%d\n", serialNumber);
   }
   else
      Eeprom_PrintModuleConfig();
   I2cClose();
   return ret;
}

