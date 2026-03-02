/***************************************************************************//**
* \file usb_app.c
* \version 1.0
*
* \brief Implements the USB Protocol Analyzer application.
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

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "cy_pdl.h"
#include "cy_device.h"
#include "cy_usbhs_dw_wrapper.h"
#include "cy_usb_common.h"
#include "cy_hbdma.h"
#include "cy_hbdma_mgr.h"
#include "cy_usb_usbd.h"
#include "usb_app.h"
#include "cy_debug.h"
#include "usb_i2c.h"
#include "cy_lvds.h"
#include "usb_i2c.h"

uint8_t __attribute__ ((section(".descSection"), used)) __attribute__ ((aligned (32))) Ep0TestBuffer[4096U];

/**
 * \name Cy_GPIO_WriteSetClear
 * \brief Set or clear the pin.
 * \param base GPIO Base
 * \param pinNum GPIO Pin
 * \param value value  *
 * \retval void
 */
void Cy_GPIO_WriteSetClear(GPIO_PRT_Type* base, uint32_t pinNum, uint32_t value) {
    if (value == 1)
        GPIO_PRT_OUT_SET(base) = (CY_GPIO_OUT_MASK<<pinNum);
    else
        GPIO_PRT_OUT_CLR(base) = (CY_GPIO_OUT_MASK<<pinNum);
}
/* SCL low delay */
#define SCLLOW  60
/**
 * \name Cy_I2CWriteBit
 * \brief Write a bit value on the i2c bus
 * \param bit
 * \retval void
 */
void Cy_I2CWriteBit(uint32_t bit)
{
    Cy_GPIO_WriteSetClear(USB_SNIFFER_I2C_PORT, USB_SNIFFER_I2C_SCL_PIN, 0);
    Cy_GPIO_WriteSetClear(USB_SNIFFER_I2C_PORT, USB_SNIFFER_I2C_SDA_PIN, bit);
    Cy_SysLib_DelayCycles(SCLLOW);
    Cy_GPIO_WriteSetClear(USB_SNIFFER_I2C_PORT, USB_SNIFFER_I2C_SCL_PIN, 1);
}

/**
 * \name Cy_usb_sniffer_i2c_send
 * \brief Write the 5bit value i2c data.
 * \param wValue value
 * \retval success or failure
 */
cy_en_scb_i2c_status_t Cy_usb_sniffer_i2c_send(uint8_t wValue)
{

    wValueData_t I2CData;

    I2CData.Value = wValue;
    cy_en_scb_i2c_status_t status = CY_SCB_I2C_SUCCESS;

    //Start condition: clk high, dat must go low
    Cy_GPIO_WriteSetClear(USB_SNIFFER_I2C_PORT, USB_SNIFFER_I2C_SDA_PIN, 0);
    
    Cy_I2CWriteBit(I2CData.bits.b0);
    Cy_I2CWriteBit(I2CData.bits.b1);
    Cy_I2CWriteBit(I2CData.bits.b2);
    Cy_I2CWriteBit(I2CData.bits.b3);
    Cy_I2CWriteBit(I2CData.bits.b4);

    /* Stop state */
    Cy_GPIO_WriteSetClear(USB_SNIFFER_I2C_PORT, USB_SNIFFER_I2C_SDA_PIN, 0x00);
    Cy_GPIO_WriteSetClear(USB_SNIFFER_I2C_PORT, USB_SNIFFER_I2C_SDA_PIN, 0x01);


    return status;
}

/**
 * \name Cy_PA_AppStop
 * \brief Stop the data stream channels
 * \param pAppCtxt application layer context pointer
 * \param pUsbdCtxt USBD layer context pointer
 * \param IsStreamStart Pass 1 to start streaming from fpga elase 0
 * \retval void
 */
static void Cy_PA_AppStop(cy_stc_usb_app_ctxt_t *pAppCtxt, cy_stc_usb_usbd_ctxt_t *pUsbdCtxt, uint32_t epNumber)
{
    cy_en_hbdma_mgr_status_t status = CY_HBDMA_MGR_SUCCESS;
    uint8_t index = 0;
    cy_stc_hbdma_sock_t sckStat;

    DBG_APP_INFO("App Stop:epNumber=0x%x\r\n",epNumber);

    if (BULK_IN_ENDPOINT_1 == epNumber)
    {
        for(index = 0; index < pAppCtxt->hbBulkInChannel[0]->prodSckCount; index++)
        {
            do {
                Cy_HBDma_GetSocketStatus(pAppCtxt->pHbDmaMgrCtxt->pDrvContext, pAppCtxt->hbBulkInChannel[0]->prodSckId[index], &sckStat);
            } while(((_FLD2VAL(LVDSSS_LVDS_ADAPTER_DMA_SCK_STATUS_STATE,sckStat.status)) != 0x1));

            DBG_APP_INFO("DMA Socket %x is stalled\r\n", pAppCtxt->hbBulkInChannel[0]->prodSckId[index]);
        }
        /* Reset the DMA channel through which data is received from the LVDS side. */
        status = Cy_HBDma_Channel_Reset(pAppCtxt->hbBulkInChannel[0]);
        ASSERT_NON_BLOCK(CY_HBDMA_MGR_SUCCESS == status,status);
        pAppCtxt->paPendingRxBufCnt1 = 0;
        DBG_APP_INFO("EP: %d DMA Reset and App Stopped\n",epNumber);

    }

    /* On USB 2.0 connection, reset the DataWire channel used to send data to the EPM. */
    Cy_USBHS_App_ResetEpDma(&(pAppCtxt->endpInDma[epNumber]));

    /* Flush and reset the endpoint and clear the STALL bit. */
    Cy_USBD_FlushEndp(pUsbdCtxt, epNumber, CY_USB_ENDP_DIR_IN);
    Cy_USBD_ResetEndp(pUsbdCtxt, epNumber, CY_USB_ENDP_DIR_IN, false);
    Cy_USB_USBD_EndpSetClearStall(pUsbdCtxt, (cy_en_usb_endp_dir_t)epNumber, CY_USB_ENDP_DIR_IN, false);

    DBG_APP_INFO("Cy_HBDma_Channel_Enable = %d\n",
            Cy_HBDma_Channel_Enable(pAppCtxt->hbBulkInChannel[0], 0));
}

/**
 * \name Cy_PA_AppHandleRxEvent
 * \brief Function to handle data received on Protocol Analyzer interface
 * \param pUsbApp application layer context pointer
 * \param pChHandle HBDMA channel handler
 * \param incFlag flag to queue data on USB
 * \param index Protocol Analyzer channel index
 * \retval void
 */
static void Cy_PA_AppHandleRxEvent (cy_stc_usb_app_ctxt_t  *pUsbApp, cy_stc_hbdma_channel_t *pChHandle, bool incFlag, uint8_t index)
{
    cy_en_hbdma_mgr_status_t   status;
    cy_stc_hbdma_buff_status_t buffStat;
    uint32_t intMask;

    if(index == 1)
    {
        /* Queue USBHS IN transfer */
        if(incFlag == 1)
        {
            intMask = Cy_SysLib_EnterCriticalSection();
            pUsbApp->paPendingRxBufCnt1++;
            if(pUsbApp->paPendingRxBufCnt1 > 1)
            {
                Cy_SysLib_ExitCriticalSection(intMask);
                return;
            }
            Cy_SysLib_ExitCriticalSection(intMask);
        }

          /* Wait for a free buffer. */
          status = Cy_HBDma_Channel_GetBuffer(pChHandle, &buffStat);
          if (status != CY_HBDMA_MGR_SUCCESS)
          {
              DBG_APP_ERR("SOC_ERROR\r\n");
              return;
          }

          Cy_USB_AppQueueWrite(pUsbApp, pUsbApp->paInEpNum1, buffStat.pBuffer, buffStat.count);
    }
}

/**
 * \name Cy_PA_AppHbDmaRxCallback
 * \brief Callback function for the channel receiving data into LVDS socket
 * \param handle HBDMA channel handle
 * \param cy_en_hbdma_cb_type_t HBDMA channel type
 * \param pbufStat HBDMA buffer status
 * \param userCtx user context
 * \retval void
 */
void Cy_PA_AppHbDmaRxCallback(
        cy_stc_hbdma_channel_t *handle,
        cy_en_hbdma_cb_type_t type,
        cy_stc_hbdma_buff_status_t *pbufStat,
        void *userCtx)
{
    cy_stc_usb_app_ctxt_t *pAppCtxt = (cy_stc_usb_app_ctxt_t *)userCtx;
    uint8_t index = 0;

    if(handle == pAppCtxt->hbBulkInChannel[0])
    {
        index = 1;
    }

    if (type == CY_HBDMA_CB_PROD_EVENT)
    {
        Cy_PA_AppHandleRxEvent(pAppCtxt, handle,1,index);
    }
}

/**
 * \name Cy_PA_AppTaskHandler
 * \brief Protocol Analyzer Application task handler
 * \param pTaskParam task param
 * \retval void
 */
