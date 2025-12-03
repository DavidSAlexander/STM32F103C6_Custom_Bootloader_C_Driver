
/********************************************************************************************************
 *  [FILE NAME]   :      <Bootloader.c>                                                                 *
 *  [AUTHOR]      :      <David S. Alexander>                                                           *
 *  [DATE CREATED]:      <Nov 18, 2023>                                                                 *
 *  [Description] :      <Source file for the STM32F103C6 Bootloader driver>                            *
 ********************************************************************************************************/

/******************************************
 *              Includes                  *
 ******************************************/
#include "Bootloader.h"


/******************************************
 *      Static Functions Decleration      *
 ******************************************/

 /* Main Commands Functions */
static void Bootloader_Get_Version(uint8_t *Host_Buffer);                                         /* Read the bootloader version, e.g., 1.1.0 */
static void Bootloader_Get_Help(uint8_t *Host_Buffer);                                            /* Read the available bootloader supported commands list */
static void Bootloader_Get_Chip_Identification_Number(uint8_t *Host_Buffer);                      /* Read the chip identification number */
static void Bootloader_Read_Protection_Level(uint8_t *Host_Buffer);                               /* Read the bootloader protection level */
static void Bootloader_Jump_To_Address(uint8_t *Host_Buffer);                                     /* Jump to a specific address */
static void Bootloader_Erase_Flash(uint8_t *Host_Buffer);                                         /* Erase the flash memory */
static void Bootloader_Memory_Write(uint8_t *Host_Buffer);                                        /* Write data to memory */
static void Bootloader_Enable_RW_Protection(uint8_t *Host_Buffer);                                /* Enable read-write protection */
static void Bootloader_Memory_Read(uint8_t *Host_Buffer);                                         /* Read data from memory */
static void Bootloader_Read_Page_Protection_Status(uint8_t *Host_Buffer);                         /* Read the page protection status */
static void Bootloader_Read_OTP(uint8_t *Host_Buffer);                                            /* Read data from OTP (One-Time Programmable) memory */
static void Bootloader_Change_Read_Protection_Level(uint8_t *Host_Buffer);                        /* Change the read protection level */

 /* Helper Functions */
static uint8_t Bootloader_CRC_Verify(uint8_t *pData, uint32_t Data_Len, uint32_t Host_CRC);       /* Verify CRC (Cyclic Redundancy Check) of data */
static void Bootloader_Send_ACK(uint8_t Replay_Len);                                              /* Send ACK (Acknowledgment) with specified length */
static void Bootloader_Send_NACK(void);                                                           /* Send NACK (Not Acknowledgment) */
static void Bootloader_Send_Data_To_Host(uint8_t *Host_Buffer, uint32_t Data_Len);                /* Send data to the host with specified length */
static uint8_t Host_Address_Verification(uint32_t Jump_Address);                                  /* Verify the validity of the jump address */
static uint8_t Perform_Flash_Erase(uint8_t Page_Numebr, uint8_t Number_Of_Pages);                 /* Perform flash erase for specified page and number of pages */
static uint8_t Bootloader_Write_Flag(uint32_t FlagAddr, uint8_t FlagVal);                         /* Write a flag value to a specified address */


/******************************************
 *      Global Variables Definitions      *
 ******************************************/

static uint8_t BL_Host_Buffer[BL_HOST_BUFFER_RX_LENGTH];
static uint8_t Bootloader_Supported_CMDs[13] =
{
    CBL_GET_VER_CMD,
    CBL_GET_HELP_CMD,
    CBL_GET_CID_CMD,
    CBL_GET_RDP_STATUS_CMD,
    CBL_GO_TO_ADDR_CMD,
    CBL_FLASH_ERASE_CMD,
    CBL_MEM_WRITE_CMD,
    CBL_EN_RW_PROTECT_CMD,
    CBL_MEM_READ_CMD,
    CBL_READ_PAGE_STATUS_CMD,
    CBL_OTP_READ_CMD,
	CBL_CHANGE_ROP_Level_CMD,
	CBL_EXIT_CMD
};


/******************************************
 *     Software Interfaces Definitions    *
 ******************************************/

void BL_Print_Message(char *format, ...)
{
    char Messsage[100] = {0};
    /* holds the information needed by va_start, va_arg, va_end */
    va_list args;
    /* Enables access to the variable arguments */
    va_start(args, format);
    /* Write formatted data from variable argument list to string */
    vsprintf(Messsage, format, args);
#if (BL_DEBUG_METHOD == BL_ENABLE_UART_DEBUG_MESSAGE)
    /* Trasmit the formatted data through the defined UART */
    HAL_UART_Transmit(BL_DEBUG_UART, (uint8_t *)Messsage, sizeof(Messsage), HAL_MAX_DELAY);
#elif (BL_DEBUG_METHOD == BL_ENABLE_SPI_DEBUG_MESSAGE)
    /* Trasmit the formatted data through the defined SPI */
#elif (BL_DEBUG_METHOD == BL_ENABLE_CAN_DEBUG_MESSAGE)
    /* Trasmit the formatted data through the defined CAN */
#endif
    /* Performs cleanup for an ap object initialized by a call to va_start */
    va_end(args);
}

