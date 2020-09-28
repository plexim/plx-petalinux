#define ENCLUSTRA_MERCURY_ZX

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

/*Define CONFIG_ZYNQ_EEPROM here and its necessaries in u-boot menuconfig if you had EEPROM memory. */
#ifdef CONFIG_ZYNQ_EEPROM
#define CONFIG_SYS_I2C_EEPROM_ADDR_LEN         1
#define CONFIG_SYS_I2C_EEPROM_ADDR             0x54
#define CONFIG_SYS_EEPROM_PAGE_WRITE_BITS      4
#define CONFIG_SYS_EEPROM_PAGE_WRITE_DELAY_MS  5
#define CONFIG_SYS_EEPROM_SIZE                 1024 /* Bytes */
#define CONFIG_SYS_I2C_MUX_ADDR                0x74
#define CONFIG_SYS_I2C_MUX_EEPROM_SEL          0x4
#endif

/* ============== Added for Enclustra ZX5 ======================= */

/* Select which flash type currently uses Pins */
#define ZX_NONE    (0)
#define ZX_NAND    (1)
#define ZX_QSPI    (2)

#ifndef PHY_ANEG_TIMEOUT
#define PHY_ANEG_TIMEOUT	8000	/* PHY needs a longer aneg time */
#endif

#define MTDIDS_DEFAULT          "nand0=nand"
#define MTDPARTS_DEFAULT        "mtdparts=" \
	                                "nand:-(nand)"
#define CONFIG_MTD_DEVICE
#define CONFIG_MTD_PARTITIONS
#define ENVIRONMENT_SIZE "0x18000"

#ifndef CONFIG_CMD_ELF
#define CONFIG_CMD_ELF
#endif

/* QSPI Flash Memory Map */
#define QSPI_SIZE                  0x04000000 // We support only 64 MB flashes
#define QSPI_BOOT_OFFSET           0x00000000 // Storage for Bootimage
#define QSPI_BOOTARGS_SIZE         0x00080000 // size 512kB

#ifdef CONFIG_ENV_SIZE
#undef CONFIG_ENV_SIZE
#endif
#define CONFIG_ENV_SIZE QSPI_BOOTARGS_SIZE

#define CONFIG_ENV_FAT_DEVICE_AND_PART	"0:auto"
#define CONFIG_ENV_FAT_FILE	"uboot.env"
#define CONFIG_ENV_FAT_INTERFACE	"mmc"


#define ENCLUSTRA_MAC               0xF7B020

/* Default MAC address */
#define ENCLUSTRA_ETHADDR_DEFAULT "00:0A:35:01:02:03"
#define ENCLUSTRA_ETH1ADDR_DEFAULT "00:0A:35:01:02:04"

/* Only one USB controller supported  */
#ifdef CONFIG_USB_MAX_CONTROLLER_COUNT
#undef CONFIG_USB_MAX_CONTROLLER_COUNT
#endif
#define CONFIG_USB_MAX_CONTROLLER_COUNT 1

