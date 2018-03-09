// Copyright (c) 2008-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

typedef void (*PFnPtr)(unsigned char);

struct RampTimes {unsigned x;
  unsigned y;
  unsigned z;
};

#define MASK_1_BIT 0x01
#define MASK_2_BIT 0x03
#define MASK_3_BIT 0x07
#define MASK_4_BIT 0x0F
#define MASK_5_BIT 0x1F
#define MASK_6_BIT 0x3F
#define MASK_7_BIT 0x7F

#define NUM_AUDIO_REGS 74
#define REG_LIST_BUF_LEN 4095 // a PAGE_SIZE piece of memory for reading from sysfs
#define REG_STRING_LEN 5 // enough for "0x", two characters and a null term

static const char *sysfsName = "/sys/devices/platform/twl4030_registers.0/module_audio_voice";
static char regListBuf[REG_LIST_BUF_LEN];
static char regString[REG_STRING_LEN];

// utility functions
int getOptionValue(void);
int populateRegList(void);
int getRegValue(int regAddr);

// register printing functions
void printUnknown(unsigned char val);       // 0x00 - n/a
void printCodecMode(unsigned char val);     // 0x01 - CODEC_MODE
void printOption(unsigned char val);        // 0x02 - OPTION
void printMicbiasCtl(unsigned char val);    // 0x04 - MICBIAS_CTL
void printAnaMicL(unsigned char val);       // 0x05 - ANAMICL
void printAnaMicR(unsigned char val);       // 0x06 - ANAMICR
void printAvadcCtl(unsigned char val);      // 0x07 - AVADC_CTL
void printAdcmicSel(unsigned char val);     // 0x08 - ADCMICSEL
void printDigMixing(unsigned char val);     // 0x09 - DIGMIXING
void printAtxL1Pga(unsigned char val);      // 0x0A - ATXL1PGA
void printAtxR1Pga(unsigned char val);      // 0x0B - ATXR1PGA
void printAvtxL2Pga(unsigned char val);     // 0x0C - AVTXL2PGA
void printAvtxR2Pga(unsigned char val);     // 0x0D - AVTXR2PGA
void printAudioIf(unsigned char val);       // 0x0E - AUDIO_IF
void printVoiceIf(unsigned char val);       // 0x0F - VOICE_IF
void printArxR1Pga(unsigned char val);      // 0x10 - ARXR1PGA
void printArxL1Pga(unsigned char val);      // 0x11 - ARXL1PGA
void printArxR2Pga(unsigned char val);      // 0x12 - ARXR2PGA
void printArxL2Pga(unsigned char val);      // 0x13 - ARXL2PGA 
void printVrxPga(unsigned char val);        // 0x14 - VRXPGA
void printVstPga(unsigned char val);        // 0x15 - VSTPGA
void printVrx2ARxPga(unsigned char val);    // 0x16 - VRX2ARXPGA
void printAvadacCtl(unsigned char val);     // 0x17 - AVADAC_CTL
void printArx2VtxPga(unsigned char val);    // 0x18 - ARX2VTXPGA
void printArxL1ApgaCtl(unsigned char val);  // 0x19 - ARXL1_APGA_CTL
void printArxR1ApgaCtl(unsigned char val);  // 0x1A - ARXR1_APGA_CTL
void printArxL2ApgaCtl(unsigned char val);  // 0x1B - ARXL2_APGA_CTL
void printArxR2ApgaCtl(unsigned char val);  // 0x1C - ARXR2_APGA_CTL
void printAtx2ArxPga(unsigned char val);    // 0x1D - ATX2ARXPGA
void printBtIf(unsigned char val);          // 0x1E - BT_IF
void printBtPga(unsigned char val);         // 0x1F - BTPGA
void printBtStPga(unsigned char val);       // 0x20 - BTSTPGA
void printEarCtl(unsigned char val);        // 0x21 - EAR_CTL
void printHsSel(unsigned char val);         // 0x22 - HS_SEL
void printHsGainSet(unsigned char val);     // 0x23 - HS_GAIN_SET
void printHsPopnSet(unsigned char val);     // 0x24 - HS_POPN_SET
void printPredlCtl(unsigned char val);      // 0x25 - PREDL_CTL
void printPredrCtl(unsigned char val);      // 0x26 - PREDR_CTL
void printPrecklCtl(unsigned char val);     // 0x27 - PRECKL_CTL
void printPreckrCtl(unsigned char val);     // 0x28 - PRECLR_CTL
void printHflCtl(unsigned char val);        // 0x29 - HFL_CTL
void printHfrCtl(unsigned char val);        // 0x2A - HFR_CTL
void printAlcCtl(unsigned char val);        // 0x2B - ALC_CTL
void printAlcSet1(unsigned char val);       // 0x2C - ALC_SET1
void printAlcSet2(unsigned char val);       // 0x2D - ALC_SET2
void printBoostCtl(unsigned char val);      // 0x2E - BOOST_CTL
void printSoftVolCtl(unsigned char val);    // 0x2F - SOFTVOL_CTL
void printDtmfFreqSel(unsigned char val);   // 0x30 - DTMF_FREQSEL
void printDtmfTonExt1H(unsigned char val);  // 0x31 - DTMF_TONEXT1H
void printDtmfTonExt1L(unsigned char val);  // 0x32 - DTMF_TONEXT1L
void printDtmfTonExt2H(unsigned char val);  // 0x33 - DTMF_TONEXT2H
void printDtmfTonExt2L(unsigned char val);  // 0x34 - DTMF_TONEXT2L
void printDtmfTonOff(unsigned char val);    // 0x35 - DTMF_TONOFF
void printDtmfWanOnOff(unsigned char val);  // 0x36 - DTMF_WANONOFF
void printCodecRxScramH(unsigned char val); // 0x37 - CODEC_RX_SCRAMBLE_H
void printCodecRxScramM(unsigned char val); // 0x38 - CODEC_RX_SCRAMBLE_M
void printCodecRxScramL(unsigned char val); // 0x39 - CODEC_RX_SCRAMBLE_L
void printApllCtl(unsigned char val);       // 0x3A - APLL_CTL
void printDtmfCtl(unsigned char val);       // 0x3B - DTMF_CTL
void printDtmfPgaCtl2(unsigned char val);   // 0x3C - DTMF_PGA_CTL2
void printDtmfPgaCtl1(unsigned char val);   // 0x3D - DTMF_PGA_CTL1
void printMiscSet1(unsigned char val);      // 0x3E - MISC_SET_1,
void printPcmBtMux(unsigned char val);      // 0x3F - PCMBTMUX
void printRxPathSel(unsigned char val);     // 0x43 - RX_PATH_SEL
void printVdlApgaCtl(unsigned char val);    // 0x44 - VDL_APGA_CTL
void printVibraCtl(unsigned char val);      // 0x45 - VIBRA_CTL,
void printVibraSet(unsigned char val);      // 0x46 - VIBRA_SET
void printAnamicGain(unsigned char val);    // 0x48 - ANAMIC_GAIN,
void printMiscSet2(unsigned char val);      // 0x49 - MISC_SET_2

static unsigned char verbose=0;


// array of pts. to functions that print out the values of their respective registers
// the names of these regs correspond to the names used in the Gaia datasheet

PFnPtr printRegisterArrayFns[NUM_AUDIO_REGS] = {printUnknown,       // 0x00 - n/a
						printCodecMode,     // 0x01 - CODEC_MODE
						printOption,        // 0x02 - OPTION
						printUnknown,       // 0x03 - n/a
						printMicbiasCtl,    // 0x04 - MICBIAS_CTL
						printAnaMicL,       // 0x05 - ANAMICL
						printAnaMicR,       // 0x06 - ANAMICR
						printAvadcCtl,      // 0x07 - AVADC_CTL
						printAdcmicSel,     // 0x08 - ADCMICSEL
						printDigMixing,     // 0x09 - DIGMIXING
						printAtxL1Pga,      // 0x0A - ATXL1PGA
						printAtxR1Pga,      // 0x0B - ATXR1PGA
						printAvtxL2Pga,     // 0x0C - AVTXL2PGA
						printAvtxR2Pga,     // 0x0D - AVTXR2PGA
						printAudioIf,       // 0x0E - AUDIO_IF
						printVoiceIf,       // 0x0F - VOICE_IF
						printArxR1Pga,      // 0x10 - ARXR1PGA
						printArxL1Pga,      // 0x11 - ARXL1PGA
						printArxR2Pga,      // 0x12 - ARXR2PGA
						printArxL2Pga,      // 0x13 - ARXL2PGA 
						printVrxPga,        // 0x14 - VRXPGA
						printVstPga,        // 0x15 - VSTPGA
						printVrx2ARxPga,    // 0x16 - VRX2ARXPGA
						printAvadacCtl,     // 0x17 - AVADAC_CTL
						printArx2VtxPga,    // 0x18 - ARX2VTXPGA
						printArxL1ApgaCtl,  // 0x19 - ARXL1_APGA_CTL
						printArxR1ApgaCtl,  // 0x1A - ARXR1_APGA_CTL
						printArxL2ApgaCtl,  // 0x1B - ARXL2_APGA_CTL
						printArxR2ApgaCtl,  // 0x1C - ARXR2_APGA_CTL
						printAtx2ArxPga,    // 0x1D - ATX2ARXPGA
						printBtIf,          // 0x1E - BT_IF
						printBtPga,         // 0x1F - BTPGA
						printBtStPga,       // 0x20 - BTSTPGA
						printEarCtl,        // 0x21 - EAR_CTL
						printHsSel,         // 0x22 - HS_SEL
						printHsGainSet,     // 0x23 - HS_GAIN_SET
						printHsPopnSet,     // 0x24 - HS_POPN_SET
						printPredlCtl,      // 0x25 - PREDL_CTL
						printPredrCtl,      // 0x26 - PREDR_CTL
						printPrecklCtl,     // 0x27 - PRECKL_CTL
						printPreckrCtl,     // 0x28 - PRECLR_CTL
						printHflCtl,        // 0x29 - HFL_CTL
						printHfrCtl,        // 0x2A - HFR_CTL
						printAlcCtl,        // 0x2B - ALC_CTL
						printAlcSet1,       // 0x2C - ALC_SET1
						printAlcSet2,       // 0x2D - ALC_SET2
						printBoostCtl,      // 0x2E - BOOST_CTL
						printSoftVolCtl,    // 0x2F - SOFTVOL_CTL
						printDtmfFreqSel,   // 0x30 - DTMF_FREQSEL
						printDtmfTonExt1H,  // 0x31 - DTMF_TONEXT1H
						printDtmfTonExt1L,  // 0x32 - DTMF_TONEXT1L
						printDtmfTonExt2H,  // 0x33 - DTMF_TONEXT2H
						printDtmfTonExt2L,  // 0x34 - DTMF_TONEXT2L
						printDtmfTonOff,    // 0x35 - DTMF_TONOFF
						printDtmfWanOnOff,  // 0x36 - DTMF_WANONOFF
						printCodecRxScramH, // 0x37 - CODEC_RX_SCRAMBLE_H
						printCodecRxScramM, // 0x38 - CODEC_RX_SCRAMBLE_M
						printCodecRxScramL, // 0x39 - CODEC_RX_SCRAMBLE_L
						printApllCtl,       // 0x3A - APLL_CTL
						printDtmfCtl,       // 0x3B - DTMF_CTL
						printDtmfPgaCtl2,   // 0x3C - DTMF_PGA_CTL2
						printDtmfPgaCtl1,   // 0x3D - DTMF_PGA_CTL1
						printMiscSet1,      // 0x3E - MISC_SET_1,
						printPcmBtMux,      // 0x3F - PCMBTMUX
						printUnknown,       // 0x40 - n/a
						printUnknown,       // 0x41 - n/a
						printUnknown,       // 0x42 - n/a
						printRxPathSel,     // 0x43 - RX_PATH_SEL
						printVdlApgaCtl,    // 0x44 - VDL_APGA_CTL
						printVibraCtl,      // 0x45 - VIBRA_CTL,
						printVibraSet,      // 0x46 - VIBRA_SET
						printUnknown,       // 0x47 - n/a
						printAnamicGain,    // 0x48 - ANAMIC_GAIN,
						printMiscSet2       // 0x49 - MISC_SET_2
};