void Cy_PA_AppTaskHandler(void *pTaskParam)
{
    cy_stc_usb_app_ctxt_t *pAppCtxt = (cy_stc_usb_app_ctxt_t *)pTaskParam;
    cy_stc_usbd_app_msg_t queueMsg;
    BaseType_t xStatus;
    uint32_t lpEntryTime = 0;
    uint8_t i ;
    uint32_t loopCnt = 0, printCnt = 0;
    static bool Powerup = true;

    vTaskDelay(250);

    /* Initialize the LVDS interface. */
    Cy_PA_LvdsInit(); 
    vTaskDelay(100);

    /* If VBus is present, enable the USB connection. */
    pAppCtxt->vbusPresent =
    (Cy_GPIO_Read(VBUS_DETECT_GPIO_PORT, VBUS_DETECT_GPIO_PIN) == VBUS_DETECT_STATE);
#if USBFS_LOGS_ENABLE
    vTaskDelay(500);
#endif /* USBFS_LOGS_ENABLE */
    if (pAppCtxt->vbusPresent) {
        Cy_USB_EnableUsbHSConnection(pAppCtxt);
    }

    DBG_APP_INFO("AppThreadCreated\r\n");

    for (;;)
    {

        /*
         * If the link has been in USB2-L1 for more than 0.5 seconds, initiate LPM exit so that
         * transfers do not get delayed significantly.
         */
        if ((MXS40USBHSDEV_USBHSDEV->DEV_PWR_CS & USBHSDEV_DEV_PWR_CS_L1_SLEEP) != 0)
        {
            if ((Cy_USBD_GetTimerTick() - lpEntryTime) >= 500UL) {
                lpEntryTime = Cy_USBD_GetTimerTick();
                Cy_USBD_GetUSBLinkActive(pAppCtxt->pUsbdCtxt);
            }
        } else {
            lpEntryTime = Cy_USBD_GetTimerTick();
        }
        loopCnt++;
        if (loopCnt > 50) {
            DBG_APP_INFO("TASKLOOP: %d\r\n", printCnt++);
            DBG_APP_INFO("Cy_LVDS_GpifGetSMState = %x\r\n", Cy_LVDS_GpifGetSMState(LVDSSS_LVDS, 0));
            loopCnt = 0;
        }
        /*
         * Wait until some data is received from the queue.
         * Timeout after 100 ms.
         */
        xStatus = xQueueReceive(pAppCtxt->usbMsgQueue, &queueMsg, 100);
        if (xStatus != pdPASS) {
            continue;
        }

        switch (queueMsg.type) {

            case CY_USB_UVC_VBUS_CHANGE_INTR:
                /* Start the debounce timer. */
                xTimerStart(pAppCtxt->vbusDebounceTimer, 0);
                break;

            case CY_USB_UVC_VBUS_CHANGE_DEBOUNCED:
                /* Check whether VBus state has changed. */
                pAppCtxt->vbusPresent = (Cy_GPIO_Read(VBUS_DETECT_GPIO_PORT, VBUS_DETECT_GPIO_PIN) == VBUS_DETECT_STATE);

                if (pAppCtxt->vbusPresent) {
                    if (!pAppCtxt->usbConnected) {
                        DBG_APP_INFO("Enabling USB connection due to VBus detect\r\n");
                        Cy_USB_EnableUsbHSConnection(pAppCtxt);
                    }
                } else {
                    if (pAppCtxt->usbConnected) {
                        /* On USB 2.x connections, make sure the DataWire channels are disabled and reset. */
 
                        for (i = 1; i < CY_USB_MAX_ENDP_NUMBER; i++) {
                            if (pAppCtxt->endpInDma[i].valid) {
                                /* DeInit the DMA channel and disconnect the triggers. */
                                Cy_USBHS_App_DisableEpDmaSet(&(pAppCtxt->endpInDma[i]));
                            }

                            if (pAppCtxt->endpOutDma[i].valid) {
                                /* DeInit the DMA channel and disconnect the triggers. */
                                Cy_USBHS_App_DisableEpDmaSet(&(pAppCtxt->endpOutDma[i]));
                            }
                        }

                        if (pAppCtxt->hbChannelCreated)
                        {
                            DBG_APP_TRACE("HBDMA destroy\r\n");
                            pAppCtxt->hbChannelCreated = false;
                            Cy_HBDma_Channel_Disable(pAppCtxt->hbBulkInChannel[0]);
                            Cy_HBDma_Channel_Destroy(pAppCtxt->hbBulkInChannel[0]);
                        }
                    }
                    DBG_APP_INFO("Disabling USB connection due to VBus removal\r\n");
                    Cy_USB_DisableUsbHSConnection(pAppCtxt);
                }
                
                break;

            case CAPTURE_CTRL_TEST:
                /* Reset DMA before start of capture */
                if (((uint8_t)(queueMsg.data[0]) == BULK_IN_ENDPOINT_1) && (!Powerup))
                {
                    Cy_PA_AppStop(pAppCtxt, pAppCtxt->pUsbdCtxt, BULK_IN_ENDPOINT_1);
                }
                if (Powerup == true) {
                    Powerup = false;
                }
                /* no break */
            case CAPTURE_CTRL_RESET:
            case CAPTURE_CTRL_ENABLE:
            case CAPTURE_CTRL_SPEED0:
            case CAPTURE_CTRL_SPEED1:
                DBG_APP_INFO("CY USB SNIFFER for EP %x wValue 0x%x\r\n", (uint8_t)queueMsg.data[0],(uint8_t)queueMsg.data[1]);
                if((uint8_t)(queueMsg.data[0]) == BULK_IN_ENDPOINT_1)
                {
                    Cy_usb_sniffer_i2c_send(queueMsg.data[1]);
                }
                break;  
            case CMD_JTAG_REQUEST:
                if((uint8_t)(queueMsg.data[0]) == BULK_IN_ENDPOINT_1) {
                    uint16_t loopCnt = 250u;
                    cy_en_usbd_ret_code_t retStatus;
                    uint16_t length = queueMsg.data[1] & 0xFF;
                    uint16_t value = queueMsg.data[1] >> 16;
                    /* Read the data out */
                    retStatus = Cy_USB_USBD_RecvEndp0Data(pAppCtxt->pUsbdCtxt, (uint8_t *)Ep0TestBuffer, length);
                    if (retStatus == CY_USBD_STATUS_SUCCESS)
                    {
                        /* Wait until receive DMA transfer has been completed. */
                        while ((!Cy_USBD_IsEp0ReceiveDone(pAppCtxt->pUsbdCtxt)) && (loopCnt--)) {
                            Cy_SysLib_DelayUs(10);
                        }
                        if (!Cy_USBD_IsEp0ReceiveDone(pAppCtxt->pUsbdCtxt)) {
                            Cy_USB_USBD_RetireRecvEndp0Data(pAppCtxt->pUsbdCtxt);
                            Cy_USB_USBD_EndpSetClearStall(pAppCtxt->pUsbdCtxt, 0x00, CY_USB_ENDP_DIR_IN, TRUE);
                            return;
                        }
                        Cy_JTAGTransfer(value);
                    }
                }

                break;

            default:
            break;
        }

       
    } /* End of for(;;) */
}

/**
 * \name Cy_USB_VbusDebounceTimerCallback
 * \brief Timer used to do debounce on VBus changed interrupt notification.
 * \param xTimer Timer Handle
 * \retval None
 */
void
Cy_USB_VbusDebounceTimerCallback (TimerHandle_t xTimer)
{
    cy_stc_usb_app_ctxt_t *pAppCtxt = (cy_stc_usb_app_ctxt_t *)pvTimerGetTimerID(xTimer);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    cy_stc_usbd_app_msg_t xMsg;

    DBG_APP_INFO("VbusDebounce_CB\r\n");
    if (pAppCtxt->vbusChangeIntr) {
        /* Notify the VCOM task that VBus debounce is complete. */
        xMsg.type = CY_USB_UVC_VBUS_CHANGE_DEBOUNCED;
        xQueueSendFromISR(pAppCtxt->usbMsgQueue, &(xMsg), &(xHigherPriorityTaskWoken));

        /* Clear and re-enable the interrupt. */
        pAppCtxt->vbusChangeIntr = false;
        Cy_GPIO_ClearInterrupt(VBUS_DETECT_GPIO_PORT, VBUS_DETECT_GPIO_PIN);
        Cy_GPIO_SetInterruptMask(VBUS_DETECT_GPIO_PORT, VBUS_DETECT_GPIO_PIN, 1);
    }
}   /* end of function  */

