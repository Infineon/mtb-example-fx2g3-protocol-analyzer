/***************************************************************************//**
* \file gpif_headers.h
* \version 1.0
*
* \brief This header provides for GPIF configuration
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

#ifndef _CY_GPIF_HEADER_LVCMOS_H_
#define _CY_GPIF_HEADER_LVCMOS_H_

#if defined(__cplusplus)
extern "C" {
#endif

const cy_stc_lvds_gpif_wavedata_t cy_lvds_gpif_wavedata0[] = {
    {{0x2E738101,0x00000100,0x80000080,0x00000000},{0x00000000,0x00000000,0x00000000,0x00000000}},
    {{0x1E728102,0x200D0006,0x80000040,0x00000000},{0x00000000,0x00000000,0x00000000,0x00000000}},
    {{0x4E728103,0x000D0106,0x80000000,0x00000000},{0x1E738104,0x20000C00,0x80000000,0x00000000}},
    {{0x1E728102,0x200D0006,0x80000040,0x00000000},{0x1E738104,0x20000C00,0x80000000,0x00000000}},
    {{0x1E738106,0x000D0C04,0x80000000,0x00000000},{0x00000000,0x00000000,0x00000000,0x00000000}},
    {{0x5E739C07,0x00000C00,0x80100000,0x00000000},{0x1E738104,0x20000C00,0x80000000,0x00000000}},
    {{0x5E739C08,0x00000C00,0x88000000,0x00000000},{0x00000000,0x00000000,0x00000000,0x00000000}},
    {{0x2E739A05,0x00000100,0x80000000,0x00000000},{0x00000000,0x00000000,0x00000000,0x00000000}}
};

uint16_t cy_lvds_gpif_transition0[] = {
    0x0000, 0x5555, 0xAAAA, 0x8888, 0x2222, 0xFFFF
};

uint8_t cy_lvds_gpif_wavedata_position0[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x00, 0x05, 0x06,
    0x07
};

const cy_stc_lvds_gpif_reg_data_t cy_lvds_gpif_reg_data[] = {
    {0x00000000,    0x00048070},    /* GPIF_CONFIG */
    {0x00000004,    0x00002000},    /* GPIF_BUS_CONFIG */
    {0x00000008,    0x00000000},    /* GPIF_BUS_CONFIG2 */
    {0x0000000C,    0x00600046},    /* GPIF_AD_CONFIG */
    {0x00000010,    0x03020100},    /* GPIF_CTL_FUNC0 */
    {0x00000014,    0x07060504},    /* GPIF_CTL_FUNC1 */
    {0x00000018,    0x0B0A0908},    /* GPIF_CTL_FUNC2 */
    {0x0000001C,    0x0F0E0D0C},    /* GPIF_CTL_FUNC3 */
    {0x00000020,    0x13121110},    /* GPIF_CTL_FUNC4 */
    {0x00000034,    0x00000000},    /* GPIF_INTR_MASK */
    {0x00000050,    0x00000001},    /* GPIF_CTRL_BUS_DIRECTION[00] */
    {0x00000054,    0x00000001},    /* GPIF_CTRL_BUS_DIRECTION[01] */
    {0x00000058,    0x00000001},    /* GPIF_CTRL_BUS_DIRECTION[02] */
    {0x0000005C,    0x00000001},    /* GPIF_CTRL_BUS_DIRECTION[03] */
    {0x00000060,    0x00000001},    /* GPIF_CTRL_BUS_DIRECTION[04] */
    {0x00000064,    0x00000002},    /* GPIF_CTRL_BUS_DIRECTION[05] */
    {0x00000068,    0x00000002},    /* GPIF_CTRL_BUS_DIRECTION[06] */
    {0x0000006C,    0x00000001},    /* GPIF_CTRL_BUS_DIRECTION[07] */
    {0x00000070,    0x00000001},    /* GPIF_CTRL_BUS_DIRECTION[08] */
    {0x00000074,    0x00000001},    /* GPIF_CTRL_BUS_DIRECTION[09] */
    {0x00000078,    0x00000001},    /* GPIF_CTRL_BUS_DIRECTION[10] */
    {0x0000007C,    0x00000001},    /* GPIF_CTRL_BUS_DIRECTION[11] */
    {0x00000080,    0x00000001},    /* GPIF_CTRL_BUS_DIRECTION[12] */
    {0x00000084,    0x00000001},    /* GPIF_CTRL_BUS_DIRECTION[13] */
    {0x00000088,    0x00000001},    /* GPIF_CTRL_BUS_DIRECTION[14] */
    {0x0000008C,    0x00000001},    /* GPIF_CTRL_BUS_DIRECTION[15] */
    {0x00000090,    0x00000001},    /* GPIF_CTRL_BUS_DIRECTION[16] */
    {0x00000094,    0x00000001},    /* GPIF_CTRL_BUS_DIRECTION[17] */
    {0x00000098,    0x00000001},    /* GPIF_CTRL_BUS_DIRECTION[18] */
    {0x0000009C,    0x00000001},    /* GPIF_CTRL_BUS_DIRECTION[19] */
    {0x000000B0,    0x0000FF7D},    /* GPIF_CTRL_BUS_DEFAULT */
    {0x000000B4,    0x00000000},    /* GPIF_CTRL_BUS_POLARITY */
    {0x000000B8,    0x00000000},    /* GPIF_CTRL_BUS_TOGGLE */
    {0x00000100,    0x00000000},    /* GPIF_CTRL_BUS_SELECT[00] */
    {0x00000104,    0x00000000},    /* GPIF_CTRL_BUS_SELECT[01] */
    {0x00000108,    0x00000000},    /* GPIF_CTRL_BUS_SELECT[02] */
    {0x0000010C,    0x00000000},    /* GPIF_CTRL_BUS_SELECT[03] */
    {0x00000110,    0x00000000},    /* GPIF_CTRL_BUS_SELECT[04] */
    {0x00000114,    0x00000000},    /* GPIF_CTRL_BUS_SELECT[05] */
    {0x00000118,    0x00000001},    /* GPIF_CTRL_BUS_SELECT[06] */
    {0x0000011C,    0x00000000},    /* GPIF_CTRL_BUS_SELECT[07] */
    {0x00000120,    0x00000000},    /* GPIF_CTRL_BUS_SELECT[08] */
    {0x00000124,    0x00000000},    /* GPIF_CTRL_BUS_SELECT[09] */
    {0x00000128,    0x00000000},    /* GPIF_CTRL_BUS_SELECT[10] */
    {0x0000012C,    0x00000000},    /* GPIF_CTRL_BUS_SELECT[11] */
    {0x00000130,    0x00000000},    /* GPIF_CTRL_BUS_SELECT[12] */
    {0x00000134,    0x00000000},    /* GPIF_CTRL_BUS_SELECT[13] */
    {0x00000138,    0x00000000},    /* GPIF_CTRL_BUS_SELECT[14] */
    {0x0000013C,    0x00000000},    /* GPIF_CTRL_BUS_SELECT[15] */
    {0x00000140,    0x0000001F},    /* GPIF_CTRL_BUS_SELECT[16] */
    {0x00000144,    0x0000001F},    /* GPIF_CTRL_BUS_SELECT[17] */
    {0x00000148,    0x0000001F},    /* GPIF_CTRL_BUS_SELECT[18] */
    {0x0000014C,    0x0000001F},    /* GPIF_CTRL_BUS_SELECT[19] */
    {0x00000160,    0x00000006},    /* GPIF_CTRL_COUNT_CONFIG */
    {0x00000164,    0x00000000},    /* GPIF_CTRL_COUNT_RESET */
    {0x00000168,    0x0000FFFF},    /* GPIF_CTRL_COUNT_LIMIT */
    {0x00000170,    0x0000010A},    /* GPIF_ADDR_COUNT_CONFIG */
    {0x00000174,    0x00000000},    /* GPIF_ADDR_COUNT_RESET */
    {0x00000178,    0x0000FFFF},    /* GPIF_ADDR_COUNT_LIMIT */
    {0x00000180,    0x00000000},    /* GPIF_STATE_COUNT_CONFIG */
    {0x00000184,    0x0000FFFF},    /* GPIF_STATE_COUNT_LIMIT */
    {0x00000190,    0x00000109},    /* GPIF_DATA_COUNT_CONFIG */
    {0x00000194,    0x00000000},    /* GPIF_DATA_COUNT_RESET_LSB */
    {0x0000019C,    0x00000000},    /* GPIF_DATA_COUNT_RESET_MSB */
    {0x000001A0,    0x00001FFA},    /* GPIF_DATA_COUNT_LIMIT_LSB */
    {0x000001A4,    0x00000000},    /* GPIF_DATA_COUNT_LIMIT_MSB */
    {0x000001A8,    0x00000000},    /* GPIF_CTRL_COMP_VALUE */
    {0x000001AC,    0x00000000},    /* GPIF_CTRL_COMP_MASK */
    {0x000001B0,    0x00000000},    /* GPIF_DATA_COMP_VALUE_WORD0 */
    {0x000001B4,    0x00000000},    /* GPIF_DATA_COMP_VALUE_WORD1 */
    {0x000001B8,    0x00000000},    /* GPIF_DATA_COMP_VALUE_WORD2 */
    {0x000001BC,    0x00000000},    /* GPIF_DATA_COMP_VALUE_WORD3 */
    {0x000001D0,    0x00000000},    /* GPIF_DATA_COMP_MASK_WORD0 */
    {0x000001D4,    0x00000000},    /* GPIF_DATA_COMP_MASK_WORD1 */
    {0x000001D8,    0x00000000},    /* GPIF_DATA_COMP_MASK_WORD2 */
    {0x000001DC,    0x00000000},    /* GPIF_DATA_COMP_MASK_WORD3 */
    {0x000001E0,    0x00000000},    /* GPIF_ADDR_COMP_VALUE */
    {0x000001E4,    0x00000000},    /* GPIF_ADDR_COMP_MASK */
    {0x00000200,    0x00040000},    /* GPIF_WAVEFORM_CTRL_STAT */
    {0x00000204,    0x00000000},    /* GPIF_WAVEFORM_SWITCH */
    {0x00000208,    0x00000000},    /* GPIF_WAVEFORM_SWITCH_TIMEOUT */
    {0x00000210,    0x00000000},    /* GPIF_CRC_CALC_CONFIG */
    {0x00000218,    0xFFFFFFC1},    /* GPIF_BETA_DEASSERT */
    {0x000002B0,    0x0000FFFF},    /* LINK_IDLE_CFG */
    {0x000002C0,    0x00000100}     /* LVCMOS_CLK_OUT_CFG 30MHz USB2 480MHz*/
};