static const char* audioSamplingFrequencies[16] = {"8", "11.025", "12", "0",
						   "16", "22.05", "24", "0",
						   "32", "44.1",  "48", "0",
						   "0",    "0",  "96", "0"};

static const struct RampTimes rampTable[8] = {{27, 20, 14},
					      {55, 40, 27},
					      {109, 81, 55},
					      {218, 161, 109},
					      {437, 323, 218},
					      {874, 645, 437},
					      {1748, 1291, 874},
					      {3495, 2581, 1748}};

#define MAX_DTMF_TONES 21
static const char* dtmfFrequencies[MAX_DTMF_TONES] = {"1: Tone 1 = 1209 Hz, Tone 2 = 697 Hz", //0x0
						     "2: Tone 1 = 1336 Hz, Tone 2 = 697 Hz", //0x1
						     "3: Tone 1 = 1477 Hz, Tone 2 = 697 Hz", //0x2
						     "A: Tone 1 = 1633 Hz, Tone 2 = 697 Hz", //0x3
						     "4: Tone 1 = 1209 Hz, Tone 2 = 770 Hz", //0x4
						     "5: Tone 1 = 1336 Hz, Tone 2 = 770 Hz", //0x5
						     "6: Tone 1 = 1477 Hz, Tone 2 = 770 Hz", //0x6
						     "B: Tone 1 = 1633 Hz, Tone 2 = 770 Hz", //0x7
						     "7: Tone 1 = 1209 Hz, Tone 2 = 770 Hz", //0x8
						     "8: Tone 1 = 1336 Hz, Tone 2 = 852 Hz", //0x9
						     "9: Tone 1 = 1477 Hz, Tone 2 = 852 Hz", //0xA
						     "C: Tone 1 = 1633 Hz, Tone 2 = 852 Hz", //0xB
						     "*: Tone 1 = 1209 Hz, Tone 2 = 941 Hz", //0xC
						     "0: Tone 1 = 1336 Hz, Tone 2 = 941 Hz", //0xD
						     "#: Tone 1 = 1477 Hz, Tone 2 = 941 Hz", //0xE
						     "D: Tone 1 = 1633 Hz, Tone 2 = 941 Hz", //0xF
						     "Single tone 1 = 400 Hz",               //0x10
						     "Single tone 1 = 425 Hz",               //0x11
						     "Single tone 1 = 2 kHz",                //0x12
						     "Single tone 1 = 2.67 kHz",             //0x13
						     "Tone 1: extended, Tone 2: extended"    //0x14
};

static unsigned int dtmfOnTimes[16] = {   0,   50,   100,  150,
				        200,  250,   300,  400,
				        450,  500,   750, 1000,
					1250, 1500, 1750, 2000};

#define MAX_DTMF_OFF_TIMES 15
static unsigned int dtmfOffTimes[MAX_DTMF_OFF_TIMES] = {   0,   50,  100,  200,
					                 250,  300,  400,  500,
					                 750, 1000, 1500, 2000,
					                2500, 3000, 3500};

#define MAX_WOBBLE_TIMES 12
static float wobbleTimes[MAX_WOBBLE_TIMES] = {  0,  31.25, 50, 100,
					      150, 200,   250, 300,
					      350, 400,   450, 500};

static const char* rxr1PgaSelect[4] = {"SDRR1 to RXR1 PGA",
				       "SDRM1 (mix SDRL1 and SDRR1) to RXR1 PGA",
				       "SDRR2 to RXR1 PGA",
				       "SDRM2 (mix SDRL2 and SDRR2) to RXR1 PGA"};

static const char* rxl1PgaSelect[4] = {"SDRL1 to RXL1 PGA",
				       "SDRM1 (mix SDRL1 and SDRR1) to RXL1 PGA",
				       "SDRL2 to RXL1 PGA",
				       "SDRM2 (mix SDRL2 and SDRR2) to RXL1 PGA"};
				       
				       

/**
 * @brief prints out msg that register addr. is invalid (not described in spec)
 *
 **/
void printUnknown(unsigned char val)
{
  printf("value unknown, reg address is not described in Gaia spec\n");
}


/**
 * @brief print values in register CODEC_MODE at 0x01
 *
 **/

void printCodecMode(unsigned char val)
{

  unsigned char curr;

  printf("CODEC_MODE reg at addr 0x01: value is 0x%x\n", val);

  if (verbose) {

    curr=val & MASK_1_BIT;
    if (curr) {
      printf("\tchip in option 1: 2 RX and TX stereo audio paths\n");
    } else {
      printf("\tchip in option 2: voice uplink (stereo) and downlink, (mono) + 1 RX and TX stereo paths\n");
    }

    val = val >> 1;
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tcodec is on\n");
    } else {
      printf("\tcodec is off\n");
    }

    val = val >> 2;
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tsampling voice at 16 khz\n");
    } else {
      printf("\tsampling voice at 8 khz\n");
    }

    val = val >> 1;
    curr = val & MASK_4_BIT;
    printf("\taudio mode sampling frequency is %s kHz\n", audioSamplingFrequencies[curr]);
  }

}

/**
 * @brief print values in register OPTION at 0x02
 *
 **/
void printOption(unsigned char val)
{
  unsigned char curr;
  printf("OPTION reg at addr 0x02: value is 0x%x\n", val);
  int option;

  if (verbose) {
    // need to know whether we're in option 1 or option 2 to interpret
    // some of the values
    option=getOptionValue();

    // bit 0
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tAudio TX left 1 enabled\n");
    } else {
      printf("\tAudio TX left 1 disabled\n");
    }
    val = val >> 1;

    // bit 1
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tAudio TX right 1 enabled\n");
    } else {
      printf("\tAudio TX right 1 disabled\n");
    }
    val = val >> 1; 

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      if (option == 1) {
	printf("\tAudio TX left 2 enabled\n");
      } else {
	printf("\tvoice TX left enabled\n");
      }
    } else {
      if (option == 1) {
	printf("\tAudio TX left 2 disabled\n");
      } else {
	printf("\tvoice TX left disabled\n");
      }
    }
    val = val >> 1;

    // bit 3
    curr = val & MASK_1_BIT;
    if (curr) {
      if (option == 1) {
	printf("\tAudio TX right 2 enabled\n");
      } else {
	printf("\tvoice TX right enabled\n");
      }
    } else {
      if (option == 1) {
	printf("\tAudio TX right 2 disabled\n");
      } else {
	printf("\tvoice TX right disabled\n");
      }
    }
    val = val >> 1;

    // bit 4
    curr = val & MASK_1_BIT;
    if (curr) {
      if (option == 1) {
	printf("\tAudio RX left 1 enabled\n");
      } else {
	printf("\tvoice RX enabled\n");
      }
    } else {
      if (option == 1) {
	printf("\tAudio RX left 1 disabled\n");
      } else {
	printf("\tvoice RX disabled\n");
      }
    }
    val = val >> 1;

    // bit 5
    curr = val & MASK_1_BIT;
    if (curr) {
      if (option == 1) {
	printf("\tAudio RX right 1 enabled\n");
      }
    } else {
      if (option == 1) {
	printf("\tAudio RX right 1 disabled\n");
      }
    }
    val = val >> 1;

    // bit 6
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tAudio RX left 2 enabled\n");
    } else {
      printf("\tAudio RX left 2 disabled\n");
    }
    val = val >> 1;

    // bit 7
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tAudio RX right 2 enabled\n");
    } else {
      printf("\tAudio RX right 2 disabled\n");
    }
    val = val >> 1;
  }
}

/**
 * @brief print values in register MICBIAS_CTL at 0x04
 *
 **/
void printMicbiasCtl(unsigned char val)
{
  unsigned char curr;

  printf("MICBIAS_CTL reg at addr 0x04: value is 0x%x\n", val);

  if (verbose) {
    // bit 0 
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tmain microphone/digital microphone 0 bias power ctl enabled\n");
    } else {
      printf("\tmain microphone/digital microphone 0 bias power ctl disabled\n");
    }
    val = val >> 1;

    // bit 1
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tsubmicrophone/digital microphone 1 bias power ctl enabled\n");
    } else {
      printf("\tsubmicrophone/digital microphone 1 bias power ctl disabled\n");
    }
    val = val >> 1;

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\theadset microphone bias power ctl enabled\n");
    } else {
      printf("\theadset microphone bias power ctl disabled\n");
    }
    val = val >> 3; // skip bits 3 & 4, reserved

    // bit 5
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tMICBIAS1 controls digital microphone bias\n");
    } else {
      printf("\tMICBIAS1 controls analog microphone bias\n");
    }
    val = val >> 1;

    // bit 6
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tMICBIAS2 controls digital microphone bias\n");
    } else {
      printf("\tMICBIAS2 controls analog microphone bias\n");
    }
    val = val >> 1;

    // bit 7 has no meaning
  }
}

/**
 * @brief print values in register ANAMICL at 0x05
 *
 **/
void printAnaMicL(unsigned char val)
{
  unsigned char curr;

  printf("ANAMICL reg at addr 0x05: value is 0x%x\n", val);

  if (verbose) {

    // bit 0
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tmain microphone input enabled\n");
    } else {
      printf("\tmain microphone input disabled\n");
    }
    val = val >> 1;

    // bit 1
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\theadset microphone enabled\n");
    } else {
      printf("\theadset microphone disabled\n");
    }
    val = val >> 1;

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tAUXL input enabled\n");
    } else {
      printf("\tAUXL input disabled\n");
    }
    val = val >> 1;

    // bit 3
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tcarkit microphone input enabled\n");
    } else {
      printf("\tcarkit microphone input disabled\n");
    }
    val = val >> 1;

    // bit 4
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tleft microphone amplifier in application mode (on)\n");
    } else {
      printf("\tleft microphone amplifier in power-down mode\n");
    }
    val = val >> 1;

    // bits 5&6 
    curr = val & MASK_2_BIT;
    printf("\tpaths selected for the offset cancellation:\n");
    switch(curr) {
    case 0:
      printf("\t\taudio RX left 1 and right 1 selected\n");
      break;
    case 1:
      printf("\t\taudio RX left 2 and right 2 selected\n");
      break;
    case 2:
      printf("\t\tvoice RX selected\n");
      break;
    case 3:
      printf("\t\tall channels selected\n");
      break;
    default:
      printf("\t\ttoo weird\n");
    }    
    val = val >> 2;

    // bit 7
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\toffset cancellation in process\n");
    } else {
      printf("\toffset cancellation either complete or not yet initiated\n");
    }
  }
}