BL_Status BL_UART_Fetch_Host_Command(void)
{
    BL_Status Status = BL_NACK;
    HAL_StatusTypeDef HAL_Status = HAL_ERROR;
    uint8_t Data_Length = 0;

    memset(BL_Host_Buffer, 0, BL_HOST_BUFFER_RX_LENGTH);
    /* Read the length of the command packet received from the HOST */
    HAL_Status = HAL_UART_Receive(BL_HOST_COMMUNICATION_UART, BL_Host_Buffer, 1, HAL_MAX_DELAY);
    if(HAL_Status != HAL_OK)
    {
        Status = BL_NACK;
    }
    else
    {
        Data_Length = BL_Host_Buffer[0];
        /* Read the command packet received from the HOST */
        HAL_Status = HAL_UART_Receive(BL_HOST_COMMUNICATION_UART, &BL_Host_Buffer[1], Data_Length, HAL_MAX_DELAY);
        if(HAL_Status != HAL_OK)
        {
            Status = BL_NACK;
        }
        else
        {
            switch(BL_Host_Buffer[1])
            {
            case CBL_GET_VER_CMD:
                Bootloader_Get_Version(BL_Host_Buffer);
                Status = BL_OK;
                break;
            case CBL_GET_HELP_CMD:
                Bootloader_Get_Help(BL_Host_Buffer);
                Status = BL_OK;
                break;
            case CBL_GET_CID_CMD:
                Bootloader_Get_Chip_Identification_Number(BL_Host_Buffer);
                Status = BL_OK;
                break;
            case CBL_GET_RDP_STATUS_CMD:
                Bootloader_Read_Protection_Level(BL_Host_Buffer);
                Status = BL_OK;
                break;
            case CBL_GO_TO_ADDR_CMD:
                Bootloader_Jump_To_Address(BL_Host_Buffer);
                Status = BL_OK;
                break;
            case CBL_FLASH_ERASE_CMD:
                Bootloader_Erase_Flash(BL_Host_Buffer);
                Status = BL_OK;
                break;
            case CBL_MEM_WRITE_CMD:
                Bootloader_Memory_Write(BL_Host_Buffer);
                Status = BL_OK;
                break;
            case CBL_EN_RW_PROTECT_CMD:
                BL_Print_Message("Enable or Disable write protect on different sectors of the user flash \r\n");
                Bootloader_Enable_RW_Protection(BL_Host_Buffer);
                Status = BL_OK;
                break;
            case CBL_MEM_READ_CMD:
                BL_Print_Message("Read data from different memories of the microcontroller \r\n");
                Bootloader_Memory_Read(BL_Host_Buffer);
                Status = BL_OK;
                break;
            case CBL_READ_PAGE_STATUS_CMD:
                BL_Print_Message("Read all the PAGE protection status \r\n");
                Bootloader_Read_Page_Protection_Status(BL_Host_Buffer);
                Status = BL_OK;
                break;
            case CBL_OTP_READ_CMD:
                BL_Print_Message("Read the OTP contents \r\n");
                Bootloader_Read_OTP(BL_Host_Buffer);
                Status = BL_OK;
                break;
            case CBL_CHANGE_ROP_Level_CMD:
                Bootloader_Change_Read_Protection_Level(BL_Host_Buffer);
                Status = BL_OK;
                break;
            case CBL_EXIT_CMD:
            	Bootloader_Write_Flag((uint32_t)USER_APP_FLAG_ADDR_BASE, (uint8_t)APP_FLAG_EXIST);
                Status = BL_OK;
                break;
            default:
                BL_Print_Message("Invalid command code received from host !! \r\n");
                break;
            }
        }
    }
    return Status;
}

void Bootloader_Jump_To_User_App(void)
{
    /* Value of the main stack pointer of our main application */
    uint32_t MSP_Value = *((volatile uint32_t *)FLASH_USER_APP_BASE_ADDRESS);

    /* Reset Handler definition function of our main application */
    uint32_t MainAppAddr = *((volatile uint32_t *)(FLASH_USER_APP_BASE_ADDRESS + 4));

    /* Fetch the reset handler address of the user application */
    pMainApp ResetHandler_Address = (pMainApp)MainAppAddr;

    /* Set Main Stack Pointer */
    __set_MSP(MSP_Value);

    /* Set the Vector Table Offset Register (VTOR) to the base address of the user application */
    SCB->VTOR = FLASH_USER_APP_BASE_ADDRESS + SCB_VTOR_OFFSET;

    /* Deinitialize/Disable modules if needed */
    HAL_RCC_DeInit(); /* Deinitialize the RCC clock configuration to the default reset state. */

    /* Jump to Application Reset Handler */
    ResetHandler_Address();
}

