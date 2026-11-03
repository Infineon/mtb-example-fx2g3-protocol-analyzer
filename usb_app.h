/***************************************************************************//**
* \file usb_app.h
* \version 1.0
*
* \brief Header file providing definitions for the USB Protocol Analyzer application.
*
*******************************************************************************
* \copyright
* (c) (2026), Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.
*
* SPDX-License-Identifier: Apache-2.0
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#ifndef _CY_USB_APP_H_
#define _CY_USB_APP_H_

#include "cy_debug.h"
#include "cy_usbhs_dw_wrapper.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define RED                            			"\033[0;31m"
#define CYAN                            		"\033[0;36m"
#define COLOR_RESET                     		"\033[0m"

#define LOG_COLOR(...)                  		Cy_Debug_AddToLog(1,CYAN);\
                                        		Cy_Debug_AddToLog(1,__VA_ARGS__); \
                                        		Cy_Debug_AddToLog(1,COLOR_RESET);

#define LOG_ERROR(...)                  		Cy_Debug_AddToLog(1,RED);\
                                        		Cy_Debug_AddToLog(1,__VA_ARGS__); \
                                       		 	Cy_Debug_AddToLog(1,COLOR_RESET);

#define LOG_CLR(CLR, ...)               		Cy_Debug_AddToLog(1,CLR);\
                                        		Cy_Debug_AddToLog(1,__VA_ARGS__); \
                                        		Cy_Debug_AddToLog(1,COLOR_RESET);


#define LOG_TRACE()                     		LOG_COLOR("-->[%s]:%d\r\n",__func__,__LINE__);

#define DELAY_MICRO(us)                 		Cy_SysLib_DelayUs(us)
#define DELAY_MILLI(ms)                 		Cy_SysLib_Delay(ms)


#define SET_BIT(byte, mask)              		(byte) |= (mask)
#define CLR_BIT(byte, mask)              		(byte) &= ~(mask)
#define CHK_BIT(byte, mask)              		(byte) & (mask)

#define DMA_BUFFER_SIZE                  		(PA_RX_MAX_BUFFER_SIZE)
#define FPGA_DMA_BUFFER_SIZE             		(DMA_BUFFER_SIZE)

#define ASSERT(condition, value)        		Cy_CheckStatus(__func__, __LINE__, condition, value, true);
#define ASSERT_NON_BLOCK(condition, value) 		Cy_CheckStatus(__func__, __LINE__, condition, value, false);
#define ASSERT_AND_HANDLE(condition, value, failureHandler) Cy_CheckStatusHandleFailure(__func__, __LINE__, condition, value, false, Cy_FailHandler);

/* Get the LS byte from a 16-bit number */
#define CY_GET_LSB(w)                    		((uint8_t)((w) & UINT8_MAX))

/* Get the MS byte from a 16-bit number */
#define CY_GET_MSB(w)                    		((uint8_t)((w) >> 8))


#define PA_RX_MAX_BUFFER_COUNT           		(4)
#define PA_RX_MAX_BUFFER_SIZE            		(16*1024)

#define BULK_IN_ENDPOINT_1               		(0x02)
#define USB_SNIFFER_CMD_CTRL             		(0xD0)

#define CY_USB_DEVICE_MSG_QUEUE_SIZE            (16)
#define CY_USB_DEVICE_MSG_SIZE                  (sizeof (cy_stc_usbd_app_msg_t))
#define CY_USB_UVC_VBUS_CHANGE_INTR             (0x0E)
#define CY_USB_UVC_VBUS_CHANGE_DEBOUNCED        (0x0F)

#define PHY_TRAINING_PATTERN_BYTE               (0x00)
#define LINK_TRAINING_PATTERN_BYTE              (0x00000000) 

/* P4.0 is used for VBus detect functionality. */
#define VBUS_DETECT_GPIO_PORT                   (P4_0_PORT)
#define VBUS_DETECT_GPIO_PIN                    (P4_0_PIN)
#define VBUS_DETECT_GPIO_INTR                   (ioss_interrupts_gpio_dpslp_4_IRQn)
#define VBUS_DETECT_STATE                       (1u)

#define USB_DESC_ATTRIBUTES __attribute__ ((section(".descSection"), used)) __attribute__ ((aligned (32)))
#define HBDMA_BUF_ATTRIBUTES __attribute__ ((section(".hbBufSection"), used)) __attribute__ ((aligned (32)))

/* Vendor command code used to return WinUSB specific descriptors. */
#define MS_VENDOR_CODE                          (0xF0)

/* SCL low delay */
#define SCLLOW                                  60

#define USB_SNIFFER_I2C_PORT            		P4_3_PORT
#define USB_SNIFFER_I2C_SCL_PIN         		P4_4_PIN
#define USB_SNIFFER_I2C_SDA_PIN         		P4_3_PIN