/**
 * @brief print values in register ANAMICR at 0x06
 *
 **/
void printAnaMicR(unsigned char val)
{
  unsigned char curr;

  printf("ANAMICR reg at addr 0x06: value is 0x%x\n", val);

  if (verbose) {

    // bit 0
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tsubmicrophone input enabled\n");
    } else {
      printf("\tsubmicrophone input disabled\n");
    }
    val = val >> 2; // skip bit 1;

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tAUXR input enabled\n");
    } else {
      printf("\tAUXR input disabled\n");
    }
    val = val >> 2;  // skip bit 3

    // bit 4
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tright microphone amplifier power control in app mode\n");
    } else {
      printf("\tright microphone amplifier power control in power-down mode\n");
    }
    // bits 5-7 are reserved
  }
}

/**
 * @brief print values in register AVADC_CTL at 0x07
 *
 **/
void printAvadcCtl(unsigned char val)
{
  unsigned char curr;
  int option;

  printf("AVADC_CTL reg at addr 0x07: value is 0x%x\n", val);

  if (verbose) {
    option = getOptionValue();
  
    // skip bit 0
    val = val >> 1;

    // bit 1
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tADCR power enabled\n");
    } else {
      printf("\tADCR power disabled\n");
    }
    val = val >> 1;

    // bit 2
    curr = val & MASK_1_BIT;
    if (option == 2) {
      // this bit only has meaning in option 2
      if (curr) {
	printf("\taudio has higher priority on the TX channel\n");
      } else {
	printf("\tvoice has higher priority on the TX channel\n");
      }
    }
    val = val >> 1;

    // bit 3
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tADCL power in application mode (on)\n");
    } else {
      printf("\tADCL power in power-down mode\n");
    }

    // bits 4-7 are reserved
  }

}

/**
 * @brief print values in register ADCMICSEL at 0x08
 *
 **/
void printAdcmicSel(unsigned char val)
{
  unsigned char curr;

  printf("ADCMICSEL reg at addr 0x08: value is 0x%x\n", val);
  
  if (verbose) {

    // bit 0
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tdigital microphone 0 is selected for input to TX1\n");
    } else {
      printf("\tADC input. ADCL routed to TXL1, ADCR routed to TXR1\n");
    }
    val = val >> 1;

    // bit 1
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tdigital microphone 0 power in application mode\n");
    } else {
      printf("\tdigital microphone 0 in power-down mode\n");
    }
    val = val >> 1;

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tdigital microphone 1 is selected for input to TX2\n");
    } else {
      printf("\tADC input. ADCL routed to TXL2, ADCR routed to TXR2\n");
    }
    val = val >> 1;

    // bit 3
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tdigital microphone 1 power in application mode\n");
    } else {
      printf("\tdigital microphone 1 in power-down mode\n");
    }

    // bits 4-7 reserved
  }
}

/**
 * @brief print values in register DIGMIXING at 0x09
 *
 **/
void printDigMixing(unsigned char val)
{
  unsigned char curr;

  printf("DIGMIXING reg at addr 0x09: value is 0x%x\n", val);

  if (verbose) {
    // bits 0 and 1 are reserved
    val = val >> 2;

    // bits 2&3
    curr = val & MASK_2_BIT;
    switch (curr) {
    case 0: 
      printf("\tvoice TX digital mixing off\n");
      break;
    case 1:
      printf("\taudio RX added to the voice TX left 2\n");
      break;
    case 2:
      printf("\taudio RX added to the voice TX right 2\n");
      break;
    case 3:
      printf("\taudio RX added to both voice TX left 2 and voice TX right 2\n");
      break;
    default:
      printf("too weird\n");
    }
    val = val >> 2;

    // bits 4&5
    curr = val & MASK_2_BIT;
    switch (curr) {
    case 0: 
      printf("\taudio RX digital mixing (VRX to ARX2) off\n");
      break;
    case 1:
      printf("\tvoice RX added to the audio RX left 2\n");
      break;
    case 2:
      printf("\tvoice RX added to the audio RX right 2\n");
      break;
    case 3:
      printf("\tvoice RX added to both audio RX left 2 and audio RX right 2\n");
      break;
    default:
      printf("too weird\n");
    }
    val = val >> 2;

    // bits 6&7
    curr = val & MASK_2_BIT;
    switch (curr) {
    case 0: 
      printf("\taudio RX digital mixing (VRX to ARX1) off\n");
      break;
    case 1:
      printf("\tvoice RX added to the audio RX left 1\n");
      break;
    case 2:
      printf("\tvoice RX added to the audio RX right 1\n");
      break;
    case 3:
      printf("\tvoice RX added to both audio RX left 1 and audio RX right 1\n");
      break;
    default:
      printf("too weird\n");
    }
  }
}

/**
 * @brief print values in register ATXL1PGA at 0x0A
 *
 **/
void printAtxL1Pga(unsigned char val)
{
  unsigned char curr;

  printf("ATXL1PGA reg at addr 0x0A: value is 0x%x\n", val);
  
  if (verbose) {
    curr = val & MASK_4_BIT;
    printf("\tTXL1 digital gain is %d dB\n", curr);
  }
}

/**
 * @brief print values in register ATXR1PGA at 0x0B
 *
 **/
void printAtxR1Pga(unsigned char val)
{
  unsigned char curr;

  printf("ATXR1PGA reg at addr 0x0B: value is 0x%x\n", val);

  if (verbose) {
    curr = val & MASK_4_BIT;
    printf("\tTXR1 digital gain is %d dB\n", curr);
  }
}

/**
 * @brief print values in register AVTXL2PGA at 0x0C
 *
 **/
void printAvtxL2Pga(unsigned char val)
{
  unsigned char curr;

  printf("AVTXL2PGA reg at addr 0x0C: value is 0x%x\n", val);
  
  if (verbose) {
    curr = val & MASK_4_BIT;
    printf("\tTXL2 digital gain is %d dB\n", curr);
  }
}

/**
 * @brief print values in register AVTXR2PGA at 0x0D
 *
 **/
void printAvtxR2Pga(unsigned char val)
{
  unsigned char curr;

  printf("AVTXR2PGA reg at addr 0x0D: value is 0x%x\n", val);

  if (verbose) {
    curr = val & MASK_4_BIT;
    printf("\tTXR2 digital gain is %d dB\n", curr);
  }
}

/**
 * @brief print values in register AUDIO_IF at 0x0E
 *
 **/
void printAudioIf(unsigned char val)
{
  unsigned char curr;
  int option;

  printf("AUDIO_IF reg at addr 0x0E: value is 0x%x\n", val);

  if (verbose) {
    option = getOptionValue();

    // bit 0
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\taudio serial interface in application mode (on)\n");
    } else {
      printf("\taudio serial interface in off mode DIN/DOUT/SYN/CLK=L\n");
    }
    val = val >> 1;

    // bit 1
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tControl for 256FS CLK output enabled\n");
    } else {
      printf("\tControl for 256FS CLK output disabled\n");
    }
    val = val >> 1;

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\thigh impedance set for audio interface output pins\n");
    } else {
      printf("\thigh impedance for audio interface output pins in application mode (off)\n");
    }
    val = val >> 1;

    // bits 3 & 4
    curr = val & MASK_2_BIT;
    switch (curr) {
    case 0: 
      printf("\taudio interface data format is codec mode\n");
      break;
    case 1:
      printf("\taudio interface data format is left-justified mode\n");
      break;
    case 2:
      printf("\taudio interface data format is right-justified mode\n");
      break;
    case 3:
      if (option == 1) {
	printf("\taudio interface data format is TDM mode\n");
      } else {
	printf("\twarning: this data format only available in option 1\n");
      }
      break;
    default:
      printf("too weird\n");
    }
    val = val >> 2;

    // bits 5 & 6
    curr = val & MASK_2_BIT;
    switch (curr) {
    case 0: 
      printf("\t16-bit sample length(slot), 16-bit word width\n");
      break;
    case 1:
      printf("\twarning: this word width reserved\n");
      break;
    case 2:
      printf("\t32-bit sample length(slot), 16-bit word width\n");
      break;
    case 3:
      printf("\t32-bit sample length(slot), 24-bit word width\n");
      break;
    default:
      printf("too weird\n");
    }
    val = val >> 2;

    // bit 7
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tTDM/CODEC in slave mode\n");
    } else {
      printf("\tTDM/CODEC in master mode\n");
    }
  }
}

/**
 * @brief print values in register VOICE_IF at 0x0F
 *
 **/
void printVoiceIf(unsigned char val)
{
  unsigned char curr;

  printf("VOICE_IF reg at addr 0x0F: value is 0x%x\n", val);
  
  if (verbose) {

    // bit 0
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tvoice interface power in application mode\n");
    } else {
      printf("\tvoice interface power mode VDR and PCM_VDX=L (off?)\n");
    }
    val = val >> 1;

    // bit 1
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tvoice and BT interface power enabled (stereo TX)\n");
    } else {
      printf("\tvoice and BT interface power diabled (mono TX)\n");
    }
    val = val >> 1;

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tVIF_TRI_EN set\n");
      printf("\tvoice interface output pins at high impedance\n");
    } else {
      printf("\tvoice interface output pins in app mode (low impedance)\n");
    }
    val = val >> 1;

    // bit 3
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tvoice and BT interface data format in mode 2\n");
      printf("\twriting on PCM_VCK falling edge\n");
    } else {
      printf("\tvoice and BT interface data format in mode 1\n");
      printf("\twriting on PCM_VCK rising edge\n");
    }
    val = val >> 1;

    // bit 4
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tPCM_VDX and PCM_VDR I/O swapped\n");
    } else {
      printf("\tPCM_VDX and PCM_VDR I/O not swapped\n");
    }
    val = val >> 1;

    // bit 5
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tvoice output in application mode (enabled)\n");
    } else {
      printf("\tvoice output in off mode, PCM_VDX = L\n");
    }
    val = val >> 1;

    // bit 6
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tvoice input in application mode (enabled)\n");
    } else {
      printf("\tvoice input in off mode, VDR = L\n");
    }
    val = val >> 1;

    // bit 7
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tvoice PCM slave mode\n");
    } else {
      printf("\tvoice PCM master mode\n");
    }
  }
}

/**
 * @brief print values in register ARXR1PGA at 0x10
 *
 **/