/******************************************
 *  Static Helper Functions Definitions   *
 ******************************************/


static uint8_t Bootloader_CRC_Verify(uint8_t *pData, uint32_t Data_Len, uint32_t Host_CRC)
{
	uint8_t CRC_Status = CRC_VERIFICATION_FAILED;
	uint32_t MCU_CRC_Calculated = 0;
	uint32_t Data_Counter = 0;
	uint32_t Data_Buffer = 0;
	/* Calculate CRC32 */
	for(Data_Counter = 0; Data_Counter < Data_Len; Data_Counter++)
	{
		Data_Buffer = (uint32_t)pData[Data_Counter];
		MCU_CRC_Calculated = HAL_CRC_Accumulate(CRC_ENGINE_OBJ, &Data_Buffer, 1);
	}
	/* Reset the CRC Calculation Unit */
     __HAL_CRC_DR_RESET(CRC_ENGINE_OBJ);
	/* Compare the Host CRC and Calculated CRC */
	if(MCU_CRC_Calculated == Host_CRC)
	{
		CRC_Status = CRC_VERIFICATION_PASSED;
	}
	else
	{
		CRC_Status = CRC_VERIFICATION_FAILED;
	}
	return CRC_Status;
}


static void Bootloader_Send_ACK(uint8_t Replay_Len)
{
    uint8_t Ack_Value[2] = {0};
    Ack_Value[0] = CBL_SEND_ACK;
    Ack_Value[1] = Replay_Len;
    HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART, (uint8_t *)Ack_Value, 2, HAL_MAX_DELAY);
}

static void Bootloader_Send_NACK(void)
{
    uint8_t Ack_Value = CBL_SEND_NACK;
    HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART, &Ack_Value, 1, HAL_MAX_DELAY);
}

static void Bootloader_Send_Data_To_Host(uint8_t *Host_Buffer, uint32_t Data_Len)
{
    HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART, Host_Buffer, Data_Len, HAL_MAX_DELAY);
}

static void Bootloader_Get_Version(uint8_t *Host_Buffer)
{
    uint8_t BL_Version[4] = { CBL_VENDOR_ID, CBL_SW_MAJOR_VERSION, CBL_SW_MINOR_VERSION, CBL_SW_PATCH_VERSION };
    uint16_t Host_CMD_Packet_Len = 0;
    uint32_t Host_CRC32 = 0;

#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
    BL_Print_Message("Read the bootloader version from the MCU \r\n");
#endif
    /* Extract the CRC32 and packet length sent by the HOST */
    Host_CMD_Packet_Len = Host_Buffer[0] + 1;
    Host_CRC32 = *((uint32_t *)((Host_Buffer + Host_CMD_Packet_Len) - CRC_TYPE_SIZE_BYTE));
    /* CRC Verification */
    if(CRC_VERIFICATION_PASSED == Bootloader_CRC_Verify((uint8_t *)&Host_Buffer[0] , Host_CMD_Packet_Len - 4, Host_CRC32))
    {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("CRC Verification Passed \r\n");
#endif
        Bootloader_Send_ACK(4);
        Bootloader_Send_Data_To_Host((uint8_t *)(&BL_Version[0]), 4);
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("Bootloader Ver. %d.%d.%d \r\n", BL_Version[1], BL_Version[2], BL_Version[3]);
#endif
    }
    else
    {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("CRC Verification Failed \r\n");
#endif
        Bootloader_Send_NACK();
    }
}

static void Bootloader_Get_Help(uint8_t *Host_Buffer)
{
    uint16_t Host_CMD_Packet_Len = 0;
    uint32_t Host_CRC32 = 0;
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
    BL_Print_Message("Read the commands supported by the bootloader \r\n");
#endif
    /* Extract the CRC32 and packet length sent by the HOST */
    Host_CMD_Packet_Len = Host_Buffer[0] + 1;
    Host_CRC32 = *((uint32_t *)((Host_Buffer + Host_CMD_Packet_Len) - CRC_TYPE_SIZE_BYTE));
    /* CRC Verification */
    if(CRC_VERIFICATION_PASSED == Bootloader_CRC_Verify((uint8_t *)&Host_Buffer[0] , Host_CMD_Packet_Len - 4, Host_CRC32))
    {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("CRC Verification Passed \r\n");
#endif
        Bootloader_Send_ACK(12);
        Bootloader_Send_Data_To_Host((uint8_t *)(&Bootloader_Supported_CMDs[0]), 12);
    }
    else
    {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("CRC Verification Failed \r\n");
#endif
        Bootloader_Send_NACK();
    }
}