//THIS PIN IS SAME AS PMODE IN M0+
#define USB_SNIFFER_JTAG_ENABLE_PORT    		P13_0_PORT
#define USB_SNIFFER_JTAG_ENABLE_PIN    		 	P13_0_PIN

#define USB_SNIFFER_JTAG_TMS            		CY_LVDS_PHY_GPIO_DQ0
#define USB_SNIFFER_JTAG_TCK            		CY_LVDS_PHY_GPIO_DQ1
#define USB_SNIFFER_JTAG_TDI            		CY_LVDS_PHY_GPIO_DQ2
#define USB_SNIFFER_JTAG_TDO            		CY_LVDS_PHY_GPIO_DQ3

#define IFCONFIG_IFCFG0                 		(1ul << 0)
#define IFCONFIG_IFCFG1                 		(1ul << 1)
#define IFCONFIG_IFCFG_PORTS            		(0ul << 0)
#define IFCONFIG_IFCFG_GPIF             		(2ul << 0)
#define IFCONFIG_IFCFG_FIFO             		(3ul << 0)
#define IFCONFIG_GSTATE                 		(1ul << 2)
#define IFCONFIG_ASYNC                  		(1ul << 3)
#define IFCONFIG_IFCLKPOL               		(1ul << 4)
#define IFCONFIG_IFCLKOE                		(1ul << 5)
#define IFCONFIG_3048MHZ                		(1ul << 6)
#define IFCONFIG_IFCLKSRC               		(1ul << 7)

typedef enum cy_en_sniffer_cmds_t {
    USB_SNIFFER_START   = 0xF1,
    USB_SNIFFER_PAUSE   = 0xF2,
    USB_SNIFFER_STOP    = 0xF3
}cy_en_sniffer_cmds_t;

typedef enum cy_en_capctrl_cmds_t
{
  CAPTURE_CTRL_RESET  = 0,
  CAPTURE_CTRL_ENABLE = 1,
  CAPTURE_CTRL_SPEED0 = 2,
  CAPTURE_CTRL_SPEED1 = 3,
  CAPTURE_CTRL_TEST   = 4,
}cy_en_capctrl_cmds_t;

typedef enum  cy_en_cmd_i2c_t
{
  CMD_I2C_READ         = 0xb0,
  CMD_I2C_WRITE        = 0xb1,
  CMD_JTAG_ENABLE      = 0xc0,
  CMD_JTAG_REQUEST     = 0xc1,
  CMD_JTAG_RESPONSE    = 0xc2,
  CMD_CTRL             = 0xd0,
}cy_en_cmd_i2c_t;

typedef struct cy_stc_bitfield_t {
    uint32_t b0:1;
    uint32_t b1:1;
    uint32_t b2:1;
    uint32_t b3:1;
    uint32_t b4:1;
    uint32_t b5:1;
    uint32_t b6:1;
    uint32_t b7:1;
}cy_stc_bitfield_t;

typedef union cy_stc_data_t{
    cy_stc_bitfield_t bits;
    uint32_t Value;
} cy_stc_data_t;


extern uint8_t glOsString[];
extern uint8_t glOsCompatibilityId[];
extern uint8_t glOsFeature[];

typedef struct cy_stc_usb_app_ctxt_ cy_stc_usb_app_ctxt_t;

/* FPGA Configuration mode selection*/
typedef enum cy_en_fpgaConfigMode_t
{
    ACTIVE_SERIAL_MODE,
    PASSIVE_SERIAL_MODE
}cy_en_fpgaConfigMode_t;

/* FPGA Stream Control*/
typedef enum cy_en_streamControl_t
{
    STOP,
    START
}cy_en_streamControl_t;

/* 
 * USB application data structure which is bridge between USB system and device
 * functionality.
 * It maintains some usb system information which comes from USBD and it also
 * maintains info about functionality.
 */
struct cy_stc_usb_app_ctxt_
{
    bool hbChannelCreated;    
    bool vbusChangeIntr;                        
    bool vbusPresent;                           
    bool usbConnected;                          
    TimerHandle_t vbusDebounceTimer;            
    uint8_t paInEpNum;
    uint8_t firstInitDone;
    uint8_t devAddr;
    uint8_t activeCfgNum;
    cy_en_usb_device_state_t devState;
    cy_en_usb_device_state_t prevDevState;
    cy_en_usb_speed_t devSpeed;
    cy_en_usb_enum_method_t enumMethod;
    uint8_t prevAltSetting;
    cy_stc_app_endp_dma_set_t endpInDma[CY_USB_MAX_ENDP_NUMBER];
    cy_stc_app_endp_dma_set_t endpOutDma[CY_USB_MAX_ENDP_NUMBER];
    DMAC_Type *pCpuDmacBase;
    DW_Type *pCpuDw0Base;
    DW_Type *pCpuDw1Base;
    cy_stc_hbdma_mgr_context_t *pHbDmaMgrCtxt;
    cy_stc_usb_usbd_ctxt_t *pUsbdCtxt;
    cy_stc_hbdma_channel_t *hbBulkInChannel;
    TaskHandle_t paTaskHandle;
    QueueHandle_t usbMsgQueue;                 

};

