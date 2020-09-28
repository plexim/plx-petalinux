#include <configs/platform-auto.h>

#ifdef CONFIG_SYS_I2C_ZYNQ 
#undef CONFIG_SYS_I2C_ZYNQ
#endif

#ifdef CONFIG_SYS_I2C
#undef CONFIG_SYS_I2C
#endif

#ifdef CONFIG_ZYNQ_SDHCI0
#undef CONFIG_ZYNQ_SDHCI0
#endif

#define CONFIG_SYS_BOOTM_LEN 0xF000000

#define DFU_ALT_INFO_RAM \
                "dfu_ram_info=" \
        "setenv dfu_alt_info " \
        "image.ub ram $netstart 0x1e00000\0" \
        "dfu_ram=run dfu_ram_info && dfu 0 ram 0\0" \
        "thor_ram=run dfu_ram_info && thordown 0 ram 0\0"

#define DFU_ALT_INFO_MMC \
        "dfu_mmc_info=" \
        "set dfu_alt_info " \
        "${kernel_image} fat 0 1\\\\;" \
        "dfu_mmc=run dfu_mmc_info && dfu 0 mmc 0\0" \
        "thor_mmc=run dfu_mmc_info && thordown 0 mmc 0\0"

/*Required for uartless designs */
#ifndef CONFIG_BAUDRATE
#define CONFIG_BAUDRATE 115200
#ifdef CONFIG_DEBUG_UART
#undef CONFIG_DEBUG_UART
#endif
#endif

/*Define CONFIG_ZYNQMP_EEPROM here and its necessaries in u-boot menuconfig if you had EEPROM memory. */
#ifdef CONFIG_ZYNQMP_EEPROM
#define CONFIG_SYS_I2C_EEPROM_ADDR_LEN  1
#define CONFIG_CMD_EEPROM
#define CONFIG_ZYNQ_EEPROM_BUS          5
#define CONFIG_ZYNQ_GEM_EEPROM_ADDR     0x54
#define CONFIG_ZYNQ_GEM_I2C_MAC_OFFSET  0x20
#endif

/* ============== Added for Enclustra XU1 ======================= */

#undef CONFIG_PREBOOT
#define CONFIG_CMD_GPIO 1
#define CONFIG_ZYNQ_GPIO 1

/* U-Boot environment is placed at the end of the flash */
#define QSPI_BOOTARGS_OFFSET       QSPI_SIZE - QSPI_BOOTARGS_SIZE

/* QSPI Flash Memory Map */


#define QSPI_SIZE                  0x04000000 // We support only 64 MB flashes
#define QSPI_BOOT_OFFSET           0x00000000 // Storage for Bootimage
#define QSPI_BOOTARGS_SIZE         0x00080000 // size 512kB

/* U-Boot environment is placed at the end of the flash */
#define QSPI_BOOTARGS_OFFSET       QSPI_SIZE - QSPI_BOOTARGS_SIZE

#ifdef CONFIG_ENV_SIZE
#undef CONFIG_ENV_SIZE
#endif
#define CONFIG_ENV_SIZE QSPI_BOOTARGS_SIZE

#ifdef CONFIG_ENV_OFFSET
#undef CONFIG_ENV_OFFSET
#endif
#define CONFIG_ENV_OFFSET QSPI_BOOTARGS_OFFSET

#define CONFIG_ENV_SECT_SIZE CONFIG_ENV_SIZE

/* Default MAC address */
#define ENCLUSTRA_ETHADDR_DEFAULT "00:0A:35:01:02:03"
#define ENCLUSTRA_ETH1ADDR_DEFAULT "00:0A:35:01:02:04"
#define ENCLUSTRA_MAC               0xF7B020

#define CONFIG_SYS_SCSI_MAX_SCSI_ID     2
#define CONFIG_SYS_SCSI_MAX_LUN         1
#define CONFIG_SYS_SCSI_MAX_DEVICE      (CONFIG_SYS_SCSI_MAX_SCSI_ID * \
                                         CONFIG_SYS_SCSI_MAX_LUN)
#define CONFIG_SYS_SATA_MAX_DEVICE 1
/* #define CONFIG_BOARD_EARLY_INIT_F 1 */

#define CONFIG_CLOCKS
#define CONFIG_CLK_ZYNQMP