/**
 * \name Cy_USB_AppInit
 * \brief This function Initializes application related data structures, register callback creates task for device function.
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
                    cy_stc_hbdma_mgr_context_t *pHbDmaMgrCtxt)
{
    uint32_t index;
    BaseType_t status = pdFALSE;
    cy_stc_app_endp_dma_set_t *pEndpInDma;
    cy_stc_app_endp_dma_set_t *pEndpOutDma;

    pAppCtxt->devState = CY_USB_DEVICE_STATE_DISABLE;
    pAppCtxt->prevDevState = CY_USB_DEVICE_STATE_DISABLE;
    pAppCtxt->devSpeed = CY_USBD_USB_DEV_FS;
    pAppCtxt->devAddr = 0x00;
    pAppCtxt->activeCfgNum = 0x00;
    pAppCtxt->prevAltSetting = 0x00;
    pAppCtxt->enumMethod = CY_USB_ENUM_METHOD_FAST;
    pAppCtxt->pHbDmaMgrCtxt = pHbDmaMgrCtxt;
    pAppCtxt->pCpuDmacBase = pCpuDmacBase;
    pAppCtxt->pCpuDw0Base = pCpuDw0Base;
    pAppCtxt->pCpuDw1Base = pCpuDw1Base;
    pAppCtxt->pUsbdCtxt = pUsbdCtxt;
    pAppCtxt->hbChannelCreated = false;
    pAppCtxt->paPendingRxBufCnt1 = 0;

    for (index = 0x00; index < CY_USB_MAX_ENDP_NUMBER; index++)
    {
        pEndpInDma = &(pAppCtxt->endpInDma[index]);
        memset((void *)pEndpInDma, 0, sizeof(cy_stc_app_endp_dma_set_t));

        pEndpOutDma = &(pAppCtxt->endpOutDma[index]);
        memset((void *)pEndpOutDma, 0, sizeof(cy_stc_app_endp_dma_set_t));
    }

    /*
     * Callbacks registered with USBD layer. These callbacks will be called
     * based on appropriate event.
     */
    Cy_USB_AppRegisterCallback(pAppCtxt);

    if (!(pAppCtxt->firstInitDone))
    {

        /* Create the message queue and register it with the kernel. */
        pAppCtxt->usbMsgQueue = xQueueCreate(CY_USB_DEVICE_MSG_QUEUE_SIZE,
                CY_USB_DEVICE_MSG_SIZE);
        if (pAppCtxt->usbMsgQueue == NULL) {
            DBG_APP_ERR("QueuecreateFail\r\n");
            return;
        }

        vQueueAddToRegistry(pAppCtxt->usbMsgQueue, "DeviceMsgQueue");
        /* Create task and check status to confirm task created properly. */
        status = xTaskCreate(Cy_PA_AppTaskHandler, "ProtocolAnalyzerDeviceTask", 2048,
                             (void *)pAppCtxt, 5, &(pAppCtxt->paTaskHandle));

        if (status != pdPASS)
        {
            DBG_APP_ERR("TaskcreateFail\r\n");
            return;
        }

        pAppCtxt->vbusDebounceTimer = xTimerCreate("VbusDebounceTimer", 200, pdFALSE,
                (void *)pAppCtxt, Cy_USB_VbusDebounceTimerCallback);
        if (pAppCtxt->vbusDebounceTimer == NULL) {
            DBG_APP_ERR("TimerCreateFail\r\n");
            return;
        }
        DBG_APP_INFO("VBus debounce timer created\r\n");
        pAppCtxt->firstInitDone = 0x01;
    }
} /* end of function. */

/**
 * \name Cy_USB_AppRegisterCallback
 * \brief This function will register all calback with USBD layer.
 * \param pAppCtxt application layer context pointer.
 * \retval None
 */
void Cy_USB_AppRegisterCallback(cy_stc_usb_app_ctxt_t *pAppCtxt)
{
    cy_stc_usb_usbd_ctxt_t *pUsbdCtxt = pAppCtxt->pUsbdCtxt;

    Cy_USBD_RegisterCallback(pUsbdCtxt, CY_USB_USBD_CB_RESET, Cy_USB_AppBusResetCallback);
    
    Cy_USBD_RegisterCallback(pUsbdCtxt, CY_USB_USBD_CB_RESET_DONE, Cy_USB_AppBusResetDoneCallback);
    
    Cy_USBD_RegisterCallback(pUsbdCtxt, CY_USB_USBD_CB_BUS_SPEED, Cy_USB_AppBusSpeedCallback);
    
    Cy_USBD_RegisterCallback(pUsbdCtxt, CY_USB_USBD_CB_SETUP, Cy_USB_AppSetupCallback);
    
    Cy_USBD_RegisterCallback(pUsbdCtxt, CY_USB_USBD_CB_SUSPEND, Cy_USB_AppSuspendCallback);
    
    Cy_USBD_RegisterCallback(pUsbdCtxt, CY_USB_USBD_CB_RESUME, Cy_USB_AppResumeCallback);
    
    Cy_USBD_RegisterCallback(pUsbdCtxt, CY_USB_USBD_CB_SET_CONFIG, Cy_USB_AppSetCfgCallback);
    
    Cy_USBD_RegisterCallback(pUsbdCtxt, CY_USB_USBD_CB_SET_INTF, Cy_USB_AppSetIntfCallback);
    
}

/**
 * \name Cy_USB_AppSetupEndpDmaParamsHs
 * \brief Configure and enable HBW DMA channels.
 * \param pAppCtxt application layer context pointer.
 * \param pEndpDscr Endpoint descriptor pointer
 * \retval None
 */
static void Cy_USB_AppSetupEndpDmaParamsHs(cy_stc_usb_app_ctxt_t *pUsbApp, uint8_t *pEndpDscr)
{
    cy_stc_hbdma_chn_config_t dmaConfig;
    cy_en_hbdma_mgr_status_t mgrStat;
    uint32_t endpNumber, dir;
    uint16_t maxPktSize;
    cy_stc_app_endp_dma_set_t *pEndpDmaSet;
    DW_Type *pDW;
    bool stat;

    Cy_USBD_GetEndpNumMaxPktDir(pEndpDscr, &endpNumber, &maxPktSize, &dir);
    
    if(endpNumber == BULK_IN_ENDPOINT_1)
    {
        pUsbApp->paInEpNum1    = (uint8_t)endpNumber;
        pUsbApp->paPendingRxBufCnt1 = 0;
        dmaConfig.size          = PA_RX_MAX_BUFFER_SIZE;      /* DMA Buffer Size in bytes */
        dmaConfig.count         = PA_RX_MAX_BUFFER_COUNT;     /* DMA Buffer Count */
        dmaConfig.bufferMode    = true;                         /* DMA buffer mode disabled */
        dmaConfig.prodHdrSize   = 0;
        dmaConfig.prodBufSize   = PA_RX_MAX_BUFFER_SIZE;
        dmaConfig.eventEnable   = 0;                            /* Enable for DMA AUTO */
        dmaConfig.intrEnable    = LVDSSS_LVDS_ADAPTER_DMA_SCK_INTR_PRODUCE_EVENT_Msk | 
                                    LVDSSS_LVDS_ADAPTER_DMA_SCK_INTR_CONSUME_EVENT_Msk;

        dmaConfig.consSckCount  = 1;                            /* No. of consumer Sockets */
        dmaConfig.consSck[1]    = (cy_hbdma_socket_id_t)0;      /* Consumer Socket ID: None */
        dmaConfig.cb            = Cy_PA_AppHbDmaRxCallback;                     /* HB-DMA callback */
        dmaConfig.userCtx       = (void *)(pUsbApp);            /* Pass the application context as user context. */
        /* Single producer */
        dmaConfig.prodSckCount = 1; /* No. of producer sockets */
        dmaConfig.prodSck[0] = CY_HBDMA_LVDS_SOCKET_00;
        dmaConfig.prodSck[1] = (cy_hbdma_socket_id_t)0; /* Producer Socket ID: None */

        dmaConfig.chType     = CY_HBDMA_TYPE_IP_TO_MEM;
        dmaConfig.consSck[0] = (cy_hbdma_socket_id_t)0;

        mgrStat = Cy_HBDma_Channel_Create(pUsbApp->pUsbdCtxt->pHBDmaMgr, 
                                          &(pUsbApp->endpInDma[endpNumber].hbDmaChannel),
                                          &dmaConfig);

        if (mgrStat != CY_HBDMA_MGR_SUCCESS)
        {
            DBG_APP_ERR("BulkIn channel create failed 0x%x\r\n", mgrStat);
            return;
        }
        else
        {
            /* Store the DMA channel pointer. */
            pUsbApp->hbBulkInChannel[0] = &(pUsbApp->endpInDma[endpNumber].hbDmaChannel);
            mgrStat = Cy_HBDma_Channel_Enable(pUsbApp->hbBulkInChannel[0], 0);
            DBG_APP_INFO("InChnEnable status: %x\r\n", mgrStat);
        }
    }

    /* Set flag to indicate DMA channels have been created. */
    pUsbApp->hbChannelCreated = true;

    if (*(pEndpDscr + CY_USB_ENDP_DSCR_OFFSET_ADDRESS) & 0x80)
    {
        dir = CY_USB_ENDP_DIR_IN;
        pEndpDmaSet = &(pUsbApp->endpInDma[endpNumber]);
        pDW = pUsbApp->pCpuDw1Base;
    }
    else
    {
        dir = CY_USB_ENDP_DIR_OUT;
        pEndpDmaSet = &(pUsbApp->endpOutDma[endpNumber]);
        pDW = pUsbApp->pCpuDw0Base;
    }

    stat = Cy_USBHS_App_EnableEpDmaSet(pEndpDmaSet, pDW, endpNumber, endpNumber,(cy_en_usb_endp_dir_t)dir, maxPktSize);
    DBG_APP_INFO("Enable EPDmaSet: endp=%x dir=%x stat=%x\r\n", endpNumber, dir, stat);

    return;
} /* end of function  */