/**
 * \name Cy_USB_AppInit
 * \brief   This function Initializes application related data structures, register callback
 *          creates task for device function.
 * \param pAppCtxt application layer context pointer.
 * \param pUsbdCtxt USBD layer Context pointer
 * \param pCpuDmacBase DMAC base address
 * \param pCpuDw0Base DataWire 0 base address
 * \param pCpuDw1Base DataWire 1 base address
 * \param pHbDmaMgrCtxt HBDMA Manager Context
 * \retval None
 */
void Cy_USB_AppInit(cy_stc_usb_app_ctxt_t *pAppCtxt, cy_stc_usb_usbd_ctxt_t *pUsbdCtxt, 
                    DMAC_Type *pCpuDmacBase, DW_Type *pCpuDw0Base, DW_Type *pCpuDw1Base, 
                    cy_stc_hbdma_mgr_context_t *pHbDmaMgrCtxt);

/**
 * \name Cy_USB_AppRegisterCallback
 * \brief This function will register all callbacks with USBD layer.
 * \param pAppCtxt application layer context pointer.
 * \retval None
 */
void Cy_USB_AppRegisterCallback(cy_stc_usb_app_ctxt_t *pAppCtxt);

/**
 * \name Cy_USB_AppSetCfgCallback
 * \brief Callback function will be invoked by USBD when set configuration is received
 * \param pAppCtxt application layer context pointer.
 * \param pUsbdCtxt USBD layer context pointer.
 * \param pMsg USB Message
 * \retval None
 */
void Cy_USB_AppSetCfgCallback(void *pAppCtxt, cy_stc_usb_usbd_ctxt_t *pUsbdCtxt, cy_stc_usb_cal_msg_t *pMsg);

/**
 * \name Cy_USB_AppBusResetCallback
 * \brief Callback function will be invoked by USBD when bus detects RESET
 * \param pAppCtxt application layer context pointer.
 * \param pUsbdCtxt USBD layer context pointer
 * \param pMsg USB Message
 * \retval None
 */
void Cy_USB_AppBusResetCallback(void *pAppCtxt, cy_stc_usb_usbd_ctxt_t *pUsbdCtxt, cy_stc_usb_cal_msg_t *pMsg);

/**
 * \name Cy_USB_AppBusResetDoneCallback
 * \brief Callback function will be invoked by USBD when RESET is completed
 * \param pAppCtxt application layer context pointer.
 * \param pUsbdCtxt USBD layer context pointer
 * \param pMsg USB Message
 * \retval None
 */
void Cy_USB_AppBusResetDoneCallback(void *pAppCtxt, cy_stc_usb_usbd_ctxt_t *pUsbdCtxt, cy_stc_usb_cal_msg_t *pMsg);

/**
 * \name Cy_USB_AppBusSpeedCallback
 * \brief   Callback function will be invoked by USBD when speed is identified or
 *          speed change is detected
 * \param pAppCtxt application layer context pointer.
 * \param pUsbdCtxt USBD context
 * \param pMsg USB Message
 * \retval None
 */
void Cy_USB_AppBusSpeedCallback(void *pAppCtxt, cy_stc_usb_usbd_ctxt_t *pUsbdCtxt, cy_stc_usb_cal_msg_t *pMsg);

/**
 * \name Cy_USB_AppSetupCallback
 * \brief Callback function will be invoked by USBD when SETUP packet is received
 * \param pAppCtxt application layer context pointer.
 * \param pUsbdCtxt USBD context
 * \param pMsg USB Message
 * \retval None
 */
void Cy_USB_AppSetupCallback(void *pAppCtxt, cy_stc_usb_usbd_ctxt_t *pUsbdCtxt, cy_stc_usb_cal_msg_t *pMsg);

/**
 * \name Cy_USB_AppSuspendCallback
 * \brief Callback function will be invoked by USBD when Suspend signal/message is detected
 * \param pAppCtxt application layer context pointer.
 * \param pUsbdCtxt USBD context
 * \param pMsg USB Message
 * \retval None
 */
void Cy_USB_AppSuspendCallback(void *pAppCtxt, cy_stc_usb_usbd_ctxt_t *pUsbdCtxt, cy_stc_usb_cal_msg_t *pMsg);