static void Bootloader_Get_Chip_Identification_Number(uint8_t *Host_Buffer)
{
    uint16_t Host_CMD_Packet_Len = 0;
    uint32_t Host_CRC32 = 0;
    uint16_t MCU_Identification_Number = 0;
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
    BL_Print_Message("Read the MCU chip identification number \r\n");
#endif
    /* Extract the CRC32 and packet length sent by the HOST */
    Host_CMD_Packet_Len = Host_Buffer[0] + 1;
    Host_CRC32 = *((uint32_t *)((Host_Buffer + Host_CMD_Packet_Len) - CRC_TYPE_SIZE_BYTE));
    /* CRC Verification */
    if(CRC_VERIFICATION_PASSED == Bootloader_CRC_Verify((uint8_t *)&Host_Buffer[0] , Host_CMD_Packet_Len - 4, Host_CRC32))
    {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("CRC Verification Passed \r\n");
#endif
        /* Get the MCU chip identification number */
        MCU_Identification_Number = (uint16_t)((DBGMCU->IDCODE) & 0x00000FFF);
        /* Report chip identification number to HOST */
        Bootloader_Send_ACK(2);
        Bootloader_Send_Data_To_Host((uint8_t *)&MCU_Identification_Number, 2);
    }
    else
    {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("CRC Verification Failed \r\n");
#endif
        Bootloader_Send_NACK();
    }
}

static uint8_t Host_Address_Verification(uint32_t Jump_Address)
{
    uint8_t Address_Verification = ADDRESS_IS_INVALID;
    if((Jump_Address >= SRAM_BASE) && (Jump_Address <= STM32F103_SRAM_END))
    {
        Address_Verification = ADDRESS_IS_VALID;
    }
    else if(Jump_Address >= SRAM_BASE)
    {
        Address_Verification = ADDRESS_IS_VALID;
    }
    else if((Jump_Address >= FLASH_BASE) && (Jump_Address <= STM32F103_FLASH_END))
    {
        Address_Verification = ADDRESS_IS_VALID;
    }
    else
    {
        Address_Verification = ADDRESS_IS_INVALID;
    }
    return Address_Verification;
}

static void Bootloader_Jump_To_Address(uint8_t *Host_Buffer)
{
    uint16_t Host_CMD_Packet_Len = 0;
    uint32_t Host_CRC32 = 0;
    uint32_t HOST_Jump_Address = 0;
    uint8_t Address_Verification = ADDRESS_IS_INVALID;

#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
    BL_Print_Message("Jump bootloader to specified address \r\n");
#endif
    /* Extract the CRC32 and packet length sent by the HOST */
    Host_CMD_Packet_Len = Host_Buffer[0] + 1;
    Host_CRC32 = *((uint32_t *)((Host_Buffer + Host_CMD_Packet_Len) - CRC_TYPE_SIZE_BYTE));
    /* CRC Verification */
    if(CRC_VERIFICATION_PASSED == Bootloader_CRC_Verify((uint8_t *)&Host_Buffer[0] , Host_CMD_Packet_Len - 4, Host_CRC32))
    {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("CRC Verification Passed \r\n");
#endif
        Bootloader_Send_ACK(1);
        /* Extract the address form the HOST packet */
        HOST_Jump_Address = *((uint32_t *)&Host_Buffer[2]);
        BL_Print_Message("Jump to : 0x%X \r\n", HOST_Jump_Address);
        /* Verify the Extracted address to be valid address */
        Address_Verification = Host_Address_Verification(HOST_Jump_Address);
        if(ADDRESS_IS_VALID == Address_Verification)
        {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
            BL_Print_Message("Address verification succeeded \r\n");
#endif
            /* Report address verification succeeded */
            Bootloader_Send_Data_To_Host((uint8_t *)&Address_Verification, 1);
            /* Prepare the address to jump */
            Jump_Ptr Jump_Address = (Jump_Ptr)(HOST_Jump_Address + 1);
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
            BL_Print_Message("Jump to : 0x%X \r\n", Jump_Address);
#endif
            Jump_Address();
        }
        else
        {
            /* Report address verification failed */
            Bootloader_Send_Data_To_Host((uint8_t *)&Address_Verification, 1);
        }
    }
    else
    {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("CRC Verification Failed \r\n");
#endif
        Bootloader_Send_NACK();
    }
}

static uint8_t CBL_STM32F103_Get_RDP_Level()
{
    FLASH_OBProgramInitTypeDef FLASH_OBProgram;
    /* Get the Option byte configuration */
    HAL_FLASHEx_OBGetConfig(&FLASH_OBProgram);

    return (uint8_t)(FLASH_OBProgram.RDPLevel);
}