/**
 * \name Cy_USB_AppConfigureEndp
 * \brief Configure all endpoints used by application (except EP0)
 * \param pUsbdCtxt USBD layer context pointer
 * \param pEndpDscr Endpoint descriptor pointer
 * \retval None
 */
void Cy_USB_AppConfigureEndp(cy_stc_usb_usbd_ctxt_t *pUsbdCtxt, uint8_t *pEndpDscr)
{
    cy_stc_usb_endp_config_t endpConfig;
    cy_en_usb_endp_dir_t endpDirection;
    bool valid;
    uint32_t endpType;
    uint32_t endpNumber, dir;
    uint16_t maxPktSize;
    uint32_t isoPkts = 0x00;
    uint8_t burstSize = 0x00;
    uint8_t maxStream = 0x00;
    uint8_t interval = 0x00;
    cy_en_usbd_ret_code_t usbdRetCode;

    /* If it is not endpoint descriptor then return */
    if (!Cy_USBD_EndpDscrValid(pEndpDscr))
    {
        return;
    }
    Cy_USBD_GetEndpNumMaxPktDir(pEndpDscr, &endpNumber, &maxPktSize, &dir);

    if (dir)
    {
        endpDirection = CY_USB_ENDP_DIR_IN;
    }
    else
    {
        endpDirection = CY_USB_ENDP_DIR_OUT;
    }
    Cy_USBD_GetEndpType(pEndpDscr, &endpType);

    if ((CY_USB_ENDP_TYPE_ISO == endpType) || (CY_USB_ENDP_TYPE_INTR == endpType))
    {
        /* The ISOINPKS setting in the USBHS register is the actual packets per microframe value. */
        isoPkts = ((*((uint8_t *)(pEndpDscr + CY_USB_ENDP_DSCR_OFFSET_MAX_PKT + 1)) & CY_USB_ENDP_ADDL_XN_MASK) >> CY_USB_ENDP_ADDL_XN_POS) + 1;
    }

    valid = 0x01;
    Cy_USBD_GetEndpInterval(pEndpDscr, &interval);

    /* Prepare endpointConfig parameter. */
    endpConfig.endpType = (cy_en_usb_endp_type_t)endpType;
    endpConfig.endpDirection = endpDirection;
    endpConfig.valid = valid;
    endpConfig.endpNumber = endpNumber;
    endpConfig.maxPktSize = (uint32_t)maxPktSize;
    endpConfig.isoPkts = isoPkts;
    endpConfig.burstSize = burstSize;
    endpConfig.streamID = maxStream;
    endpConfig.interval = interval;
    /*
     * allowNakTillDmaRdy = true means device will send NAK
     * till DMA setup is ready. This field is applicable to only
     * ingress direction ie OUT transfer/OUT endpoint.
     * For Egress ie IN transfer, this field is ignored.
     */
    endpConfig.allowNakTillDmaRdy = TRUE;
    usbdRetCode = Cy_USB_USBD_EndpConfig(pUsbdCtxt, endpConfig);

    /* Print status of the endpoint configuration to help debug. */
    DBG_APP_INFO("#ENDPCFG: %d, %d\r\n", endpNumber, usbdRetCode);
    return;
} /* end of function */

/**
 * \name Cy_USB_AppSetCfgCallback
 * \brief Callback function will be invoked by USBD when set configuration is received
 * \param pAppCtxt application layer context pointer.
 * \param pUsbdCtxt USBD layer context pointer.
 * \param pMsg USB Message
 * \retval None
 */
void Cy_USB_AppSetCfgCallback(void *pAppCtxt, cy_stc_usb_usbd_ctxt_t *pUsbdCtxt,
                              cy_stc_usb_cal_msg_t *pMsg)
{
    cy_stc_usb_app_ctxt_t *pUsbApp;
    uint8_t *pActiveCfg, *pIntfDscr, *pEndpDscr;
    uint8_t index, numOfIntf, numOfEndp;
    DBG_APP_INFO("AppSetCfgCbStart\r\n");

    pUsbApp = (cy_stc_usb_app_ctxt_t *)pAppCtxt;
    pUsbApp->devSpeed = Cy_USBD_GetDeviceSpeed(pUsbdCtxt);
    
    /*
     * Based on type of application as well as how data flows,
     * data wire can be used so initialize datawire.
     */
    Cy_DMA_Enable(pUsbApp->pCpuDw0Base);
    Cy_DMA_Enable(pUsbApp->pCpuDw1Base);

    pActiveCfg = Cy_USB_USBD_GetActiveCfgDscr(pUsbdCtxt);
    if (!pActiveCfg)
    {
        /* Set config should be called when active config value > 0x00. */
        return;
    }
    numOfIntf = Cy_USBD_FindNumOfIntf(pActiveCfg);
    if (numOfIntf == 0x00)
    {
        return;
    }

    for (index = 0x00; index < numOfIntf; index++)
    {
        /* During Set Config command always altSetting 0 will be active. */
        pIntfDscr = Cy_USBD_GetIntfDscr(pUsbdCtxt, index, 0x00);
        if (pIntfDscr == NULL)
        {
            DBG_APP_INFO("pIntfDscrNull\r\n");
            return;
        }

        numOfEndp = Cy_USBD_FindNumOfEndp(pIntfDscr);
        if (numOfEndp == 0x00)
        {
            DBG_APP_INFO("numOfEndp 0\r\n");
            continue;
        }

        pEndpDscr = Cy_USBD_GetEndpDscr(pUsbdCtxt, pIntfDscr);
        while (numOfEndp != 0x00)
        {
            Cy_USB_AppConfigureEndp(pUsbdCtxt, pEndpDscr);
            Cy_USB_AppSetupEndpDmaParamsHs(pAppCtxt, pEndpDscr);
            numOfEndp--;
            pEndpDscr = (pEndpDscr + (*(pEndpDscr + CY_USB_DSCR_OFFSET_LEN)));
            
        }
    }

#if CY_CPU_CORTEX_M4
    Cy_USB_AppInitDmaIntr(BULK_IN_ENDPOINT_1, CY_USB_ENDP_DIR_IN, Cy_PA_RxChannel1DataWire_ISR);
#else
    Cy_USB_AppInitDmaIntr(BULK_IN_ENDPOINT_1, CY_USB_ENDP_DIR_IN, Cy_PA_RxDataWireCombined_ISR);
#endif

    pUsbApp->prevDevState = CY_USB_DEVICE_STATE_CONFIGURED;
    pUsbApp->devState = CY_USB_DEVICE_STATE_CONFIGURED;

    Cy_USBD_LpmDisable(pUsbdCtxt);

    Cy_LVDS_GpifThreadConfig(LVDSSS_LVDS, 0, 0, 0, 1, 0);
    Cy_LVDS_GpifThreadConfig(LVDSSS_LVDS, 1, 1, 0, 1, 0);

    DBG_APP_INFO("AppSetCfgCbEnd Done\r\n");
    return;
} /* end of function */

/**
 * \name Cy_USB_AppBusResetCallback
 * \brief Callback function will be invoked by USBD when bus detects RESET
 * \param pAppCtxt application layer context pointer.
 * \param pUsbdCtxt USBD layer context pointer
 * \param pMsg USB Message
 * \retval None
 */
void Cy_USB_AppBusResetCallback(void *pAppCtxt, cy_stc_usb_usbd_ctxt_t *pUsbdCtxt,
                                cy_stc_usb_cal_msg_t *pMsg)
{
    cy_stc_usb_app_ctxt_t *pUsbApp;

    pUsbApp = (cy_stc_usb_app_ctxt_t *)pAppCtxt;

    DBG_APP_INFO("AppBusResetCallback\r\n");

    /* Stop and destroy the high bandwidth DMA channel if present. To be done before AppInit is called. */
    if (pUsbApp->hbChannelCreated)
    {
        DBG_APP_TRACE("HBDMA destroy\r\n");
        pUsbApp->hbChannelCreated = false;
        Cy_HBDma_Channel_Disable(pUsbApp->hbBulkInChannel[0]);
        Cy_HBDma_Channel_Destroy(pUsbApp->hbBulkInChannel[0]);
    }

    /*
     * USBD layer takes care of reseting its own data structure as well as
     * takes care of calling CAL reset APIs. Application needs to take care
     * of reseting its own data structure as well as "device function".
     */
    Cy_USB_AppInit(pUsbApp, pUsbdCtxt, pUsbApp->pCpuDmacBase, pUsbApp->pCpuDw0Base, pUsbApp->pCpuDw1Base, pUsbApp->pHbDmaMgrCtxt);
    pUsbApp->devState = CY_USB_DEVICE_STATE_RESET;
    pUsbApp->prevDevState = CY_USB_DEVICE_STATE_RESET;

    return;
} /* end of function. */