void printArxR1Pga(unsigned char val)
{
  int curr;

  printf("ARXR1PGA reg at addr 0x10: value is 0x%x\n", val);

  if (verbose) {
  
    //bits 0-5
    curr = val & MASK_6_BIT;
    if (curr == 0) {
      printf("\taudio RXR1 fine gain is muted\n");
    } else {
      printf("\taudio RXR1 fine gain is %d dB\n", curr-63);
    }
    val = val >> 6;

    // bits 6&7
    curr = val & MASK_2_BIT;
    switch (curr) {
    case 0: 
      printf("\taudio RXR1 coarse gain is 0 dB\n");
      break;
    case 1:
      printf("\taudio RXR1 coarse gain is 6 dB\n");
      break;
    case 2:
      printf("\taudio RXR1 coarse gain is 12 dB\n");
      break;
    case 3:
      printf("\taudio RXR1 coarse gain is 12 dB\n");
      break;
    default:
      printf("too weird\n");
    }
  }
}

/**
 * @brief print values in register ARXL1PGA at 0x11
 *
 **/
void printArxL1Pga(unsigned char val)
{
  int curr;

  printf("ARXL1PGA reg at addr 0x11: value is 0x%x\n", val);

  if (verbose) {
  
    //bits 0-5
    curr = val & MASK_6_BIT;
    if (curr == 0) {
      printf("\taudio RXL1 fine gain is muted\n");
    } else {
      printf("\taudio RXL1 fine gain is %d dB\n", curr-63);
    }
    val = val >> 6;

    // bits 6&7
    curr = val & MASK_2_BIT;
    switch (curr) {
    case 0: 
      printf("\taudio RXL1 coarse gain is 0 dB\n");
      break;
    case 1:
      printf("\taudio RXL1 coarse gain is 6 dB\n");
      break;
    case 2:
      printf("\taudio RXL1 coarse gain is 12 dB\n");
      break;
    case 3:
      printf("\taudio RXL1 coarse gain is 12 dB\n");
      break;
    default:
      printf("too weird\n");
    }
  }
}

/**
 * @brief print values in register ARXR2PGA at 0x12
 *
 **/
void printArxR2Pga(unsigned char val)
{
  int curr; 

  printf("ARXR2PGA reg at addr 0x12: value is 0x%x\n", val);

  if (verbose) {

    //bits 0-5
    curr = val & MASK_6_BIT;
    if (curr == 0) {
      printf("\taudio RXR2 fine gain is muted\n");
    } else {
      printf("\taudio RXR2 fine gain is %d dB\n", curr-63);
    }
    val = val >> 6;

    // bits 6&7
    curr = val & MASK_2_BIT;
    switch (curr) {
    case 0: 
      printf("\taudio RXR2 coarse gain is 0 dB\n");
      break;
    case 1:
      printf("\taudio RXR2 coarse gain is 6 dB\n");
      break;
    case 2:
      printf("\taudio RXR2 coarse gain is 12 dB\n");
      break;
    case 3:
      printf("\taudio RXR2 coarse gain is 12 dB\n");
      break;
    default:
      printf("\ttoo weird\n");
    }
  }
}

/**
 * @brief print values in register ARXL2PGA at 0x13
 *
 **/
void printArxL2Pga(unsigned char val)
{
  int curr;

  printf("ARXL2PGA reg at addr 0x13: value is 0x%x\n", val);

  if (verbose) {

    //bits 0-5
    curr = val & MASK_6_BIT;
    if (curr == 0) {
      printf("\taudio RXL2 fine gain is muted\n");
    } else {
      printf("\taudio RXL2 fine gain is %d dB\n", curr-63);
    }
    val = val >> 6;

    // bits 6&7
    curr = val & MASK_2_BIT;
    switch (curr) {
    case 0: 
      printf("\taudio RXL2 coarse gain is 0 dB\n");
      break;
    case 1:
      printf("\taudio RXL2 coarse gain is 6 dB\n");
      break;
    case 2:
      printf("\taudio RXL2 coarse gain is 12 dB\n");
      break;
    case 3:
      printf("\taudio RXL2 coarse gain is 12 dB\n");
      break;
    default:
      printf("too weird\n");
    }
  }
}

/**
 * @brief print values in register VRXPGA at 0x14
 *
 **/
void printVrxPga(unsigned char val)
{
  int curr;

  printf("VRXPGA reg at addr 0x14: value is 0x%x\n", val);

  if (verbose) {
    //bits 0-5
    curr = val & MASK_6_BIT;
    if (curr == 0) {
      printf("\tvoice downlink gain is muted\n");
    } else {
      printf("\tvoice downlink gain is %d dB\n", curr-37);
    }
    // bits 6 & 7 are reserved
  }
}

/**
 * @brief print values in register VSTPGA at 0x15
 *
 **/
void printVstPga(unsigned char val)
{
  int curr;

  printf("VSTPGA reg at addr 0x15: value is 0x%x\n", val);

  if (verbose) {
    //bits 0-5
    curr = val & MASK_6_BIT;
    if (curr == 0) {
      printf("\tvoice side tone is muted\n");
    } else {
      printf("\tvoice side tone gain is %d dB\n", curr-51);
    }
  }
}

/**
 * @brief print values in register VRX2ARXPGA at 0x16
 *
 **/
void printVrx2ARxPga(unsigned char val)
{
  int curr;

  printf("VRX2ARXPGA reg at addr 0x16: value is 0x%x\n", val);

  if (verbose) {

    //bits 0-4
    curr = val & MASK_5_BIT;
    if (curr == 0) {
      printf("\tgain of voice RX to audio RX mixing is muted\n");
    } else {
      printf("\tgain of voice RX to audio RX mixing is %d dB\n", curr-25);
    }
  }
}

/**
 * @brief print values in register AVADAC_CTL at 0x17
 *
 **/
void printAvadacCtl(unsigned char val)
{
  unsigned char curr;

  printf("AVADAC_CTL reg at addr 0x17: value is 0x%x\n", val);

  if (verbose) {

    // bit 0
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tAudio DACR1 power enabled\n");
    } else {
      printf("\tAudio DACR1 power disabled\n");
    }
    val = val >> 1;

    // bit 1
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tAudio DACL1 power enabled\n");
    } else {
      printf("\tAudio DACL1 power disabled\n");
    }
    val = val >> 1;

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tAudio DACR2 power enabled\n");
    } else {
      printf("\tAudio DACR2 power disabled\n");
    }
    val = val >> 1;

    // bit 3
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tAudio DACL2 power enabled\n");
    } else {
      printf("\tAudio DACL2 power disabled\n");
    }
    val = val >> 1;

    // bit 4
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tVoice DAC power enabled\n");
    } else {
      printf("\tVoice DAC power disabled\n");
    }
    // bits 5-7 reserved
  }
}

/**
 * @brief print values in register ARX2VTXPGA  at 0x18
 *
 **/
void printArx2VtxPga(unsigned char val)
{
  int curr;

  printf("ARX2VTXPGA reg at addr 0x18: value is 0x%x\n", val);

  if (verbose) {

    //bits 0-5
    curr = val & MASK_6_BIT;
    if (curr == 0) {
      printf("\tgain of audio RX to voice TX mixing is muted\n");
    } else {
      printf("\tgain of audio RX to voice TX mixing is %d dB\n", curr-63);
    }
    // bits 6&7 reserved
  }
}


/**
 * @brief print values in register ARXL1_APGA_CTL at 0x19
 *
 **/
void printArxL1ApgaCtl(unsigned char val)
{
  int curr;

  printf("ARXL1_APGA_CTL reg at addr 0x19: value is 0x%x\n", val);

  if (verbose) {

    // bit 0
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tanalog RXL1 PGA power control in app mode (on)\n");
    } else {
      printf("\tanalog RXL1 PGA power control in power-down mode\n");
    }
    val = val >> 1;

    // bit 1
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tdigital to analog RXL1 path enabled\n");
    } else {
      printf("\tdigital to analog RXL1 path disabled\n");
    }
    val = val >> 1;

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tFM loop path for RXL1 enabled\n");
    } else {
      printf("\tFM loop path for RXL1 disabled\n");
    }
    val = val >> 1;

    // bits 3-7 
    curr = val & MASK_5_BIT;
    printf("\tanalog programmable gain for RXL1 is %2d dB\n", 12-(2*curr));
  }
}

/**
 * @brief print values in register ARXR1_APGA_CTL at 0x1A
 *
 **/
void printArxR1ApgaCtl(unsigned char val)
{
  int curr;

  printf("ARXR1_APGA_CTL reg at addr 0x1A: value is 0x%x\n", val);

  if (verbose) {

    // bit 0
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tanalog RXR1 PGA power control in app mode (on)\n");
    } else {
      printf("\tanalog RXR1 PGA power control in power-down mode\n");
    }
    val = val >> 1;

    // bit 1
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tdigital to analog RXR1 path enabled\n");
    } else {
      printf("\tdigital to analog RXR1 path disabled\n");
    }
    val = val >> 1;

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tFM loop path for RXR1 enabled\n");
    } else {
      printf("\tFM loop path for RXR1 disabled\n");
    }
    val = val >> 1;

    // bits 3-7 
    curr = val & MASK_5_BIT;
    printf("\tanalog programmable gain for RXR1 is %2d dB\n", 12-(2*curr));
  }

}

/**
 * @brief print values in register ARXL2_APGA_CTL at 0x1B
 *
 **/
void printArxL2ApgaCtl(unsigned char val)
{
  int curr;

  printf("ARXL2_APGA_CTL reg at addr 0x1B: value is 0x%x\n", val);

  if (verbose) {

    // bit 0
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tanalog RXL2 PGA power control in app mode (on)\n");
    } else {
      printf("\tanalog RXL2 PGA power control in power-down mode\n");
    }
    val = val >> 1;

    // bit 1
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tdigital to analog RXL2 path enabled\n");
    } else {
      printf("\tdigital to analog RXL2 path disabled\n");
    }
    val = val >> 1;

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tFM loop path for RXL2 enabled\n");
    } else {
      printf("\tFM loop path for RXL2 disabled\n");
    }
    val = val >> 1;

    // bits 3-7 
    curr = val & MASK_5_BIT;
    printf("\tanalog programmable gain for RXL2 is %2d dB\n", 12-(2*curr));
  }
}

/**
 * @brief print values in register  ARXR2_APGA_CTL at 0x1C
 *
 **/
void printArxR2ApgaCtl(unsigned char val)
{
  int curr;

  printf("ARXR2_APGA_CTL reg at addr 0x1C: value is 0x%x\n", val);

  if (verbose) {
    // bit 0
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tanalog RXR2 PGA power control in app mode (on)\n");
    } else {
      printf("\tanalog RXR2 PGA power control in power-down mode\n");
    }
    val = val >> 1;

    // bit 1
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tdigital to analog RXR2 path enabled\n");
    } else {
      printf("\tdigital to analog RXR2 path disabled\n");
    }
    val = val >> 1;

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tFM loop path for RXR2 enabled\n");
    } else {
      printf("\tFM loop path for RXR2 disabled\n");
    }
    val = val >> 1;

    // bits 3-7 
    curr = val & MASK_5_BIT;
    printf("\tanalog programmable gain for RXR2 is %2d dB\n", 12-(2*curr));
  }
}