static void Bootloader_Read_Protection_Level(uint8_t *Host_Buffer)
{
    uint16_t Host_CMD_Packet_Len = 0;
    uint32_t Host_CRC32 = 0;
    uint8_t RDP_Level = 0;

#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
    BL_Print_Message("Read the FLASH Read Protection Out level \r\n");
#endif
    /* Extract the CRC32 and packet length sent by the HOST */
    Host_CMD_Packet_Len = Host_Buffer[0] + 1;
    Host_CRC32 = *((uint32_t *)((Host_Buffer + Host_CMD_Packet_Len) - CRC_TYPE_SIZE_BYTE));
    /* CRC Verification */
    if(CRC_VERIFICATION_PASSED == Bootloader_CRC_Verify((uint8_t *)&Host_Buffer[0] , Host_CMD_Packet_Len - 4, Host_CRC32))
    {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("CRC Verification Passed \r\n");
#endif
        Bootloader_Send_ACK(1);
        /* Read Protection Level */
        RDP_Level = CBL_STM32F103_Get_RDP_Level();
        /* Report Valid Protection Level */
        Bootloader_Send_Data_To_Host((uint8_t *)&RDP_Level, 1);
    }
    else
    {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("CRC Verification Failed \r\n");
#endif
        Bootloader_Send_NACK();
    }
}

static uint8_t Perform_Flash_Erase(uint8_t Page_Numebr, uint8_t Number_Of_Pages)
{
    uint8_t Page_Validity_Status = INVALID_PAGE_NUMBER;
    FLASH_EraseInitTypeDef pEraseInit;
    uint8_t Remaining_Pages = 0;
    HAL_StatusTypeDef HAL_Status = HAL_ERROR;
    uint32_t PageError = 0;

    if(Number_Of_Pages > CBL_FLASH_MAX_PAGE_NUMBER)
    {
        /* Number Of sectors is out of range */
        Page_Validity_Status = INVALID_PAGE_NUMBER;
    }
    else
    {
        if((Page_Numebr <= (CBL_FLASH_MAX_PAGE_NUMBER - 1)) || (CBL_FLASH_MASS_ERASE == Page_Numebr))
        {
            /* Check if user needs Mass erase */
            if(CBL_FLASH_MASS_ERASE == Page_Numebr)
            {
                pEraseInit.TypeErase = FLASH_TYPEERASE_MASSERASE; /* Flash Mass erase activation */
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
                BL_Print_Message("Flash Mass erase activation \r\n");
#endif
            }
            else
            {
                /* User needs PAGE erase */
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
                BL_Print_Message("User needs PAGE erase \r\n");
#endif
                Remaining_Pages = CBL_FLASH_MAX_PAGE_NUMBER - Page_Numebr;
                if(Number_Of_Pages > Remaining_Pages)
                {
                    Number_Of_Pages = Remaining_Pages;
                }
                else
                {
                    /* Nothing */
                }

                pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES; /* Pages erase only */
                pEraseInit.Banks = FLASH_BANK_1;              /* Bank 1  */
                pEraseInit.PageAddress = (uint32_t)(Page_Numebr * 1024) + FLASH_BASE;         /* Initial FLASH PAGE to erase when Mass erase is disabled */
                pEraseInit.NbPages = Number_Of_Pages;         /* Number of sectors to be erased. */
                BL_Print_Message("Flash erase activation Page 0x%X \r\n",pEraseInit.PageAddress);
            }
            /* Unlock the FLASH control register access */
            HAL_Status = HAL_FLASH_Unlock();
            /* Perform a mass erase or erase the specified FLASH memory sectors */
            HAL_Status = HAL_FLASHEx_Erase(&pEraseInit, &PageError);
            if(HAL_SUCCESSFUL_ERASE == PageError && HAL_Status == HAL_OK)
            {
                Page_Validity_Status = SUCCESSFUL_ERASE;
            }
            else
            {
                Page_Validity_Status = UNSUCCESSFUL_ERASE;
            }
            /* Locks the FLASH control register access */
            HAL_Status = HAL_FLASH_Lock();
        }
        else
        {
            Page_Validity_Status = UNSUCCESSFUL_ERASE;
        }
    }
    return Page_Validity_Status;
}

