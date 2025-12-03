
/********************************************************************************************************
 *  [FILE NAME]   :      <Bootloader.h>                                                                 *
 *  [AUTHOR]      :      <David S. Alexander>                                                           *
 *  [DATE CREATED]:      <Nov 18, 2023>                                                                 *
 *  [Description} :      <Header file for the STM32F103C6 Bootloader driver>                            *
 ********************************************************************************************************/

#ifndef _BOOTLOADER
#define _BOOTLOADER

/******************************************
 *              Includes                  *
 ******************************************/
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "usart.h"
#include "crc.h"

/******************************************
 *          Macro Declarations            *
 ******************************************/
#define BL_DEBUG_UART                &huart1
#define BL_HOST_COMMUNICATION_UART   &huart2
#define CRC_ENGINE_OBJ               &hcrc

#define DEBUG_INFO_DISABLE           0x00
#define DEBUG_INFO_ENABLE            0x01
#define BL_DEBUG_ENABLE              DEBUG_INFO_DISABLE

#define BL_HOST_BUFFER_RX_LENGTH     200

#define BL_ENABLE_UART_DEBUG_MESSAGE 0x00
#define BL_ENABLE_SPI_DEBUG_MESSAGE  0x01
#define BL_ENABLE_CAN_DEBUG_MESSAGE  0x02

#define BL_DEBUG_METHOD              (BL_ENABLE_UART_DEBUG_MESSAGE)

#define CBL_GET_VER_CMD              0x10
#define CBL_GET_HELP_CMD             0x11
#define CBL_GET_CID_CMD              0x12
/* Get Read Protection Status */
#define CBL_GET_RDP_STATUS_CMD       0x13
#define CBL_GO_TO_ADDR_CMD           0x14
#define CBL_FLASH_ERASE_CMD          0x15
#define CBL_MEM_WRITE_CMD            0x16
/* Enable/Disable Write Protection */
#define CBL_EN_RW_PROTECT_CMD        0x17
#define CBL_MEM_READ_CMD             0x18
/* Get PAGE Read/Write Protection Status */
#define CBL_READ_PAGE_STATUS_CMD     0x19
#define CBL_OTP_READ_CMD             0x20
/* Change Read Out Protection Level */
#define CBL_CHANGE_ROP_Level_CMD     0x21
/* Exit the bootloader after payload written successfully */
#define CBL_EXIT_CMD                 0x22

#define CBL_VENDOR_ID                100
#define CBL_SW_MAJOR_VERSION         1
#define CBL_SW_MINOR_VERSION         0
#define CBL_SW_PATCH_VERSION         0

/* CRC_VERIFICATION */
#define CRC_TYPE_SIZE_BYTE           0x04

#define CRC_VERIFICATION_FAILED      0x00
#define CRC_VERIFICATION_PASSED      0x01

#define CBL_SEND_NACK                0xAB
#define CBL_SEND_ACK                 0xCD

/* Start address of User App */
#define FLASH_USER_APP_BASE_ADDRESS  0x08004400U
#define USER_APP_FLAG_ADDR_BASE      0x08007C00U

#define ADDRESS_IS_INVALID           0x00
#define ADDRESS_IS_VALID             0x01

#define STM32F103_PAGE_SIZE          (1024)
#define STM32F103_SRAM_SIZE          (6 * STM32F103_PAGE_SIZE)
#define STM32F103_FLASH_SIZE         (32 * STM32F103_PAGE_SIZE)
#define STM32F103_SRAM_END           (SRAM_BASE + STM32F103_SRAM_SIZE)
#define STM32F103_FLASH_END          (FLASH_BASE + STM32F103_FLASH_SIZE)
#define SCB_VTOR_OFFSET              0x00
#define STM32F103_PAGE_MASK          0x0000FFFFU

#define APP_EXIST                    0xF1
#define APP_NOT_EXIST                0xF0
#define APP_FLAG_EXIST               0xF1
#define APP_FLAG_Not_EXIST           0xF0


/* CBL_FLASH_ERASE_CMD */
#define CBL_FLASH_MAX_PAGE_NUMBER    32
#define CBL_FLASH_MASS_ERASE         0xFF

#define INVALID_PAGE_NUMBER          0x00
#define VALID_PAGE_NUMBER            0x01
#define UNSUCCESSFUL_ERASE           0x02
#define SUCCESSFUL_ERASE             0x03

#define HAL_SUCCESSFUL_ERASE         0xFFFFFFFFU

/* CBL_MEM_WRITE_CMD */
#define FLASH_PAYLOAD_WRITE_FAILED   0x00
#define FLASH_PAYLOAD_WRITE_PASSED   0x01

#define FLASH_LOCK_WRITE_FAILED      0x00
#define FLASH_LOCK_WRITE_PASSED      0x01

/* CBL_GET_RDP_STATUS_CMD */
#define ROP_LEVEL_READ_INVALID       0x00
#define ROP_LEVE_READL_VALID         0X01

/* CBL_CHANGE_ROP_Level_CMD */
#define ROP_LEVEL_CHANGE_INVALID     0x00
#define ROP_LEVEL_CHANGE_VALID       0X01

#define CBL_ROP_LEVEL_0              0x00
#define CBL_ROP_LEVEL_1              0x01
#define CBL_ROP_LEVEL_2              0x02


/******************************************
 *          Datatype Declarations         *
 ******************************************/
typedef enum
{
    BL_NACK = 0,
    BL_OK
} BL_Status;

typedef void (*pMainApp)(void);
typedef void (*Jump_Ptr)(void);


/******************************************
 *    Software Interfaces Declarations    *
 ******************************************/
void BL_Print_Message(char *format, ...);
BL_Status BL_UART_Fetch_Host_Command(void);
void Bootloader_Jump_To_User_App(void);

#endif