/**
 * @brief print values in register ATX2ARXPGA at 0x1D
 *
 **/
void printAtx2ArxPga(unsigned char val)
{
  int curr; 

  printf("ATX2ARXPGA reg at addr 0x1D: value is 0x%x\n", val);

  if (verbose) {
  
    // bits 0-2
    curr = val & MASK_3_BIT;
    if (0 == curr) {
      printf("\tLoop TX to RX digital gain (right) is muted\n");
    } else if (curr <= 3) {
      printf("\tLoopTX to RX digital gain (right) is -24 dB\n");
    } else {
      printf("\tLoop TX to RX digital gain (right) is %d dB\n", (curr-7)*6);
    }
    val = val >> 3;

    // bits 3-5
    curr = val & MASK_3_BIT;
    if (0 == curr) {
      printf("\tLoop TX to RX digital gain (left) is muted\n");
    } else if (curr <= 3) {
      printf("\tLoopTX to RX digital gain (left) is -24 dB\n");
    } else {
      printf("\tLoop TX to RX digital gain (left) is %d dB\n", (curr-7)*6);
    }

    // bits 6&7 reserved
  }

}


/**
 * @brief print values in register BT_IF at 0x1E
 *
 **/
void printBtIf(unsigned char val)
{
  unsigned char curr;

  printf("BT_IF reg at addr 0x1E: value is 0x%x\n", val);

  if (verbose) {

    // bit 0
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tBluetooth and voice interface in application mode (on)\n");
    } else {
      printf("\tbluetooth and voice interface in mode BTDIN/BTDOUT=L\n");
    }
    val = val >> 2; // skip bit 1

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tBT_TRI_EN set\n");
      printf("\tBluetooth interface output pins(BT_PCM_VDR and BT_PCM_VDX\n");
      printf("\tin high impedance\n");
      printf("\tPCM_VCK and and PCM_VFS will be in high impedanc if VIF_TRI_EN\n");
      printf("\tis also set\n");
    } else {
      printf("\tBluetooth interface pins in app mode (low impedance)\n");
    }
    val = val >> 2; // skip bit 3

    // bit 4
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tbt_pcm_vdr and bt_pcm_vdx I/O swapped\n");
    } else {
      printf("\tbt_pcm_vdr and bt_pcm_vdx I/O not swapped\n");
    }
    val = val >> 1;

    // bit 5
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tBluetooth output in application mode (enabled)\n");
    } else {
      printf("\tBluetooth output in off mode BT_PCM_VDX = L\n");
    }
    val = val >> 1;

    // bit 6
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tBluetooth input in application mode (enabled)\n");
    } else {
      printf("\tBluetooth input in off mode BTDIN = L\n");
    }

    // bit 7 reserved
  }

}

/**
 * @brief print values in register BTPGA at 0x1F
 *
 **/
void printBtPga(unsigned char val)
{
  int curr;

  printf("BTPGA reg at addr 0x1F: value is 0x%x\n", val);

  if (verbose) {

    // bits 0-3
    curr = val & MASK_4_BIT;
    printf("\tBluetooth RX digital gain is %d dB\n", (3*curr)-30);
    val = val >> 4;

    // bits 4-7
    curr = val & MASK_4_BIT;
    printf("\tBluetooth TX digital gain is %d dB\n", (3*curr)-15);
  }
}

/**
 * @brief print values in register BTSTPGA at 0x20
 *
 **/
void printBtStPga(unsigned char val)
{
  int curr;

  printf("BTSTPGA reg at addr 0x20: value is 0x%x\n", val);

  if (verbose) {
    // bits 0-5
    curr = val & MASK_6_BIT;
    if (0 == curr) {
      printf("\tBluetooth side tone gain is muted\n");
    } else {
      printf("\tBluetooth side tone gain is %d dB\n", curr-51);
    }
  }
  
}

/**
 * @brief print values in register EAR_CTL at 0x21
 *
 **/
void printEarCtl(unsigned char val)
{
  unsigned char curr;

  printf("EAR_CTL reg at addr 0x21: value is 0x%x\n", val);

  if (verbose) {
    // bit 0
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tEarphone voice enabled\n");
    } else {
      printf("\tEarphone voice disabled\n");
    }
    val = val >> 1;

    // bit 1 
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tEarphone L1 enabled\n");
    } else {
      printf("\tEarphone L1 disabled\n");
    }
    val = val >> 1;

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tEarphone L2 enabled\n");
    } else {
      printf("\tEarphone L2 disabled\n");
    }
    val = val >> 1;

    // bit 3 
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tEarphone R1 enabled\n");
    } else {
      printf("\tEarphone R1 disabled\n");
    }
    val = val >> 1;

    // bits 4 & 5 
    curr = val & MASK_2_BIT;
    if (0 == curr) {
      printf("\tEarphone gain amplifier in power-down mode\n");
    } else {
      printf("\tEarphone gain is %d\n", (3-curr)*6);
    }
    val = val >> 3; // skip bit 6

    // bit 7
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tEarphone output low-level enabled (Output=L)\n");
    } else {
      printf("\tEarphone output low-level disabled\n");
    }
  }
}

/**
 * @brief print values in register HS_SEL at 0x22
 *
 **/
void printHsSel(unsigned char val)
{
  unsigned char curr;

  printf("HS_SEL reg at addr 0x22: value is 0x%x\n", val);

  if (verbose) {
    // bit 0 
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tHeadset left voice enabled\n");
    } else {
      printf("\tHeadset left voice disabled\n");
    }
    val = val >> 1;

    // bit 1 
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tHeadset left audio L1 enabled\n");
    } else {
      printf("\tHeadset left audio L1 disabled\n");
    }
    val = val >> 1;

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tHeadset left audio L2 enabled\n");
    } else {
      printf("\tHeadset left audio L2 disabled\n");
    }
    val = val >> 1;

    // bit 3
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tHeadset right voice enabled\n");
    } else {
      printf("\tHeadset right voice disabled\n");
    }
    val = val >> 1;

    // bit 4
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tHeadset right audio R1 enabled\n");
    } else {
      printf("\tHeadset right audio R1 disabled\n");
    }
    val = val >> 1;

    // bit 5
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tHeadset right audio R2 enabled\n");
    } else {
      printf("\tHeadset right audio R2 disabled\n");
    }
    val = val >> 1;

    // bit 6
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tHeadset amplifier output is in low state\n");
    } else {
      printf("\tHeadset amplifier low state is disabled\n");
    }
    val = val >> 1;

    // bit 7
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tHeadset HSOR signals inverted\n");
    } else {
      printf("\tHeadset HSOR signals not inverted\n");
    }
    val = val >> 1;
  }

}

/**
 * @brief print values in register HS_GAIN_SET at 0x23
 *
 **/
void printHsGainSet(unsigned char val)
{
  int curr;

  printf("HS_GAIN_SET reg at addr 0x23: value is 0x%x\n", val);

  if (verbose) {

    // bits 0 & 1
    curr = val & MASK_2_BIT;
    {
      if (0 == curr) {
	printf("\tHeadset left amplifier in power down mode\n");
      } else {
	printf("\tHeadset left amplifier gain is %d dB\n", (2-curr)*6);
      }
    }
    val = val >> 2;

    // bits 2 & 3;
    curr = val & MASK_2_BIT;
    {
      if (0 == curr) {
	printf("\tHeadset right amplifier in power down mode\n");
      } else {
	printf("\tHeadset right amplifier gain is %d dB\n", (2-curr)*6);
      }
    }

    // bits 4-7 not used
  }

}

/**
 * @brief print values in register HS_POPN_SET at 0x24
 *
 **/
void printHsPopnSet(unsigned char val)
{
  unsigned char curr;

  printf("HS_POPN_SET reg at addr 0x24: value is 0x%x\n", val);

  if (verbose) {

    val = val >> 1; // skip bit 0

    // bit 1
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tHeadset amplifier ramping up from zero to hso_vmid\n");
    } else {
      printf("\tHeadset amplifier ramping down from hso_vmid to 0\n");
    }
    val = val >> 1;

    // bits 2-4
    curr = val & MASK_3_BIT;
    printf("\tRamp time is 2^%d/f(MCLK)\n", 19+curr);
    printf("\t*f(MCLK) = 19.2 MHz->delay=%d ms\n", rampTable[curr].x);
    printf("\t*f(MCLK) = 26 MHz->delay=%d ms\n", rampTable[curr].y);
    printf("\t*f(MCLK) = 38.4 MHz->delay=%d ms\n", rampTable[curr].z);
    val = val >> 3;

    // bit 5
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tHeadset mute control (external FET) is on\n");
    } else {
      printf("\tHeadset mute control (external FET) is off\n");
    }
    val = val >> 1;

    // bit 6
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tHeadset VMID buffer enabled\n");
    } else {
      printf("\tHeadset VMID buffer diabled\n");
    }

    // bit 7 is reserved
  }
}

/**
 * @brief print values in register PREDL_CTL at 0x25
 *
 **/
void printPredlCtl(unsigned char val)
{
  int curr;

  printf("PREDL_CTL reg at addr 0x25: value is 0x%x\n", val);

  if (verbose) {

    // bit 0
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tPredriver left amplifier voice enabled\n");
    } else {
      printf("\tPredriver left amplifier voice disabled\n");
    }
    val = val >> 1;

    // bit 1
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tPredriver left amplifier audio L1 enabled\n");
    } else {
      printf("\tPredriver left amplifier audio L1 disabled\n");
    }
    val = val >> 1;

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tPredriver left amplifier audio L2 enabled\n");
    } else {
      printf("\tPredriver left amplifier audio L2 disabled\n");
    }
    val = val >> 1;

    // bit 3
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tPredriver left amplifier audio R2 enabled\n");
    } else {
      printf("\tPredriver left amplifier audio R2 disabled\n");
    }
    val = val >> 1;

    // bits 4 & 5
    curr = val & MASK_2_BIT;
    if (0 == curr) {
      printf("\tPredriver left amplifier in power-down mode\n");
    } else {
      printf("\tPredriver left amplifier gain is %d dB\n", (2-curr)*6);
    }
    val = val >> 3; // skip bit 6

    // bit 7
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tPredriver left amplifier output low-level enabled (OUTPUT=L)\n");
    } else {
      printf("\tPredriver left amplifier output low-level disabled\n");
    }
  }

}

/**
 * @brief print values in register PREDR_CTL at 0x26
 *
 **/