/**
 * \name Cy_USB_AppResumeCallback
 * \brief Callback function will be invoked by USBD when Resume signal/message is detected
 * \param pAppCtxt application layer context pointer.
 * \param pUsbdCtxt USBD context
 * \param pMsg USB Message
 * \retval None
 */
void Cy_USB_AppResumeCallback (void *pAppCtxt, cy_stc_usb_usbd_ctxt_t *pUsbdCtxt, cy_stc_usb_cal_msg_t *pMsg);

/**
 * \name Cy_USB_AppSetIntfCallback
 * \brief Callback function will be invoked by USBD when SET_INTERFACE is received
 * \param pAppCtxt application layer context pointer.
 * \param pUsbdCtxt USBD context
 * \param pMsg USB Message
 * \retval None
 */
void Cy_USB_AppSetIntfCallback(void *pAppCtxt, cy_stc_usb_usbd_ctxt_t *pUsbdCtxt, cy_stc_usb_cal_msg_t *pMsg);

/**
 * \name Cy_USB_AppInitDmaIntr
 * \brief Function to register an ISR for the DMA channel associated with an endpoint
 * \param endpNumber USB endpoint number
 * \param endpDirection Endpoint direction
 * \param userIsr ISR function pointer. Can be NULL if interrupt is to be disabled.
 * \retval None
 */
void Cy_USB_AppInitDmaIntr(uint32_t endpNumber, cy_en_usb_endp_dir_t endpDirection, cy_israddress userIsr);

/**
 * \name Cy_USB_AppClearDmaInterrupt
 * \brief Clear DMA Interrupt
 * \param pAppCtxt application layer context pointer.
 * \param endpNumber Endpoint number
 * \param endpDirection Endpoint direction
 * \retval None
 */
void Cy_USB_AppClearDmaInterrupt(cy_stc_usb_app_ctxt_t *pAppCtxt, uint32_t endpNumber, cy_en_usb_endp_dir_t endpDirection);

/**
 * \name Cy_PA_RxChannelDataWire_ISR
 * \brief Datawire ISR for Protocol Analyzer RX for channel#1 
 * \retval None
 */
void Cy_PA_RxChannelDataWire_ISR(void);

/**
 * \name Cy_USB_EnableUsbHSConnection
 * \brief Enable USBHS connection
 * \param pAppCtxt Pointer to UVC application context structure.
 * \retval None
 */
bool Cy_USB_EnableUsbHSConnection(cy_stc_usb_app_ctxt_t *pAppCtxt);

/**
 * \name Cy_USB_DisableUsbHSConnection
 * \brief Disable USBHS connection
 * \retval None
 */
void Cy_USB_DisableUsbHSConnection (cy_stc_usb_app_ctxt_t *pAppCtxt);

/**
 * \name Cy_PA_LvdsInit
 * \brief   Initialize the LVDS interface. Currently, only the SIP #0 is being initialized
 *          and configured to allow transfers into the HBW SRAM through DMA.
 * \retval None
 */
void Cy_PA_LvdsInit(void);

/**
 * \name Cy_GPIO_WriteSetClear
 * \brief Set or clear the pin.
 * \param base GPIO Base
 * \param pinNum GPIO Pin
 * \param value value  *
 * \retval void
 */
void Cy_GPIO_WriteSetClear(GPIO_PRT_Type* base, uint32_t pinNum, uint32_t value);

/**
 * \name Cy_JTAGInit
 * \brief Initialize the JTAG interface
 * \retval void
 */
void Cy_JTAGInit(uint16_t wValue);


/**
 * \name Cy_CheckStatus
 * \brief Function that handles prints error log
 * \param function Pointer to function
 * \param line Line number where error is seen
 * \param condition condition of failure
 * \param value error code
 * \param isBlocking blocking function
 * \retval None
 */
void Cy_CheckStatus(const char *function, uint32_t line, uint8_t condition, uint32_t value, uint8_t isBlocking);

/**
 * \name Cy_CheckStatusHandleFailure
 * \brief Function that handles prints error log
 * \param function Pointer to function
 * \param line LineNumber where error is seen
 * \param condition Line number where error is seen
 * \param value error code
 * \param isBlocking blocking function
 * \param failureHandler failure handler function
 * \retval None
 */
void Cy_CheckStatusHandleFailure(const char *function, uint32_t line, uint8_t condition, uint32_t value, uint8_t isBlocking, void (*failureHandler)());

/**
 * \name Cy_FailHandler
 * \brief Error Handler
 * \retval None
 */
void Cy_FailHandler(void);

#if defined(__cplusplus)
}
#endif

#endif /* _CY_USB_APP_H_ */

/* End of File */