static void Bootloader_Erase_Flash(uint8_t *Host_Buffer)
{
    uint16_t Host_CMD_Packet_Len = 0;
    uint32_t Host_CRC32 = 0;
    uint8_t Erase_Status = 0;

#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
    BL_Print_Message("Mass erase or Page erase of the user flash \r\n");
#endif
    /* Extract the CRC32 and packet length sent by the HOST */
    Host_CMD_Packet_Len = Host_Buffer[0] + 1;
    Host_CRC32 = *((uint32_t *)((Host_Buffer + Host_CMD_Packet_Len) - CRC_TYPE_SIZE_BYTE));
    /* CRC Verification */
    if(CRC_VERIFICATION_PASSED == Bootloader_CRC_Verify((uint8_t *)&Host_Buffer[0] , Host_CMD_Packet_Len - 4, Host_CRC32))
    {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("CRC Verification Passed \r\n");
#endif
        /* Send acknowledgement to the HOST */
        Bootloader_Send_ACK(1);
        /* Perform Mass erase or PAGE erase of the user flash */
        BL_Print_Message("Pages = %d \r\n",Host_Buffer[2]);
        Erase_Status = Perform_Flash_Erase(Host_Buffer[2], Host_Buffer[3]);
        if(SUCCESSFUL_ERASE == Erase_Status)
        {
            /* Report erase Passed */
            Bootloader_Send_Data_To_Host((uint8_t *)&Erase_Status, 1);
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
            BL_Print_Message("Successful Erase \r\n");
#endif
        }
        else
        {
            /* Report erase failed */
            Bootloader_Send_Data_To_Host((uint8_t *)&Erase_Status, 1);
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
            BL_Print_Message("Erase request failed !!\r\n");
#endif
        }
    }
    else
    {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("CRC Verification Failed \r\n");
#endif
        /* Send Not acknowledge to the HOST */
        Bootloader_Send_NACK();
    }
}

static uint8_t Flash_Memory_Write_Payload(uint8_t *Host_Payload, uint32_t Payload_Start_Address, uint16_t Payload_Len)
{
    HAL_StatusTypeDef HAL_Status = HAL_ERROR;
    uint8_t Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
    uint8_t Payload_Counter = 0;

    /* Unlock the FLASH control register access */
    HAL_Status = HAL_FLASH_Unlock();

    if(HAL_Status != HAL_OK)
    {
        Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
    }
    else
    {
    	BL_Print_Message("Code Length = %d ! \r\n",Payload_Len);
    	/*if (Payload_Len % 2 != 0) Payload_Len++;
    	Payload_Len /= 2;*/
        for(Payload_Counter = 0; Payload_Counter < Payload_Len; Payload_Counter+=2)
        {
            /* Program a half-word at a specified address */
        	BL_Print_Message("Payload_Start_Address + Payload_Counter = 0x0%X ! \r\n",Payload_Start_Address + Payload_Counter);
            HAL_Status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, Payload_Start_Address + Payload_Counter, *((uint16_t*)(&Host_Payload[Payload_Counter])));
            if(HAL_Status != HAL_OK)
            {
                Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
                break;
            }
            else
            {
                Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_PASSED;
                BL_Print_Message("Address 0x%X passed! \r\n", Payload_Start_Address + Payload_Counter);
            }
        }
    }

    if((FLASH_PAYLOAD_WRITE_PASSED == Flash_Payload_Write_Status) && (HAL_OK == HAL_Status))
    {
        /* Locks the FLASH control register access */
        HAL_Status = HAL_FLASH_Lock();
        if(HAL_Status != HAL_OK)
        {
            Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
        }
        else
        {
            Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_PASSED;
        }
    }
    else
    {
        Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
    }

    return Flash_Payload_Write_Status;
}

static void Bootloader_Memory_Write(uint8_t *Host_Buffer)
{
    uint16_t Host_CMD_Packet_Len = 0;
    uint32_t Host_CRC32 = 0;
    uint32_t HOST_Address = 0;
    uint8_t Payload_Len = 0;
    uint8_t Address_Verification = ADDRESS_IS_INVALID;
    uint8_t Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;

#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
    BL_Print_Message("Write data into different memories of the MCU \r\n");
#endif
    /* Extract the CRC32 and packet length sent by the HOST */
    Host_CMD_Packet_Len = Host_Buffer[0] + 1;
    Host_CRC32 = *((uint32_t *)((Host_Buffer + Host_CMD_Packet_Len) - CRC_TYPE_SIZE_BYTE));
    /* CRC Verification */
    if(CRC_VERIFICATION_PASSED == Bootloader_CRC_Verify((uint8_t *)&Host_Buffer[0] , Host_CMD_Packet_Len - 4, Host_CRC32))
    {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("CRC Verification Passed \r\n");
#endif
        /* Send acknowledgement to the HOST */
        Bootloader_Send_ACK(1);
        /* Extract the start address from the Host packet */
        HOST_Address = *((uint32_t *)(&Host_Buffer[2]));
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("HOST_Address = 0x%X \r\n", HOST_Address);
#endif
        /* Extract the payload length from the Host packet */
        Payload_Len = Host_Buffer[6];
        /* Verify the Extracted address to be valid address */
        Address_Verification = Host_Address_Verification(HOST_Address);
        if(ADDRESS_IS_VALID == Address_Verification)
        {
            /* Write the payload to the Flash memory */
            Flash_Payload_Write_Status = Flash_Memory_Write_Payload((uint8_t *)&Host_Buffer[7], HOST_Address, Payload_Len);
            if(FLASH_PAYLOAD_WRITE_PASSED == Flash_Payload_Write_Status)
            {
                /* Report payload write passed */
                Bootloader_Send_Data_To_Host((uint8_t *)&Flash_Payload_Write_Status, 1);
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
                BL_Print_Message("Payload Valid \r\n");
#endif
            }
            else
            {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
                BL_Print_Message("Payload InValid \r\n");
#endif
                /* Report payload write failed */
                Bootloader_Send_Data_To_Host((uint8_t *)&Flash_Payload_Write_Status, 1);
            }
        }
        else
        {
            /* Report address verification failed */
            Address_Verification = ADDRESS_IS_INVALID;
            Bootloader_Send_Data_To_Host((uint8_t *)&Address_Verification, 1);
        }
    }
    else
    {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("CRC Verification Failed \r\n");
#endif
        /* Send Not acknowledge to the HOST */
        Bootloader_Send_NACK();
    }
}