void printPredrCtl(unsigned char val)
{
  int curr;

  printf("PREDR_CTL reg at addr 0x26: value is 0x%x\n", val);

  if (verbose) {
    // bit 0
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tPredriver right amplifier voice enabled\n");
    } else {
      printf("\tPredriver right amplifier disabled\n");
    }
    val = val >> 1;

    // bit 1
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tPredriver right amplifier audio R1 enabled\n");
    } else {
      printf("\tPredriver right amplifier audio R1 disabled\n");
    }
    val = val >> 1;

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tPredriver right amplifier audio R2 enabled\n");
    } else {
      printf("\tPredriver right amplifier audio R2 disabled\n");
    }
    val = val >> 1;

    // bit 3
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tPredriver right amplifier audio L2 enabled\n");
    } else {
      printf("\tPredriver right amplifier audio L2 disabled\n");
    }
    val = val >> 1;

    // bits 4 & 5
    curr = val & MASK_2_BIT;
    if (0 == curr) {
      printf("\tPredriver right amplifier amplifier in power-down mode\n");
    } else {
      printf("\tPredriver right amplifier amplifer gain is %d dB\n", (2-curr)*6);
    }
    val = val >> 3; // skip bit 6

    // bit 7
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tPredriver right amplifier output low-level enabled (OUTPUT=L)\n");
    } else {
      printf("\tPredriver right amplifier output low-level disabled\n");
    }
  }
}

/**
 * @brief print values in register PRECKL_CTL at 0x27
 *
 **/
void printPrecklCtl(unsigned char val)
{
  int curr;

  printf("PRECKL_CTL reg at addr 0x27: value is 0x%x\n", val);

  if (verbose) {

    // bit 0
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tPreamplifier left carkit voice enabled\n");
    } else {
      printf("\tPreamplifier left carkit voice disabled\n");
    }
    val = val >> 1;

    // bit 1
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tPreamplifier left carkit audio L1 enabled\n");
    } else {
      printf("\tPreamplifier left carkit audio L1 disabled\n");
    }
    val = val >> 1;

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tPreamplifier left carkit audio L2 enabled\n");
    } else {
      printf("\tPreamplifier left carkit audio L2 disabled\n");
    }
    val = val >> 2;  //skip bit 3


    // bits 4 & 5
    curr = val & MASK_2_BIT;
    if (0 == curr) {
      printf("\tPreamplifier left carkit gain in power-down mode\n");
    } else {
      printf("\tPreamplifier left carkit gain is %d dB\n", (2-curr)*6);
    }
    val = val >> 2; 

    // bit 6
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tPreamplifier left carkit enabled\n");
    } else {
      printf("\tPreamplifier left carkit disabled\n");
    }

    // bit 7 is reserved
  }
}

/**
 * @brief print values in register PRECKR_CTL at 0x28
 *
 **/
void printPreckrCtl(unsigned char val)
{
  int curr;

  printf("PRECKR_CTL reg at addr 0x28: value is 0x%x\n", val);

  if (verbose) {
    // bit 0
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tPreamplifier right carkit voice enabled\n");
    } else {
      printf("\tPreamplifier right carkit voice disabled\n");
    }
    val = val >> 1;

    // bit 1
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tPreamplifier right carkit audio L1 enabled\n");
    } else {
      printf("\tPreamplifier right carkit audio L1 disabled\n");
    }
    val = val >> 1;

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tPreamplifier right carkit audio L2 enabled\n");
    } else {
      printf("\tPreamplifier right carkit audio L2 disabled\n");
    }
    val = val >> 2;  //skip bit 3


    // bits 4 & 5
    curr = val & MASK_2_BIT;
    if (0 == curr) {
      printf("\tPreamplifier right carkit gain in power-down mode\n");
    } else {
      printf("\tPreamplifier right carkit gain is %d dB\n", (2-curr)*6);
    }
    val = val >> 2; 

    // bit 6
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tPreamplifier right carkit enabled\n");
    } else {
      printf("\tPreamplifier right carkit disabled\n");
    }

    // bit 7 is reserved
  }
}


/**
 * @brief print values in register  HFL_CTL at 0x29
 *
 **/
void printHflCtl(unsigned char val)
{
  unsigned char curr;

  printf("HFL_CTL reg at addr 0x29: value is 0x%x\n", val);

  if (verbose) {

    // bits 0 & 1 
    curr = val & MASK_2_BIT;
    switch (curr) {
    case 0: 
      printf("\tinput select for hands-free left amplifier is voice\n");
      break;
    case 1:
      printf("\tinput select for hands-free left amplifier is audio left channel 1\n");
      break;
    case 2:
      printf("\tinput select for hands-free left amplifier is audio left channel 2\n");
      break;
    case 3:
      printf("\tinput select for hands-free left amplifier is audio right channel 2\n");
      break;
    default:
      printf("too weird\n");
    }
    val = val >> 2;

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tH-bridge for hands-free left amplifier is eanbled\n");
    } else {
      printf("\tH-bridge for hands-free left amplifier is disabled\n");
    }
    val = val >> 1;

    // bit 3
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tLoop for hands-free left amplifier is enabled\n");
    } else {
      printf("\tLoop for hands-free left amplifier is disabled\n");
    }
    val = val >> 1;

    // bit 4 
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tramp for hands-free left amplifier is enabled\n");
    } else {
      printf("\tramp for hands-free left amplifier is disabled\n");
    }
    val = val >> 1;

    // bit 5
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\treference for hands-free left amplifier is enabled\n");
    } else {
      printf("\treference for hands-free left amplifier is disabled\n");
    }

    // bits 6&7 reserved
  }

}

/**
 * @brief print values in register  HFR_CTL at 0x2A
 *
 **/
void printHfrCtl(unsigned char val)
{
  unsigned char curr;

  printf("HFR_CTL reg at addr 0x2A: value is 0x%x\n", val);

  if (verbose) {
    // bits 0 & 1 
    curr = val & MASK_2_BIT;
    switch (curr) {
    case 0: 
      printf("\tinput select for hands-free right amplifier is voice\n");
      break;
    case 1:
      printf("\tinput select for hands-free right amplifier is audio right channel 1\n");
      break;
    case 2:
      printf("\tinput select for hands-free right amplifier is audio right channel 2\n");
      break;
    case 3:
      printf("\tinput select for hands-free right amplifier is audio left channel 2\n");
      break;
    default:
      printf("too weird\n");
    }
    val = val >> 2;

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tH-bridge for hands-free right amplifier is enabled\n");
    } else {
      printf("\tH-bridge for hands-free right amplifier is disabled\n");
    }
    val = val >> 1;

    // bit 3
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tLoop for hands-free right amplifier is enabled\n");
    } else {
      printf("\tLoop for hands-free right amplifier is disabled\n");
    }
    val = val >> 1;

    // bit 4 
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tramp for hands-free right amplifier is enabled\n");
    } else {
      printf("\tramp for hands-free right amplifier is disabled\n");
    }
    val = val >> 1;

    // bit 5
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\treference for hands-free right amplifier is enabled\n");
    } else {
      printf("\treference for hands-free right amplifier is disabled\n");
    }

    // bits 6&7 reserved
  }
}

/**
 * @brief print values in register ALC_CTL at 0x2B
 *
 **/
void printAlcCtl(unsigned char val)
{
  unsigned char curr;


  if (verbose) {
    printf("ALC_CTL reg at addr 0x2B: value is 0x%x\n", val);

    // bits 0-2
    curr = val & MASK_3_BIT;
    printf("\tWait time control for ALC is %d * WaitToRelease\n", (int)pow(2,curr));
    val = val >> 3;

    // bit 3
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tMain mic ALC power is on\n");
    } else {
      printf("\tMain mic ALC power is off\n");
    }
    val = val >> 1;

    // bit 4
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tSub mic ALC power is on\n");
    } else {
      printf("\tSub mic ALC power is off\n");
    }
    val = val >> 1;

    // bit 5
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tALC mode is interlock\n");
    } else {
      printf("\tALC mode is normal\n");
    }

    // bits 6 & 7 are spare
  }
}

/**
 * @brief print values in register ALC_SET1 at 0x2C
 *
 **/
void printAlcSet1(unsigned char val)
{
  int curr;

  if (verbose) {

    printf("ALC_SET1 reg at addr 0x2C: value is 0x%x\n", val);

    // bits 0-2
    curr = val & MASK_3_BIT;
    if (curr >= 7) {
      curr = 6;
    }
    printf("\tALC low threshold is %d\n", (-3*curr)-12);
    val = val >> 3;

    // bits 3-5
    curr = val & MASK_3_BIT;
    if (curr >= 7) {
      curr = 6;
    }
    printf("\tALC high threshold is %d\n", (-3*curr)-9);

    // bits 6 & 7 reserved
  }

}

/**
 * @brief print values in register ALC_SET2 at 0x2D
 *
 **/
void printAlcSet2(unsigned char val)
{
  unsigned char curr;

  printf("ALC_SET2 reg at addr 0x2D: value is 0x%x\n", val);

  if (verbose) {

    // bits 0-2
    curr = val & MASK_3_BIT;
    if (7 == curr) {
      curr = 6;
    }
    printf("\tALC release time defines the minimum between two steps\n");
    printf("\tof increasing gain.\n");
    printf("\tALC release time: %f ms\n", ((float)(pow(2,curr)))*1.6);
    val = val >> 3;

    // bits 3-5
    curr = val & MASK_3_BIT;
    printf("\tALC attack time defines the minimum time between two steps\n");
    printf("\tof decreasing gain.\n");
    if (curr <=3) {
      printf("\tALC attack time: %f us\n", ((float)(pow(2,curr)))*6.25);
    } else {
      printf("\tALC attack time: %d us\n", (int)(pow(2, curr-4)*100));
    }
    val = val >> 3;

    // bit 6
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tALC step is 2 dB\n");
    } else {
      printf("\tALC step is 1 dB\n");
    }

    // bit 7 reserved
  }
}

/**
 * @brief print values in register BOOST_CTL at 0x2E
 *
 **/
void printBoostCtl(unsigned char val)
{
  unsigned char curr;

  printf("BOOST_CTL reg at addr 0x2E: value is 0x%x\n", val);

  if (verbose) {
    printf("\tBoost effect adds emphasis to low frequencies to compensate the\n");
    printf("\thigh pass filter created by the RC filter of the headset in the\n");
    printf("\tac coupling configuration.  Four modes define the equalization profile:\n");
    printf("\tthree modes have slightly different frequency responses;the fourth mode\n");
    printf("\tdisables the boost effect.  The difference between the three boost modes\n");
    printf("\tis the frequency response, which is defined by the frequency response\n");
    printf("\tvs. the input frequency and hte Fs frequency (see section 6.1.2 Boost Stage\n");
    printf("\tin the Gaia data manual\n");
  
    // bits 0 and 1
    curr = val & MASK_2_BIT;
    if (0 == curr) {
      printf("\tBoost mode not in effect\n");
    } else {
      printf("\tBoost mode is %d\n", curr);
    }

    // bits 2-7 reserved
  }
}

/**
 * @brief print values in register SOFTVOL_CTL at 0x2F
 *
 **/