/**
 * \name Cy_USB_AppBusResetDoneCallback
 * \brief Callback function will be invoked by USBD when RESET is completed
 * \param pAppCtxt application layer context pointer.
 * \param pUsbdCtxt USBD layer context pointer
 * \param pMsg USB Message
 * \retval None
 */
void Cy_USB_AppBusResetDoneCallback(void *pAppCtxt,
                                    cy_stc_usb_usbd_ctxt_t *pUsbdCtxt,
                                    cy_stc_usb_cal_msg_t *pMsg)
{
    cy_stc_usb_app_ctxt_t *pUsbApp;

    DBG_APP_INFO("ppBusResetDoneCallback\r\n");

    pUsbApp = (cy_stc_usb_app_ctxt_t *)pAppCtxt;
    pUsbApp->devState = CY_USB_DEVICE_STATE_DEFAULT;
    pUsbApp->prevDevState = pUsbApp->devState;
    return;
} /* end of function. */

/**
 * \name Cy_USB_AppBusSpeedCallback
 * \brief   Callback function will be invoked by USBD when speed is identified or
 *          speed change is detected
 * \param pAppCtxt application layer context pointer.
 * \param pUsbdCtxt USBD context
 * \param pMsg USB Message
 * \retval None
 */
void Cy_USB_AppBusSpeedCallback(void *pAppCtxt, cy_stc_usb_usbd_ctxt_t *pUsbdCtxt,
                                cy_stc_usb_cal_msg_t *pMsg)
{
    cy_stc_usb_app_ctxt_t *pUsbApp;

    pUsbApp = (cy_stc_usb_app_ctxt_t *)pAppCtxt;
    pUsbApp->devState = CY_USB_DEVICE_STATE_DEFAULT;
    pUsbApp->devSpeed = Cy_USBD_GetDeviceSpeed(pUsbdCtxt);
    return;
} /* end of function. */

void Cy_JTAGTransfer(uint8_t count) {

      uint8_t full = count >> 2;
      uint8_t i;
      wValueData_t B;
      uint32_t intMask;
      intMask = Cy_SysLib_EnterCriticalSection();

      for (i = 0; i < full; i++)
      {
        /* Refernce code do not delete: 
            B = EP0BUF[i]; */
        /*
        JTAG_TMS = B_0_b;
        JTAG_TDI = B_1_b;
        JTAG_TCK = 1;
        B_0_b = JTAG_TDO;
        JTAG_TCK = 0;
        */
        B.Value = Ep0TestBuffer[i];
        (B.bits.b0) ?   Cy_LVDS_PhyGpioSet(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TMS):
                        Cy_LVDS_PhyGpioClr(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TMS);

        (B.bits.b1) ?   Cy_LVDS_PhyGpioSet(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TDI):
                        Cy_LVDS_PhyGpioClr(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TDI);

        Cy_LVDS_PhyGpioSet(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TCK);
        B.bits.b0  = (Cy_LVDS_PhyGpioRead(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TDO));
        Cy_LVDS_PhyGpioClr(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TCK);

        /*
        JTAG_TMS = B_2_b;
        JTAG_TDI = B_3_b;
        JTAG_TCK = 1;
        B_1_b = JTAG_TDO;
        JTAG_TCK = 0;
        */
        (B.bits.b2) ?   Cy_LVDS_PhyGpioSet(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TMS):
                        Cy_LVDS_PhyGpioClr(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TMS);

        (B.bits.b3) ?   Cy_LVDS_PhyGpioSet(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TDI):
                        Cy_LVDS_PhyGpioClr(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TDI);

        Cy_LVDS_PhyGpioSet(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TCK);
        B.bits.b1  = (Cy_LVDS_PhyGpioRead(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TDO));
        Cy_LVDS_PhyGpioClr(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TCK);

        /*
        JTAG_TMS = B_4_b;
        JTAG_TDI = B_5_b;
        JTAG_TCK = 1;
        B_2_b = JTAG_TDO;
        JTAG_TCK = 0;
        */
        (B.bits.b4) ?   Cy_LVDS_PhyGpioSet(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TMS):
                        Cy_LVDS_PhyGpioClr(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TMS);

        (B.bits.b5) ?   Cy_LVDS_PhyGpioSet(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TDI):
                        Cy_LVDS_PhyGpioClr(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TDI);

        Cy_LVDS_PhyGpioSet(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TCK);
        B.bits.b2  = (Cy_LVDS_PhyGpioRead(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TDO));
        Cy_LVDS_PhyGpioClr(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TCK);

        /*
        JTAG_TMS = B_6_b;
        JTAG_TDI = B_7_b;
        JTAG_TCK = 1;
        B_3_b = JTAG_TDO;
        JTAG_TCK = 0;
        */
        (B.bits.b6) ?   Cy_LVDS_PhyGpioSet(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TMS):
                        Cy_LVDS_PhyGpioClr(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TMS);

        (B.bits.b7) ?   Cy_LVDS_PhyGpioSet(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TDI):
                        Cy_LVDS_PhyGpioClr(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TDI);

        Cy_LVDS_PhyGpioSet(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TCK);
        B.bits.b3  = (Cy_LVDS_PhyGpioRead(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TDO));
        Cy_LVDS_PhyGpioClr(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TCK);

        Ep0TestBuffer[i] = (uint8_t) B.Value;
      }
    
      if (count & 3)
      {

        /*
        B = EP0BUF[full];
        JTAG_TMS = B_0_b;
        JTAG_TDI = B_1_b;
        JTAG_TCK = 1;
        B_0_b = JTAG_TDO;
        B_1_b = 0;
        JTAG_TCK = 0;
        */
            B.Value = Ep0TestBuffer[full];
            (B.bits.b0) ?   Cy_LVDS_PhyGpioSet(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TMS):
                            Cy_LVDS_PhyGpioClr(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TMS);

            (B.bits.b1) ?   Cy_LVDS_PhyGpioSet(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TDI):
                            Cy_LVDS_PhyGpioClr(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TDI);

            Cy_LVDS_PhyGpioSet(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TCK);
            B.bits.b0  = (Cy_LVDS_PhyGpioRead(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TDO));
            B.bits.b1 = 0;
            Cy_LVDS_PhyGpioClr(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TCK);

        if (count & 2)
        {
        /*
          JTAG_TMS = B_2_b;
          JTAG_TDI = B_3_b;
          JTAG_TCK = 1;
          B_1_b = JTAG_TDO;
          B_2_b = 0;
          JTAG_TCK = 0;
          */
            (B.bits.b2) ?   Cy_LVDS_PhyGpioSet(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TMS):
                            Cy_LVDS_PhyGpioClr(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TMS);

            (B.bits.b3) ?   Cy_LVDS_PhyGpioSet(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TDI):
                            Cy_LVDS_PhyGpioClr(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TDI);

            Cy_LVDS_PhyGpioSet(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TCK);
            B.bits.b1  = (Cy_LVDS_PhyGpioRead(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TDO));
            B.bits.b2 = 0;
            Cy_LVDS_PhyGpioClr(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TCK);
          if (count & 1)
          {
            /*
            JTAG_TMS = B_4_b;
            JTAG_TDI = B_5_b;
            JTAG_TCK = 1;
            B_2_b = JTAG_TDO;
            JTAG_TCK = 0;
            */
                (B.bits.b4) ?   Cy_LVDS_PhyGpioSet(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TMS):
                                Cy_LVDS_PhyGpioClr(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TMS);

                (B.bits.b5) ?   Cy_LVDS_PhyGpioSet(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TDI):
                                Cy_LVDS_PhyGpioClr(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TDI);

                Cy_LVDS_PhyGpioSet(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TCK);
                B.bits.b2  = (Cy_LVDS_PhyGpioRead(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TDO));
                Cy_LVDS_PhyGpioClr(LVDSSS_LVDS, 0, USB_SNIFFER_JTAG_TCK);
          }
        }
        else
        {
          //B_2_b = 0;
            B.bits.b2 = 0;
        }

        //B_3_b = 0;
        B.bits.b3 = 0;
        Ep0TestBuffer[full] = (uint8_t) B.Value;
      }
    Cy_SysLib_ExitCriticalSection(intMask);
}

/**
 * \name Cy_USB_AppSetupCallback
 * \brief Callback function will be invoked by USBD when SETUP packet is received
 * \param pAppCtxt application layer context pointer.
 * \param pUsbdCtxt USBD context
 * \param pMsg USB Message
 * \retval None
 */