static void Bootloader_Memory_Read(uint8_t *Host_Buffer)
{
    uint16_t Host_CMD_Packet_Len = 0;
    uint32_t Host_CRC32 = 0;
    uint32_t address = 0;
    uint32_t length = 0;

#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
    BL_Print_Message("Memory Read Operation\r\n");
#endif

    /* Extract the CRC32 and packet length sent by the HOST */
    Host_CMD_Packet_Len = Host_Buffer[0] + 1;
    Host_CRC32 = *((uint32_t *)((Host_Buffer + Host_CMD_Packet_Len) - CRC_TYPE_SIZE_BYTE));

    /* CRC Verification */
    if (CRC_VERIFICATION_PASSED == Bootloader_CRC_Verify((uint8_t *)&Host_Buffer[0], Host_CMD_Packet_Len - 4, Host_CRC32))
    {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("CRC Verification Passed\r\n");
#endif
        Bootloader_Send_ACK(1);

        /* Extract the address and length from the host buffer */
        address = *((uint32_t *)&Host_Buffer[1]);
        length = *((uint32_t *)&Host_Buffer[5]);

        /* Perform memory read operation */
        uint8_t *data = (uint8_t *)address;
        Bootloader_Send_Data_To_Host(data, length);
    }
    else
    {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("CRC Verification Failed\r\n");
#endif
        Bootloader_Send_NACK();
    }
}

static void Bootloader_Read_OTP(uint8_t *Host_Buffer)
{
    uint16_t Host_CMD_Packet_Len = 0;
    uint32_t Host_CRC32 = 0;
    uint32_t address = 0;
    uint32_t length = 0;

#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
    BL_Print_Message("Read OTP Operation\r\n");
#endif

    /* Extract the CRC32 and packet length sent by the HOST */
    Host_CMD_Packet_Len = Host_Buffer[0] + 1;
    Host_CRC32 = *((uint32_t *)((Host_Buffer + Host_CMD_Packet_Len) - CRC_TYPE_SIZE_BYTE));

    /* CRC Verification */
    if (CRC_VERIFICATION_PASSED == Bootloader_CRC_Verify((uint8_t *)&Host_Buffer[0], Host_CMD_Packet_Len - 4, Host_CRC32))
    {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("CRC Verification Passed\r\n");
#endif
        Bootloader_Send_ACK(1);

        /* Extract the address and length from the host buffer */
        address = *((uint32_t *)&Host_Buffer[1]);
        length = *((uint32_t *)&Host_Buffer[5]);

        /* Perform OTP read operation */
        uint8_t *data = (uint8_t *)address;
        Bootloader_Send_Data_To_Host(data, length);
    }
    else
    {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("CRC Verification Failed\r\n");
#endif
        Bootloader_Send_NACK();
    }
}