void printSoftVolCtl(unsigned char val)
{
  unsigned char curr;

  printf("SOFTVOL_CTL reg at addr 0x2F: value is 0x%x\n", val);

  if (verbose) {
  // bit 0
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tsoft volume control in application mode (on)\n");
    } else {
      printf("\tsoft volume control in bypass mode (off)\n");
    }
    val = val >> 5; // bits 1-4 reserved

    curr = val & MASK_3_BIT;
    if (curr <= 4) {
      printf("\tsoft volume sweep time control is %d * 0.8/Fs\n", (int)(pow(2,(10-curr))));
    } else {
      printf("\tsoft volume sweep time control is %d * 0.8/Fs\n", (int)(pow(4,( 7-curr))));
    }
  }
}

/**
 * @brief print values in register DTMF_FREQSEL at 0x30
 *
 **/
void printDtmfFreqSel(unsigned char val)
{
  unsigned char curr;

  printf("DTMF_FREQSEL reg at addr 0x30: value is 0x%x\n", val);

  if (verbose) {
    curr = val & MASK_5_BIT;
    if (curr > MAX_DTMF_TONES) {
      printf("\tdtmf tone unknown\n");
    } else {
      printf("\t%s\n", dtmfFrequencies[curr]);
    }

    // bits 5-7 reserved
  }
    
}

/**
 * @brief print values in register DTMF_TONEXT1H at 0x31
 *
 **/
void printDtmfTonExt1H(unsigned char val)
{
  printf("DTMF_TONEXT1H reg at addr 0x31: value is 0x%x\n", val);

  if (verbose) {
    printf("\tDTMF tone 1 high frequency is %f\n", ((float)(val + 256))*7.8125);
  }
}

/**
 * @brief print values in register DTMF_TONEXT1L at 0x32
 *
 **/
void printDtmfTonExt1L(unsigned char val)
{
  printf("DTMF_TONEXT1L reg at addr 0x32: value is 0x%x\n", val);
  if (verbose) {
    printf("\tDTMF tone 1 low frequency is %f\n", ((float)(val))*7.8125);
  }
}

/**
 * @brief print values in register DTMF_TONEXT2H at 0x33
 *
 **/
void printDtmfTonExt2H(unsigned char val)
{
  printf("DTMF_TONEXT2H reg at addr 0x33: value is 0x%x\n", val);

  if (verbose) {
    printf("\tDTMF tone 2 high frequency is %f\n", ((float)(val + 256))*7.8125);
  }
}

/**
 * @brief print values in register DTMF_TONEXT2 at 0x34
 *
 **/
void printDtmfTonExt2L(unsigned char val)
{
  printf("DTMF_TONEXT2 reg at addr 0x34: value is 0x%x\n", val);

  if (verbose) {
    printf("\tDTMF tone 2 low frequency is %f\n", ((float)(val))*7.8125);
  }
}

/**
 * @brief print values in register DTMF_TONOFF at 0x35
 *
 **/
void printDtmfTonOff(unsigned char val)
{
  unsigned char curr;

  printf("DTMF_TONOFF reg at addr 0x35: value is 0x%x\n", val);

  if (verbose) {

    // bits 0-3
    curr = val & MASK_4_BIT;
    printf("\tDTMF on time is %d ms\n", dtmfOnTimes[curr]);
    val = val >> 4;

    // bits 4-7
    curr = val & MASK_4_BIT;
    if (curr >= MAX_DTMF_OFF_TIMES) {
      printf("\twarning: DTMF on time is a reserved value\n");
    } else {
      printf("\tDTMF off time is %d ms\n", dtmfOffTimes[curr]);
    }
  }

}

/**
 * @brief print values in register  DTMF_WANONOFF at 0x36
 *
 **/
void printDtmfWanOnOff(unsigned char val)
{
  unsigned char curr;

  printf("DTMF_WANONOFF reg at addr 0x36: value is 0x%x\n", val);

  if (verbose) {
    // bits 0-3
    curr = val & MASK_4_BIT;
    printf("\tduring wobble on, the frequence is that of Tone 1\n");
    if (curr < MAX_WOBBLE_TIMES) {
      printf("\twobble on time is %f ms\n", wobbleTimes[curr]);
    } else {
      printf("\twarning: wobble on time is undefined\n");
    }
    val = val >> 4;

    // bits 4-7
    curr = val & MASK_4_BIT;
    printf("\tduring wobble off, the frequency is that of Tone 2\n");
    if (curr < MAX_WOBBLE_TIMES) {
      printf("\twobble off time is %f ms\n", wobbleTimes[curr]);
    } else {
      printf("\twarning: wobble off time is undefined\n");
    }
  }
}

/**
 * @brief print values in register CODEC_RX_SCRAMBLE_H at 0x37
 *
 **/
void printCodecRxScramH(unsigned char val)
{
  printf("CODEC_RX_SCRAMBLE_H reg at addr 0x37: value is 0x%x\n", val);
}

/**
 * @brief print values in register CODEC_RX_SCRAMBLE_M at 0x38
 *
 **/
void printCodecRxScramM(unsigned char val)
{
  printf("CODEC_RX_SCRAMBLE_M reg at addr 0x38: value is 0x%x\n", val);
}

/**
 * @brief print values in register CODEC_RX_SCRAMBLE_L at 0x39
 *
 **/
void printCodecRxScramL(unsigned char val)
{
  printf("CODEC_RX_SCRAMBLE_L reg at addr 0x39: value is 0x%x\n", val);
}

/**
 * @brief print values in register  APLL_CTL at 0x3A
 *
 **/
void printApllCtl(unsigned char val)
{
  unsigned char curr;

  printf("APLL_CTL reg at addr 0x3A: value is 0x%x\n", val);

  if (verbose) {
    // bits 0-3
    curr = val & MASK_4_BIT;
    switch (curr) {
    case 5:
      printf("\tinput frequency is 19.2 MHz\n");
      break;
    case 6:
      printf("\tinput frequency is 26 MHz\n");
      break;
    case 15:
      printf("\tintput frequency is 38.4 MHz\n");
      break;
    default:
      printf("warning: input frequency is a reserved value\n");
    }
    val = val >> 4;

    // bit 4
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tPLL power control enabled\n");
    } else {
      printf("\tPLL powr control disabled\n");
    }

    // bits 5-7 reserved
  }
}

/**
 * @brief print values in register DTMF_CTL at 0x3B
 *
 **/
void printDtmfCtl(unsigned char val)
{
  unsigned char curr;

  printf("DTMF_CTL reg at addr 0x3B: value is 0x%x\n", val);

  if (verbose) {

    // bit 0
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tDTMF tone generator enabled\n");
    } else {
      printf("\tDTMF tone generator disabled\n");
    }
    val = val >> 1;

    // bit 1
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tDTMF dual tone enabled\n");
    } else {
      printf("\tDTMF single tone enabled\n");
    }
    val = val >> 1;

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tDTMF tone pattern is 'wobble'\n");
    } else {
      printf("\tDTMF tone pattern is 'continuous'\n");
    }
    val = val >> 1;

    // bit 3
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tDTMF dual tone\n");
    } else {
      printf("\tDTMF single tone\n");
    }

    // bits 4-8 reserved
  }
}

/**
 * @brief print values in register DTMF_PGA_CTL2 at 0x3C
 *
 **/
void printDtmfPgaCtl2(unsigned char val)
{
  int curr;

  printf("DTMF_PGA_CTL2 reg at addr 0x3C: value is 0x%x\n", val);

  if (verbose) {
  
    // bits 0-2
    curr = val & MASK_3_BIT;
    printf("\ttone gain 3 is %d dB\n", (curr-7)*6);
    val = val >> 4; // skip bit 4

    // bits 4-6
    curr = val & MASK_3_BIT;
    printf("\ttone gain 4 is %d dB\n", (curr-7)*6);
    val = val >> 3;

    // bit 7
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\ttone attenuator enabled: -18 dB\n");
    } else {
      printf("\ttone attenuator disabled: 0 dB\n");
    }
  }
}

/**
 * @brief print values in register DTMF_PGA_CTL1 at 0x3D
 *
 **/
void printDtmfPgaCtl1(unsigned char val)
{
  int curr;

  printf("DTMF_PGA_CTL1 reg at addr 0x3D: value is 0x%x\n", val);


  if (verbose) {
    // bits 0-3
    curr = val & MASK_4_BIT;
    printf("\ttone gain 1 is %d dB\n", curr-15);
    val = val >> 4;

    // bits 4-7
    curr = val & MASK_4_BIT;
    printf("\ttone gain 2 is %d dB\n", curr-15);
    val = val >> 4;
  }
}

/**
 * @brief print values in register MISC_SET_1 at 0x3E
 *
 **/
void printMiscSet1(unsigned char val)
{
  unsigned char curr;

  printf("MISC_SET_1 reg at addr 0x3E: value is 0x%x\n", val);

  if (verbose) {

    // bit 0
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tdigital mixing left and right swapped\n");
    } else {
      printf("\tdigital mixing left and right not swapped\n");
    }
    val = val >> 1;

    // bit 1
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tpop noise reduced when analog gain change.  The\n");
      printf("\tgain change is done when the signal croses zero or\n");
      printf("\tafter a timeout of 25 ms\n");
    } else {
      printf("\tBypass mode.  Gain change is effective immediately.\n");
      printf("\tIf there is a signal, pop noise can be generated\n");
    }
    val = val >> 4;  // skip bits 2-4

    // bit 5
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tAnalog FM radio loop enabled\n");
    } else {
      printf("\tAnalog FM radio loop disabled\n");
    }
    val = val >> 1;

    // bit 6 
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tscramble function enabled\n");
    } else {
      printf("\tscramble function disabled\n");
    }
    val = val >> 1;

    // bit 7
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\t64 kHz clock enabled\n");
    } else {
      printf("\t64 kHz clock disabled\n");
    }
  }
}

/**
 * @brief print values in register PCMBTMUX at 0x3F
 *
 **/
void printPcmBtMux(unsigned char val)
{

  unsigned char curr;

  printf("PCMBTMUX reg at addr 0x3F: value is 0x%x\n", val);

  if (verbose) {

    // bit 0 
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\ttone on for BT RX signal\n");
    } else {
      printf("\ttone off for BT RX signal\n");
    }
    val = val >> 1;

    // bit 1 
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\ttone on for BT TX signal\n");
    } else {
      printf("\ttone off for BT TX signal\n");
    }
    val = val >> 1;

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\ttone on for PCM RX signal\n");
    } else {
      printf("\ttone off for PCM RX signal\n");
    }
    val = val >> 1;

    // bit 3
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\ttone on for PCM TX signal\n");
    } else {
      printf("\ttone off for PCM TX signal\n");
    }
    val = val >> 2; // skip bit 4

    // bit 5 
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tBluetooth data out mux select is BT data\n");
    } else {
      printf("\tBluetooth data out mux select is VTx data\n");
    }
    val = val >> 1;

    // bit 6
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tVoice RX input mux select is BT data\n");
    } else {
      printf("\tVoice RX input mux select is VTx data\n");
    }
    val = val >> 1;


    // bit 7
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tvoice data out mux select is BT data\n");
    } else {
      printf("\tvoice data out mux select is VTx data\n");
    }
  }
}