void Cy_USB_AppSetupCallback(void *pAppCtxt, cy_stc_usb_usbd_ctxt_t *pUsbdCtxt,
                             cy_stc_usb_cal_msg_t *pMsg)
{
    cy_stc_usb_app_ctxt_t *pUsbApp;
    pUsbApp = (cy_stc_usb_app_ctxt_t *)pAppCtxt;
    uint8_t bRequest, bReqType;
    uint8_t bType, bTarget;
    uint16_t wValue, wIndex, wLength;
    bool isReqHandled = false;
    cy_stc_usbd_app_msg_t xMsg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    cy_en_usbd_ret_code_t retStatus = CY_USBD_STATUS_SUCCESS;
    cy_en_usb_endp_dir_t epDir = CY_USB_ENDP_DIR_INVALID;


    DBG_APP_INFO("AppSetupCallback\r\n");

    /* Fast enumeration is used. Only requests addressed to the interface, class,
     * vendor and unknown control requests are received by this function. */

    /* Decode the fields from the setup request. */
    bReqType = pUsbdCtxt->setupReq.bmRequest;
    bType = ((bReqType & CY_USB_CTRL_REQ_TYPE_MASK) >> CY_USB_CTRL_REQ_TYPE_POS);
    bTarget = (bReqType & CY_USB_CTRL_REQ_RECIPENT_OTHERS);
    bRequest = pUsbdCtxt->setupReq.bRequest;
    wValue = pUsbdCtxt->setupReq.wValue;
    wIndex = pUsbdCtxt->setupReq.wIndex;
    wLength = pUsbdCtxt->setupReq.wLength;

    if (bType == CY_USB_CTRL_REQ_STD)
    {
        DBG_APP_INFO("CY_USB_CTRL_REQ_STD\r\n");
        if (bRequest == CY_USB_SC_SET_FEATURE)
        {
            if ((bTarget == CY_USB_CTRL_REQ_RECIPENT_INTF) && (wValue == 0))
            {
                Cy_USB_USBD_EndpSetClearStall(pUsbdCtxt, 0x00, CY_USB_ENDP_DIR_IN, TRUE);
                isReqHandled = true;
            }

            /* SET-FEATURE(EP-HALT) is only supported to facilitate Chapter 9 compliance tests. */
            if ((bTarget == CY_USB_CTRL_REQ_RECIPENT_ENDP) && (wValue == CY_USB_FEATURE_ENDP_HALT))
            {
                epDir = ((wIndex & 0x80UL) ? (CY_USB_ENDP_DIR_IN) : (CY_USB_ENDP_DIR_OUT));
                Cy_USB_USBD_EndpSetClearStall(pUsbdCtxt, ((uint32_t)wIndex & 0x7FUL),
                        epDir, true);

                Cy_USBD_SendAckSetupDataStatusStage(pUsbdCtxt);
                isReqHandled = true;
            }
        }

        if (bRequest == CY_USB_SC_CLEAR_FEATURE)
        {
            if ((bTarget == CY_USB_CTRL_REQ_RECIPENT_INTF) && (wValue == 0))
            {
                Cy_USB_USBD_EndpSetClearStall(pUsbdCtxt, 0x00, CY_USB_ENDP_DIR_IN, TRUE);

                isReqHandled = true;
            }

            if ((bTarget == CY_USB_CTRL_REQ_RECIPENT_ENDP) && (wValue == CY_USB_FEATURE_ENDP_HALT))
            {
                epDir = ((wIndex & 0x80UL) ? (CY_USB_ENDP_DIR_IN) : (CY_USB_ENDP_DIR_OUT));
                if(((wIndex & 0x7F) == pUsbApp->paInEpNum1))
                {
                    /*Stop stream from FPGA*/
                }
                Cy_USBD_SendAckSetupDataStatusStage(pUsbdCtxt);
                isReqHandled = true;
            }
        }

        /* Handle Microsoft OS String Descriptor request. */
        if ((bTarget == CY_USB_CTRL_REQ_RECIPENT_DEVICE) &&
                (bRequest == CY_USB_SC_GET_DESCRIPTOR) &&
                (wValue == ((CY_USB_STRING_DSCR << 8) | 0xEE))) {

            /* Make sure we do not send more data than requested. */
            if (wLength > glOsString[0]) {
                wLength = glOsString[0];
            }

            DBG_APP_INFO("OSString\r\n");
            retStatus = Cy_USB_USBD_SendEndp0Data(pUsbdCtxt, (uint8_t *)glOsString, wLength);
            if(retStatus == CY_USBD_STATUS_SUCCESS) {
                isReqHandled = true;
            }
        }

    }

    if (bType == CY_USB_CTRL_REQ_VENDOR) {
        /* If trying to bind to WinUSB driver, we need to support additional control requests. */
        /* Handle OS Compatibility and OS Feature requests */

        if (bRequest == MS_VENDOR_CODE) {
            if (wIndex == 0x04) {
                if (wLength > *((uint16_t *)glOsCompatibilityId)) {
                    wLength = *((uint16_t *)glOsCompatibilityId);
                }

                DBG_APP_INFO("OSCompat\r\n");
                retStatus = Cy_USB_USBD_SendEndp0Data(pUsbdCtxt, (uint8_t *)glOsCompatibilityId, wLength);
                if(retStatus == CY_USBD_STATUS_SUCCESS) {
                    isReqHandled = true;
                }
            }
            else if (wIndex == 0x05) {
                if (wLength > *((uint16_t *)glOsFeature)) {
                    wLength = *((uint16_t *)glOsFeature);
                }

                DBG_APP_INFO("OSFeature\r\n");
                retStatus = Cy_USB_USBD_SendEndp0Data(pUsbdCtxt, (uint8_t *)glOsFeature, wLength);
                if(retStatus == CY_USBD_STATUS_SUCCESS) {
                    isReqHandled = true;
                }
            }
        }

        if (isReqHandled) {
            return;
        }
    }

    if ((bType == CY_USB_CTRL_REQ_VENDOR) && 
        (bTarget == CY_USB_CTRL_REQ_RECIPENT_DEVICE) && 
        (bRequest == CMD_CTRL)) {       
        
        uint8_t Index = wValue & 0x0F;
        switch(Index) {
            case CAPTURE_CTRL_RESET:
                DBG_APP_INFO("CAPTURE_CTRL_RESET\r\n");
                isReqHandled = true;
                xMsg.type = CAPTURE_CTRL_RESET;
                xMsg.data[0] = BULK_IN_ENDPOINT_1;              
                xMsg.data[1] = wValue;
                xQueueSendFromISR(pUsbApp->usbMsgQueue, &(xMsg), &(xHigherPriorityTaskWoken));
                break;
            case CAPTURE_CTRL_ENABLE:
                DBG_APP_INFO("CAPTURE_CTRL_ENABLE\r\n");
                isReqHandled = true;
                xMsg.type = CAPTURE_CTRL_ENABLE;
                xMsg.data[0] = BULK_IN_ENDPOINT_1;              
                xMsg.data[1] = wValue;
                xQueueSendFromISR(pUsbApp->usbMsgQueue, &(xMsg), &(xHigherPriorityTaskWoken));  
                break;              
            case CAPTURE_CTRL_SPEED0:
                DBG_APP_INFO("CAPTURE_CTRL_SPEED0\r\n");
                isReqHandled = true;
                xMsg.type = CAPTURE_CTRL_SPEED0;
                xMsg.data[0] = BULK_IN_ENDPOINT_1;              
                xMsg.data[1] = wValue;
                xQueueSendFromISR(pUsbApp->usbMsgQueue, &(xMsg), &(xHigherPriorityTaskWoken));      
                break;              
            case CAPTURE_CTRL_SPEED1:
                DBG_APP_INFO("CAPTURE_CTRL_SPEED1\r\n");
                isReqHandled = true;
                xMsg.type = CAPTURE_CTRL_SPEED1;
                xMsg.data[0] = BULK_IN_ENDPOINT_1;              
                xMsg.data[1] = wValue;
                xQueueSendFromISR(pUsbApp->usbMsgQueue, &(xMsg), &(xHigherPriorityTaskWoken));  
                break;              
            case CAPTURE_CTRL_TEST:
                DBG_APP_INFO("CAPTURE_CTRL_TEST\r\n");
                isReqHandled = true;
                xMsg.type = CAPTURE_CTRL_TEST;
                xMsg.data[0] = BULK_IN_ENDPOINT_1;              
                xMsg.data[1] = wValue;
                xQueueSendFromISR(pUsbApp->usbMsgQueue, &(xMsg), &(xHigherPriorityTaskWoken));
                break;              
            default:
                break;
        }

        if (isReqHandled) {
            return;
        }
    }
    else if ((bType == CY_USB_CTRL_REQ_VENDOR) && 
        (bTarget == CY_USB_CTRL_REQ_RECIPENT_DEVICE) && 
        (bRequest == CMD_I2C_READ)) {       
        DBG_APP_INFO("CMD_I2C_READ\r\n");
        
        if (!Cy_I2C_MasterRead(SCB0, wValue, Ep0TestBuffer, wLength, true)) {
            retStatus = Cy_USB_USBD_SendEndp0Data(pUsbdCtxt, Ep0TestBuffer, wLength);               
            if (retStatus == CY_USBD_STATUS_SUCCESS) {              
                isReqHandled = true;
            }
            if (isReqHandled) {
                return;
            }           
        }

    }
    else if ((bType == CY_USB_CTRL_REQ_VENDOR) && 
        (bTarget == CY_USB_CTRL_REQ_RECIPENT_DEVICE) && 
        (bRequest == CMD_I2C_WRITE)) {      
        
        DBG_APP_INFO("CMD_I2C_WRITE\r\n");      
        retStatus = Cy_USB_USBD_RecvEndp0Data(pUsbdCtxt,(uint8_t *)Ep0TestBuffer, wLength);
        if (retStatus == CY_USBD_STATUS_SUCCESS) {
            if (!Cy_I2C_MasterWrite(SCB0, wValue, Ep0TestBuffer, wLength, true)) {
                isReqHandled = true;
            }
            if (isReqHandled) {
                return;
            }   
        }
    }
    else if ((bType == CY_USB_CTRL_REQ_VENDOR) && 
        (bTarget == CY_USB_CTRL_REQ_RECIPENT_DEVICE) && 
        (bRequest == CMD_JTAG_ENABLE)) {        
        if (wValue) {
            DBG_APP_INFO("CMD_JTAG_ENABLE\r\n");            
        }
        else {
            DBG_APP_INFO("CMD_JTAG_DISABLE\r\n");
        }
        Cy_JTAGInit(wValue);    
        isReqHandled = true;
        if (isReqHandled) {
            return;
        }       
    }   
    else if ((bType == CY_USB_CTRL_REQ_VENDOR) && 
        (bTarget == CY_USB_CTRL_REQ_RECIPENT_DEVICE) && 
        (bRequest == CMD_JTAG_REQUEST)) {       
        
        DBG_APP_INFO("CMD_JTAG_REQUEST\r\n");
        isReqHandled = true;
        xMsg.type = CMD_JTAG_REQUEST;
        xMsg.data[0] = BULK_IN_ENDPOINT_1;
        xMsg.data[1] = (wValue<<16)| wLength;

        xQueueSendFromISR(pUsbApp->usbMsgQueue, &(xMsg), &(xHigherPriorityTaskWoken));
        if (isReqHandled) {
            return;
        }       
    }       
    else if ((bType == CY_USB_CTRL_REQ_VENDOR) && 
        (bTarget == CY_USB_CTRL_REQ_RECIPENT_DEVICE) && 
        (bRequest == CMD_JTAG_RESPONSE)) {      
        
        DBG_APP_INFO("CMD_JTAG_RESPONSE\r\n");
        retStatus = Cy_USB_USBD_SendEndp0Data(pUsbdCtxt, Ep0TestBuffer, wLength);
        if (retStatus == CY_USBD_STATUS_SUCCESS) {              
            isReqHandled = true;
            DBG_APP_INFO("CMD_JTAG_RESPONSE true\r\n");
        }
        if (isReqHandled) {
            return;
        }       
    }       

    /* If Request is not handled by the callback, Stall the command */
    if (!isReqHandled)
    {
        Cy_USB_USBD_EndpSetClearStall(pUsbdCtxt, 0x00, CY_USB_ENDP_DIR_IN, TRUE);
    }
} /* end of function. */