static uint8_t Change_ROP_Level(uint32_t ROP_Level)
{
    HAL_StatusTypeDef HAL_Status = HAL_ERROR;
    FLASH_OBProgramInitTypeDef FLASH_OBProgramInit;
    uint8_t ROP_Level_Status = ROP_LEVEL_CHANGE_INVALID;

    /* Unlock the FLASH Option Control Registers access */
    HAL_Status = HAL_FLASH_OB_Unlock();
    if(HAL_Status != HAL_OK)
    {
        ROP_Level_Status = ROP_LEVEL_CHANGE_INVALID;
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("Failed -> Unlock the FLASH Option Control Registers access \r\n");
#endif
    }
    else
    {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("Passed -> Unlock the FLASH Option Control Registers access \r\n");
#endif
        FLASH_OBProgramInit.OptionType = OPTIONBYTE_RDP; /* RDP option byte configuration */
        FLASH_OBProgramInit.Banks = FLASH_BANK_1;
        FLASH_OBProgramInit.RDPLevel = ROP_Level;
        /* Program option bytes */
        HAL_Status = HAL_FLASHEx_OBProgram(&FLASH_OBProgramInit);
        if(HAL_Status != HAL_OK)
        {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
            BL_Print_Message("Failed -> Program option bytes \r\n");
#endif
            HAL_Status = HAL_FLASH_OB_Lock();
            ROP_Level_Status = ROP_LEVEL_CHANGE_INVALID;
        }
        else
        {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
            BL_Print_Message("Passed -> Program option bytes \r\n");
#endif
            /* Launch the option byte loading */
            HAL_FLASH_OB_Launch();
            /* Lock the FLASH Option Control Registers access */
            HAL_Status = HAL_FLASH_OB_Lock();
            if(HAL_Status != HAL_OK)
            {
                ROP_Level_Status = ROP_LEVEL_CHANGE_INVALID;
            }
            else
            {
                ROP_Level_Status = ROP_LEVEL_CHANGE_VALID;
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
                BL_Print_Message("Passed -> Program ROP to Level : 0x%X \r\n", ROP_Level);
#endif
            }
        }
    }
    return ROP_Level_Status;
}
static void Bootloader_Change_Read_Protection_Level(uint8_t *Host_Buffer)
{
    uint16_t Host_CMD_Packet_Len = 0;
    uint32_t Host_CRC32 = 0;
    uint8_t ROP_Level_Status = ROP_LEVEL_CHANGE_INVALID;
    uint8_t Host_ROP_Level = 0;

#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
    BL_Print_Message("Change read protection level of the user flash \r\n");
#endif
    /* Extract the CRC32 and packet length sent by the HOST */
    Host_CMD_Packet_Len = Host_Buffer[0] + 1;
    Host_CRC32 = *((uint32_t *)((Host_Buffer + Host_CMD_Packet_Len) - CRC_TYPE_SIZE_BYTE));
    /* CRC Verification */
    if(CRC_VERIFICATION_PASSED == Bootloader_CRC_Verify((uint8_t *)&Host_Buffer[0] , Host_CMD_Packet_Len - 4, Host_CRC32))
    {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("CRC Verification Passed \r\n");
#endif
        Bootloader_Send_ACK(1);
        /* Request change the Read Out Protection Level */
        Host_ROP_Level = Host_Buffer[2];
        if(Host_ROP_Level > CBL_ROP_LEVEL_1)
        {
            ROP_Level_Status = ROP_LEVEL_CHANGE_INVALID;
        }
        else
        {
            if(CBL_ROP_LEVEL_0 == Host_ROP_Level)
            {
                Host_ROP_Level = OB_RDP_LEVEL_0;
            }
            else if(CBL_ROP_LEVEL_1 == Host_ROP_Level)
            {
                Host_ROP_Level = OB_RDP_LEVEL_1;
            }
            ROP_Level_Status = Change_ROP_Level(Host_ROP_Level);
        }
        Bootloader_Send_Data_To_Host((uint8_t *)&ROP_Level_Status, 1);
    }
    else
    {
#if (BL_DEBUG_ENABLE == DEBUG_INFO_ENABLE)
        BL_Print_Message("CRC Verification Failed \r\n");
#endif
        Bootloader_Send_NACK();
    }
}
static void Bootloader_Read_Page_Protection_Status(uint8_t *Host_Buffer)
{
}
static void Bootloader_Enable_RW_Protection(uint8_t *Host_Buffer)
{
}
static uint8_t Bootloader_Write_Flag(uint32_t FlagAddr, uint8_t FlagVal)
{
    HAL_StatusTypeDef HAL_Status = HAL_ERROR;
    uint8_t Address_Verification = ADDRESS_IS_INVALID;
    /* Verify the Extracted address to be valid address */
    Address_Verification = Host_Address_Verification(FlagAddr);
    if(ADDRESS_IS_VALID == Address_Verification)
    {
    	uint8_t Page_Index = (uint8_t)((FlagAddr & STM32F103_PAGE_MASK)/STM32F103_PAGE_SIZE);
    	Perform_Flash_Erase(Page_Index, 1);
    	/* Unlock the FLASH control register access */
    	HAL_Status = HAL_FLASH_Unlock();
    	if(HAL_Status == HAL_OK)
    	{
    		HAL_Status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,FlagAddr,(uint16_t)(FlagVal));
    		/* Lock the FLASH control register access */
    		HAL_FLASH_Lock();
        }
    	else
    	{
    		/* nothing */
    	}
    }
    else
    {
    	HAL_Status = HAL_ERROR;
    }
    Bootloader_Jump_To_User_App();
    return HAL_Status;
}