/**
 * @brief print values in register RX_PATH_SEL at 0x43
 *
 **/
void printRxPathSel(unsigned char val)
{
  unsigned char curr;
  unsigned char option;

  printf("RX_PATH_SEL reg at addr : value is 0x%x\n", val);

  if (verbose) {

    option = getOptionValue();

    // bits 0-1
    curr = val & MASK_2_BIT;
    switch (curr) {
    case 0: 
    case 1:
      if (1 == option) {
	printf("\t%s\n", rxr1PgaSelect[curr]);
      } else {
	printf("\twarning: setting not supported in option 2\n");
      }
      break;
    case 2:
    case 3:
      printf("\t%s\n", rxr1PgaSelect[curr]);
      break;
    default:
      printf("too weird\n");
    }
    val = val >> 2;

    // bits 2-3
    curr = val & MASK_2_BIT;
    switch (curr) {
    case 0: 
    case 1:
      if (1 == option) {
	printf("\t%s\n", rxl1PgaSelect[curr]);
      } else {
	printf("\twarning: setting not supported in option 2\n");
      }
      break;
    case 2:
    case 3:
      printf("\t%s\n", rxl1PgaSelect[curr]);
      break;
    default:
      printf("too weird\n");
    }
    val = val >> 2;

    // bit 4
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tSDRM2 (mix SDRL2 and SDRR2) to RXR2 PGA\n");
    } else {
      printf("\tSDRR2 to RXR2 PGA\n");
    }
    val = val >> 1;

    // bit 5
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tSDRM2 ( mix SDRL2 and SDRR2) to RXL2 PGA\n");
    } else {
      printf("\tSDRL2 to RXL2 PGA\n");
    }

    // bits 6 & 7 reserved
  }


}

/**
 * @brief print values in register VDL_APGA_CTL at 0x44
 *
 **/
void printVdlApgaCtl(unsigned char val)
{

  int curr;

  printf("VDL_APGA_CTL reg at addr 0x44: value is 0x%x\n", val);

  if (verbose) {
    // bit 0
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tvoice downlink analog PGA in application mode (on)\n");
    } else {
      printf("\tvoice downlink analog PGA in power-down maode\n");
    }
    val = val >> 1;

    // bit 1
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tvoice downlink digital to analog path enabled\n");
    } else {
      printf("\tvoice downlink digital to analog path disabled\n");
    }
    val = val >> 1;

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tvoice downlink FM loop path enabled\n");
    } else {
      printf("\tvoice downlink VM loop path disabled\n");
    }
    val = val >> 1;

    // bits 3-7
    curr = val & MASK_5_BIT;
    if (curr <= 0x12) {
      printf("\tvoice downlink PGA gain is %d dB\n", (-2*curr)+ 12);
    } else {
      printf("\twarning: voice downlink PGA gain set to reserved value\n");
    }
  }

}

/**
 * @brief print values in register VIBRA_CTL at 0x45
 *
 **/
void printVibraCtl(unsigned char val)
{
  unsigned char curr;

  printf("VIBRA_CTL reg at addr 0x45: value is 0x%x\n", val);

  if (verbose) {

    // bit 0
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tvibrator H-bridge is on\n");
    } else {
      printf("\tvibrator H-bridge is off\n");
    }
    val = val >> 1;

    // bit 1
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tVIBRA_DIR: H-bridge direction is negative polarity\n");
    } else {
      printf("\tVIBRA_DIR: H-bridge direction is positive polarity\n");
    }
    val = val >> 1;

    // bits 2 & 3
    curr = val & MASK_2_BIT;
    switch (curr) {
    case 0: 
      printf("\taudio channel LEFT1 input selected\n");
      break;
    case 1:
      printf("\taudio channel RIGHT1 input selected\n");
      break;
    case 2:
      printf("\taudio channel LEFT2 input selected\n");
      break;
    case 3:
      printf("\taudio channel RIGHT2 input selected\n");
      break;
    default:
      printf("too weird\n");
    }
    val = val >> 2;

    // bit 4
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\taudio data is vibrator driver\n");
      printf("\tThe audio channel is controlled by\n");
      printf("\tAUDIOCH word control\n");
    } else {
      printf("\tlocal vibrator driver (controlled by\n");
      printf("\tOCP register, average value control word in\n");
      printf("\tVIBRA_SET register\n");
    }
    val = val >> 1;

    // bit 5
    if (curr) {
      // only meaningful if audio data is driving the vibrator H-bridge
      curr = val & MASK_1_BIT;
      if (curr) {
	printf("\tDirection is given by the audio data MSB\n");
      } else {
	printf("\tDirection is given by the VIBRA_DIR bit\n");
      }
    }

    // bits 6 & 7 reserved
  }
}

/**
 * @brief print values in register VIBRA_SET at 0x46
 *
 **/
void printVibraSet(unsigned char val)
{
  printf("VIBRA_SET reg at addr 0x46: value is 0x%x\n", val);

  if (verbose) {
    if (0 == val) {
      printf("\twarning: the value 0 is forbidden for the vibrator H-bridge PWM generator turn on value\n");
    } else {
      printf("\tThis is the vibrator H-bridge PWM generator turn on value\n");
      printf("\tThe average value is the inverse of this value\n");
    }
  }
}

/**
 * @brief print values in register ANAMIC_GAIN at 0x48
 *
 **/
void printAnamicGain(unsigned char val)
{
  unsigned char curr;

  printf("ANAMIC_GAIN reg at addr 0x48: value is 0x%x\n", val);

  if (verbose) {

    // bits 0-2
    curr = val & MASK_3_BIT;
    printf("\tIf ALC is enabled for the main microphone, this is the maximum gain\n");
    printf("\tsetting of the left microphone amplifier.  If ALC is disabled, it's\n");
    printf("\tthe gain setting of the left microphone amplifier:\n");
    printf("\tGain: %d dB\n", curr*6);
    val = val >> 3;

    // bits 3-5
    curr = val & MASK_3_BIT;
    printf("\tIf ALC is enabled for the sub-microphone, this is the maximum gain\n");
    printf("\tsetting of the right microphone amplifier.  If ALC is disabled, it's\n");
    printf("\tthe gain setting of the right microphone amplifier:\n");
    printf("\tGain: %d dB\n", curr*6);

    // bits 6 &7 reserved
  }

}

/**
 * @brief print values in register MISC_SET_2 at 0x49
 *
 **/
void printMiscSet2(unsigned char val)
{
  unsigned char curr;

  printf("MISC_SET_2 reg at addr 0x49: value is 0x%x\n", val);

  if (verbose) {

    val = val >> 1; // skip bit 0

    // bit 1
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tVoice RX high pass filter bypassed\n");
    } else {
      printf("\tVoice RX high pass filter in app mode (on)\n");
    }
    val = val >> 1;

    // bit 2
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tVoice TX high pass filter bypassed\n");
    } else {
      printf("\tVoice TX high pass filter in application mode (on)\n");
    }
    val = val >> 1;

    // bit 3
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tAudio RX high pass filter bypassed\n");
    } else {
      printf("\tAudio RX high pass filter in application mode (on)\n");
    }
    val = val >> 1;

    // bit 4
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tVoice RX third-order high pass filter bypassed\n");
    } else {
      printf("\tVoice RX third-order high pass filter in application mode (on)\n");
    }
    val = val >> 1;

    // bit 5
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tAudio TX high-pass filter bypassed\n");
    } else {
      printf("\tAudio TX high-pass filter in application mode (on)\n");
    }
    val = val >> 1;

    // bit 6
    curr = val & MASK_1_BIT;
    if (curr) {
      printf("\tVoice TX third-order high-pass filter bypassed\n");
    } else {
      printf("\tVoice TX third-order high-pass filter in application mode (on)\n");
    }

    // bit 7 reserved
  }
}

int populateRegList(void) 
{    
  int audioCtl, rc;

  audioCtl = open(sysfsName, O_RDWR);
  if (audioCtl < 0) {
    printf("%s not available\n", sysfsName);
    return (-1);
  }
  rc= read(audioCtl, regListBuf, REG_LIST_BUF_LEN);
  if (rc < 0) {
    printf("error reading from audio registers\n");
    close(audioCtl);
    return (-1);
  }
  close(audioCtl);
  return 0;
}

int getRegValue(int regAddr) 
{
    int numHeaderChars = (6*16)+8;
    int numRowChars = 4; // line feed, 2 character row label, and '|'
    int numColChars = 3; // space + 2 characters
    int numCharsPerRow = 16 * numColChars;
    int numRows, numCols, val, offset;

    numRows = regAddr/16 + 1;
    numCols = regAddr % 16;
    offset = numHeaderChars + \
        (numRows * numRowChars) + \
        (numCols * numColChars) + \
        ((numRows-1)*numCharsPerRow) + 1;
    strncpy(&(regString[2]), &(regListBuf[offset]), 2);
    regString[4] = 0;
    val=strtod(regString, NULL);
    return(val);
}

/**
 * @brief returns 1 if in option 1, 2 if in option 2 
 *
 **/
int getOptionValue(void)
{
  int optionReg, option;
  optionReg = getRegValue(1);
  if (optionReg & MASK_1_BIT) {
    option=1;
  } else {
    option=2;
  }
  return(option);
}



int
main(int argc, char **argv)
{
  int opt, reg, i, val;
  extern char* optarg;
  extern int optind, optopt;

  if (populateRegList() < 0) {
    return(-1);
  }

  sprintf(regString, "0x");

  if (argc == 1) {
    printf("dumping all regs\n");
    for (i=1; i < NUM_AUDIO_REGS; i++) {
      if (printRegisterArrayFns[i] != printUnknown) {
	val=getRegValue(i);
	printf("%2x: ", i);
	(*(printRegisterArrayFns[i]))(val);
      }
    }
  } else {
    while ((opt = getopt(argc, argv, "hr:v")) != -1) {
      switch (opt) {
      case 'r':
	reg = strtod(optarg, NULL);
	printf("%2x: ", reg);
	val=getRegValue(reg);
	(*(printRegisterArrayFns[reg]))(val);
	break;
      case 'v':
	verbose=1;
	if (argc == 2) {
	  // no regs specified, just dump everything
	  for (i=1; i < NUM_AUDIO_REGS; i++) {
	    if (printRegisterArrayFns[i] != printUnknown) {
	      val=getRegValue(i);
	      printf("%2x: ", i);
	      (*(printRegisterArrayFns[i]))(val);
	    }
	  }
	}
	break;
      default:
	printf("usage: audioreg [-v][-r regAddressInHex]\n");
	printf("-v turns on verbose mode\n");
	printf("can specify multiple regs with multiple -r parameters\n");
	printf("example: 'audioreg' to dump all audio regs\n");
	printf("example: 'audioreg -v' to dump all regs with bit info\n");
	printf("example: 'audioreg -v -r 0x1e -r 0x1f -r 0x20 -r 0x3f'\n");
	printf("\tto dump Bluetooth registers with bit info\n");
	break;
      }
    }
  }
  return(0);
}




 

            