/**
 * \name Cy_USB_AppSuspendCallback
 * \brief  Callback function will be invoked by USBD when Suspend signal/message is detected
 * \param pAppCtxt application layer context pointer.
 * \param pUsbdCtxt USBD context
 * \param pMsg USB Message
 * \retval None
 */
void Cy_USB_AppSuspendCallback(void *pAppCtxt, cy_stc_usb_usbd_ctxt_t *pUsbdCtxt,
                               cy_stc_usb_cal_msg_t *pMsg)
{
    cy_stc_usb_app_ctxt_t *pUsbApp;

    pUsbApp = (cy_stc_usb_app_ctxt_t *)pAppCtxt;
    pUsbApp->prevDevState = pUsbApp->devState;
    pUsbApp->devState = CY_USB_DEVICE_STATE_SUSPEND;
} /* end of function. */

/**
 * \name Cy_USB_AppResumeCallback
 * \brief Callback function will be invoked by USBD when Resume signal/message is detected
 * \param pAppCtxt application layer context pointer.
 * \param pUsbdCtxt USBD context
 * \param pMsg USB Message
 * \retval None
 */
void Cy_USB_AppResumeCallback(void *pAppCtxt, cy_stc_usb_usbd_ctxt_t *pUsbdCtxt,
                              cy_stc_usb_cal_msg_t *pMsg)
{
    cy_stc_usb_app_ctxt_t *pUsbApp;
    cy_en_usb_device_state_t tempState;

    pUsbApp = (cy_stc_usb_app_ctxt_t *)pAppCtxt;

    tempState = pUsbApp->devState;
    pUsbApp->devState = pUsbApp->prevDevState;
    pUsbApp->prevDevState = tempState;
    return;
} /* end of function. */

/**
 * \name Cy_USB_AppSetIntfCallback
 * \brief Callback function will be invoked by USBD when SET_INTERFACE is  received
 * \param pAppCtxt application layer context pointer.
 * \param pUsbdCtxt USBD context
 * \param pMsg USB Message
 * \retval None
 */
void Cy_USB_AppSetIntfCallback(void *pAppCtxt, cy_stc_usb_usbd_ctxt_t *pUsbdCtxt,
                               cy_stc_usb_cal_msg_t *pMsg)
{
    cy_stc_usb_setup_req_t *pSetupReq;
    uint8_t intfNum, altSetting;
    int8_t numOfEndp;
    uint8_t *pIntfDscr, *pEndpDscr;
    uint32_t endpNumber;
    cy_en_usb_endp_dir_t endpDirection;
    cy_stc_usb_app_ctxt_t *pUsbApp = (cy_stc_usb_app_ctxt_t *)pAppCtxt;

    DBG_APP_INFO("AppSetIntfCallback Start\r\n");
    pSetupReq = &(pUsbdCtxt->setupReq);
    /*
     * Get interface and alt setting info. If new setting same as previous
     * then return.
     * If new alt setting came then first Unconfigure previous settings
     * and then configure new settings.
     */
    intfNum = pSetupReq->wIndex;
    altSetting = pSetupReq->wValue;

    if (altSetting == pUsbApp->prevAltSetting)
    {
        DBG_APP_INFO("SameAltSetting\r\n");
        Cy_USB_USBD_EndpSetClearStall(pUsbdCtxt, 0x00, CY_USB_ENDP_DIR_IN, TRUE);
        return;
    }

    /* New altSetting is different than previous one so unconfigure previous. */
    pIntfDscr = Cy_USBD_GetIntfDscr(pUsbdCtxt, intfNum, pUsbApp->prevAltSetting);
    DBG_APP_INFO("unconfigPrevAltSet\r\n");
    if (pIntfDscr == NULL)
    {
        DBG_APP_INFO("pIntfDscrNull\r\n");
        return;
    }
    numOfEndp = Cy_USBD_FindNumOfEndp(pIntfDscr);
    if (numOfEndp == 0x00)
    {
        DBG_APP_INFO("SetIntf:prevNumEp 0\r\n");
    }
    else
    {
        pEndpDscr = Cy_USBD_GetEndpDscr(pUsbdCtxt, pIntfDscr);
        while (numOfEndp != 0x00)
        {
            if (*(pEndpDscr + CY_USB_ENDP_DSCR_OFFSET_ADDRESS) & 0x80)
            {
                endpDirection = CY_USB_ENDP_DIR_IN;
            }
            else
            {
                endpDirection = CY_USB_ENDP_DIR_OUT;
            }
            endpNumber =
                (uint32_t)((*(pEndpDscr + CY_USB_ENDP_DSCR_OFFSET_ADDRESS)) & 0x7F);

            /* with FALSE, unconfgure previous settings. */
            Cy_USBD_EnableEndp(pUsbdCtxt, endpNumber, endpDirection, FALSE);

            numOfEndp--;
            pEndpDscr = (pEndpDscr + (*(pEndpDscr + CY_USB_DSCR_OFFSET_LEN)));
        }
    }

    /* Now take care of different config with new alt setting. */
    pUsbApp->prevAltSetting = altSetting;
    pIntfDscr = Cy_USBD_GetIntfDscr(pUsbdCtxt, intfNum, altSetting);
    if (pIntfDscr == NULL)
    {
        DBG_APP_INFO("pIntfDscrNull\r\n");
        return;
    }

    numOfEndp = Cy_USBD_FindNumOfEndp(pIntfDscr);
    if (numOfEndp == 0x00)
    {
        DBG_APP_INFO("SetIntf:numEp 0\r\n");
    }
    else
    {
        pUsbApp->prevAltSetting = altSetting;
        pEndpDscr = Cy_USBD_GetEndpDscr(pUsbdCtxt, pIntfDscr);
        while (numOfEndp != 0x00)
        {
            Cy_USB_AppConfigureEndp(pUsbdCtxt, pEndpDscr);
            Cy_USB_AppSetupEndpDmaParamsHs(pAppCtxt, pEndpDscr);
            numOfEndp--;
            pEndpDscr = (pEndpDscr + (*(pEndpDscr + CY_USB_DSCR_OFFSET_LEN)));
        }
    }

    DBG_APP_INFO("AppSetIntfCallback done\r\n");
    return;
} /* end of function. */