//Pushing data into Thread 0 Socket
const cy_stc_lvds_gpif_config_t cy_lvcmos_gpif0_config = {
    (uint16_t)(sizeof(cy_lvds_gpif_wavedata_position0)/sizeof(uint8_t)),
    (cy_stc_lvds_gpif_wavedata_t *) cy_lvds_gpif_wavedata0,
    cy_lvds_gpif_wavedata_position0,
    (uint16_t)(sizeof(cy_lvds_gpif_transition0)/sizeof(uint16_t)),
    cy_lvds_gpif_transition0,
    (uint16_t)(sizeof(cy_lvds_gpif_reg_data)/sizeof(cy_stc_lvds_gpif_reg_data_t)),
    (cy_stc_lvds_gpif_reg_data_t *) cy_lvds_gpif_reg_data
};

/* LVCMOS  PHY configuration output generated by Device Configurator */
cy_stc_lvds_phy_config_t cy_lvcmos_phy0_config = {
    .wideLink = 0u,
#if BUS_WIDTH_16
    .dataBusWidth = CY_LVDS_PHY_LVCMOS_MODE_NUM_LANE_16,
#else
    .dataBusWidth = CY_LVDS_PHY_LVCMOS_MODE_NUM_LANE_8,
#endif
    .modeSelect = CY_LVDS_PHY_MODE_LVCMOS,
    .gearingRatio = CY_LVDS_PHY_GEAR_RATIO_1_1, 
    .clkSrc = CY_LVDS_GPIF_CLOCK_USB2,
    .clkDivider = CY_LVDS_GPIF_CLOCK_DIV_2,
    .interfaceClock = CY_LVDS_PHY_INTERFACE_CLK_80_MHZ,
    .slaveFifoMode       = CY_LVDS_NORMAL_MODE,
    .loopbackModeEn = false,
    .isPutLoopbackMode = false,
    .phyTrainingPattern = PHY_TRAINING_PATTERN_BYTE,
    .linkTrainingPattern = LINK_TRAINING_PATTERN_BYTE,
    .ctrlBusBitMap       = 0x00000297,
    .dataBusDirection = CY_LVDS_PHY_AD_BUS_DIR_INPUT,
    .lvcmosClkMode = CY_LVDS_LVCMOS_CLK_MASTER,
    .lvcmosMasterClkSrc = CY_LVDS_MASTER_CLK_SRC_USB2
};

cy_stc_lvds_config_t cy_lvds0_config =
{
    .phyConfig  = (cy_stc_lvds_phy_config_t *)&cy_lvcmos_phy0_config,
    .gpifConfig = &cy_lvcmos_gpif0_config
};

#if defined(__cplusplus)
}
#endif

#endif /* _CY_GPIF_HEADER_LVCMOS_H_ */

/* End of File */