/**
 * \name Cy_USB_AppQueueWrite
 * \brief Queue USBHS Write on the USB endpoint
 * \param pAppCtxt application layer context pointer.
 * \param endpNumber Endpoint number
 * \param pBuffer Data Buffer Pointer
 * \param dataSize DataSize to send on USB bus
 * \retval None
 */
void Cy_USB_AppQueueWrite(cy_stc_usb_app_ctxt_t *pAppCtxt, uint8_t endpNumber,
                          uint8_t *pBuffer, uint16_t dataSize)
{
    cy_stc_app_endp_dma_set_t *dmaset_p=NULL;

    /* Null pointer checks. */
    if ((pAppCtxt == NULL) || (pAppCtxt->pUsbdCtxt == NULL) ||
            (pAppCtxt->pCpuDw1Base == NULL) || (pBuffer == NULL)) 
    {
        DBG_APP_ERR("QueueWrite Err0\r\n");
        return;
    }

    /*
     * Verify that the selected endpoint is valid and the dataSize
     * is non-zero.
     */
    dmaset_p = &(pAppCtxt->endpInDma[endpNumber]);
    if ((dmaset_p->valid == 0) || (dataSize == 0)) 
    {
        DBG_APP_ERR("QueueWrite Err1 %d %d\r\n",dmaset_p->valid,dataSize);
        return;
    }

    Cy_USBHS_App_QueueWrite(dmaset_p, pBuffer, dataSize);
} /* end of function */

/**
 * \name Cy_USB_AppInitDmaIntr
 * \brief Function to register an ISR for the DMA channel associated with an endpoint
 * \param endpNumber USB endpoint number
 * \param endpDirection Endpoint direction
 * \param userIsr ISR function pointer. Can be NULL if interrupt is to be disabled.
 * \retval None
 */
void Cy_USB_AppInitDmaIntr(uint32_t endpNumber, cy_en_usb_endp_dir_t endpDirection,
                           cy_israddress userIsr)
{
    cy_stc_sysint_t intrCfg;
    if ((endpNumber > 0) && (endpNumber < CY_USB_MAX_ENDP_NUMBER))
    {
#if (!CY_CPU_CORTEX_M4)

        if (endpDirection == CY_USB_ENDP_DIR_IN)
        {
            intrCfg.intrPriority = 3;
            intrCfg.intrSrc = NvicMux1_IRQn;
            /* DW1 channels 0 onwards are used for IN endpoints. */
            intrCfg.cm0pSrc = (cy_en_intr_t)(cpuss_interrupts_dw1_0_IRQn + endpNumber);
        }
        else
        {
            intrCfg.intrPriority = 3;
            intrCfg.intrSrc = NvicMux6_IRQn;
            /* DW0 channels 0 onwards are used for OUT endpoints. */
            intrCfg.cm0pSrc = (cy_en_intr_t)(cpuss_interrupts_dw0_0_IRQn + endpNumber);
        }
#else
        intrCfg.intrPriority = 5;
        if (endpDirection == CY_USB_ENDP_DIR_IN)
        {
            /* DW1 channels 0 onwards are used for IN endpoints. */
            intrCfg.intrSrc =
                (IRQn_Type)(cpuss_interrupts_dw1_0_IRQn + endpNumber);
        }
        else
        {
            /* DW0 channels 0 onwards are used for OUT endpoints. */
            intrCfg.intrSrc =
                (IRQn_Type)(cpuss_interrupts_dw0_0_IRQn + endpNumber);
        }
#endif /* (!CY_CPU_CORTEX_M4) */

        if (userIsr != NULL)
        {
            /* If an ISR is provided, register it and enable the interrupt. */
            Cy_SysInt_Init(&intrCfg, userIsr);
            NVIC_EnableIRQ(intrCfg.intrSrc);
        }
        else
        {
            /* ISR is NULL. Disable the interrupt. */
            NVIC_DisableIRQ(intrCfg.intrSrc);
        }
    }
}

/**
 * \name Cy_USB_AppClearDmaInterrupt
 * \brief Clear DMA Interrupt
 * \param pAppCtxt application layer context pointer.
 * \param endpNumber Endpoint number
 * \param endpDirection Endpoint direction
 * \retval None
 */
void Cy_USB_AppClearDmaInterrupt(cy_stc_usb_app_ctxt_t *pAppCtxt,
                                 uint32_t endpNumber, cy_en_usb_endp_dir_t endpDirection)
{
    if ((pAppCtxt != NULL) && (endpNumber > 0) &&
            (endpNumber < CY_USB_MAX_ENDP_NUMBER)) {
        if (endpDirection == CY_USB_ENDP_DIR_IN) {
            Cy_USBHS_App_ClearDmaInterrupt(&(pAppCtxt->endpInDma[endpNumber]));
        } else  {
            Cy_USBHS_App_ClearDmaInterrupt(&(pAppCtxt->endpOutDma[endpNumber]));
        }
    }
} /* end of function. */

/**
 * \name Cy_PA_AppHandleRxCompletion
 * \brief Protocol Analyzer receive completion handler
 * \param pUsbApp application layer context pointer.
 * \param  index channel index number
 * \retval None
 */
void Cy_PA_AppHandleRxCompletion (cy_stc_usb_app_ctxt_t *pUsbApp, uint8_t index)
{
    cy_stc_hbdma_buff_status_t buffStat;
    cy_en_hbdma_mgr_status_t   dmaStat;
    uint32_t intMask;

    if( index == 1)
    {
        /* At least one buffer must be pending. */
        if (pUsbApp->paPendingRxBufCnt1 == 0)
        {
            DBG_APP_ERR("PendingBufCnt=0 on SendComplete\r\n");
            return;
        }

        /* The buffer which has been sent to the USB host can be discarded. */
        dmaStat = Cy_HBDma_Channel_DiscardBuffer(pUsbApp->hbBulkInChannel[0], &buffStat);
        if (dmaStat != CY_HBDMA_MGR_SUCCESS)
        {
            DBG_APP_ERR("DiscardBuffer failed with status=%x\r\n", dmaStat);
            return;
        }

        /* If another DMA buffer has already been filled by the producer, go
        * on and send it to the host controller.
        */
        intMask = Cy_SysLib_EnterCriticalSection();
        pUsbApp->paPendingRxBufCnt1--;
        if (pUsbApp->paPendingRxBufCnt1 > 0)
        {
            Cy_SysLib_ExitCriticalSection(intMask);
            Cy_PA_AppHandleRxEvent(pUsbApp, pUsbApp->hbBulkInChannel[0], 0, index);
        }
        else
        {
            Cy_SysLib_ExitCriticalSection(intMask);
        }
    }
}

/**
 * \name Cy_CheckStatus
 * \brief Function that handles prints error log
 * \param function Pointer to function
 * \param line Line number where error is seen
 * \param condition condition of failure
 * \param value error code
 * \param isBlocking blocking function
 * \return None
 */
void Cy_CheckStatus(const char *function, uint32_t line, uint8_t condition, uint32_t value, uint8_t isBlocking)
{
    if (!condition)
    {
        /* Application failed with the error code status */
        Cy_Debug_AddToLog(1, RED);
        Cy_Debug_AddToLog(1, "Function %s failed at line %d with status = 0x%x\r\n", function, line, value);
        Cy_Debug_AddToLog(1, COLOR_RESET);
        if (isBlocking)
        {
            /* Loop indefinitely */
            for (;;)
            {
            }
        }
    }
}

/**
 * \name Cy_CheckStatusHandleFailure
 * \brief Function that handles prints error log
 * \param function Pointer to function
 * \param line LineNumber where error is seen
 * \param condition Line number where error is seen
 * \param value error code
 * \param isBlocking blocking function
 * \param failureHandler failure handler function
 * \return None
 */
void Cy_CheckStatusHandleFailure(const char *function, uint32_t line, uint8_t condition, uint32_t value, uint8_t isBlocking, void (*failureHandler)(void))
{
    if (!condition)
    {
        /* Application failed with the error code status */
        Cy_Debug_AddToLog(1, RED);
        Cy_Debug_AddToLog(1, "Function %s failed at line %d with status = 0x%x\r\n", function, line, value);
        Cy_Debug_AddToLog(1, COLOR_RESET);

        if(failureHandler != NULL)
        {
            (*failureHandler)();
        }
        if (isBlocking)
        {
            /* Loop indefinitely */
            for (;;)
            {
            }
        }
    }
}

/**
 * \name Cy_FailHandler
 * \brief Error Handler
 * \retval None
 */
void Cy_FailHandler(void)
{
    DBG_APP_ERR("Reset Done\r\n");
}

/* [] END OF FILE */
