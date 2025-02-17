#include "LpcAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "LpcAnalyzer.h"
#include "LpcAnalyzerSettings.h"
#include <iostream>
#include <sstream>
#include <string.h>

LpcAnalyzerResults::LpcAnalyzerResults(LpcAnalyzer* analyzer, LpcAnalyzerSettings* settings)
    :   AnalyzerResults(),
        mSettings(settings),
        mAnalyzer(analyzer)
{
}

LpcAnalyzerResults::~LpcAnalyzerResults()
{
}

void LpcAnalyzerResults::GenerateBubbleText(U64 frame_index, Channel& channel, DisplayBase display_base)    //unrefereced vars commented out to remove warnings.
{

#define NIBBLE_TO_BINARY_PATTERN "%c%c%c%c"
#define NIBBLE_TO_BINARY(nibble)  \
	  (nibble & 0x08 ? '1' : '0'), \
	  (nibble & 0x04 ? '1' : '0'), \
	  (nibble & 0x02 ? '1' : '0'), \
	  (nibble & 0x01 ? '1' : '0') 


	ClearResultStrings();
	Frame frame = GetFrame(frame_index);


	if (channel == mSettings->mLFRAMEChannel) {
		LpcAnalyzer::LPC_STATE state = (LpcAnalyzer::LPC_STATE)frame.mData2;
		char bubble_str[128];

		/* COMBINE ADDRESS NIBBLES AND BUBBLE THE FULL ADDRESS OVER THE ADDRESS FRAMES*/
		/* IO READ WRITES */
		if (state == LpcAnalyzer::LPC_STATE::IO_READ_ADD ||
			state == LpcAnalyzer::LPC_STATE::IO_WRITE_ADD) {

			snprintf(bubble_str, 100, "Address (0x%04x)", (U32)frame.mData1);
			AddResultString(bubble_str);
		}

		/* COMBINE ADDRESS NIBBLES AND BUBBLE THE FULL ADDRESS OVER THE ADDRESS FRAMES*/
		/* MEM READ WRITES */
		else if (state == LpcAnalyzer::LPC_STATE::MEM_READ_ADD ||
			state == LpcAnalyzer::LPC_STATE::MEM_WRITE_ADD) {
			snprintf(bubble_str, 100, "Address (0x%08x)", (U32)frame.mData1);
			AddResultString(bubble_str);
		}


		/* COMBINE DATA NIBBLES AND BUBBLE THE DATA BYTE OVER THE DATA FRAMES*/
		/* IO READ WRITES AND MEM READ WRITES*/
		if (state == LpcAnalyzer::LPC_STATE::IO_READ_DATA ||
			state == LpcAnalyzer::LPC_STATE::IO_WRITE_DATA ||
			state == LpcAnalyzer::LPC_STATE::MEM_WRITE_DATA || 
			state == LpcAnalyzer::LPC_STATE::MEM_WRITE_DATA) {

			snprintf(bubble_str, 100, "Data (0x%02x)", (U32)frame.mData1);
			AddResultString(bubble_str);
		}

	}



	if (channel == mSettings->mLCLKChannel) {
		char state_str[128];
		char bubble_str[128];

		LpcAnalyzer::LPC_STATE state = (LpcAnalyzer::LPC_STATE)frame.mData2;

		switch (state) {
		case LpcAnalyzer::LPC_STATE::START:
			memcpy(state_str, "START", 6);
			break;

		case LpcAnalyzer::LPC_STATE::CYCTYPE:
			switch (frame.mData1) {
			case 0b0000:
				memcpy(state_str, "IO READ", 8);
				break;
			case 0b0010:
				memcpy(state_str, "IO WRITE", 9);
				break;
			case 0b0100:
				memcpy(state_str, "MEM READ", 9);
				break;
			case 0b0110:
				memcpy(state_str, "MEM WRITE", 10);
				break;
			}

			break;

		case LpcAnalyzer::LPC_STATE::IO_READ_ADD:
		case LpcAnalyzer::LPC_STATE::IO_WRITE_ADD:
			snprintf(bubble_str, 100, "Address (0x%04x)", (U32)frame.mData1);
			AddResultString(bubble_str);
			return;
		case LpcAnalyzer::LPC_STATE::MEM_READ_ADD:
		case LpcAnalyzer::LPC_STATE::MEM_WRITE_ADD:
			snprintf(bubble_str, 100, "Address (0x%08x)", (U32)frame.mData1);
			AddResultString(bubble_str);
			return;

		case LpcAnalyzer::LPC_STATE::IO_READ_DATA:
		case LpcAnalyzer::LPC_STATE::IO_WRITE_DATA:
		case LpcAnalyzer::LPC_STATE::MEM_READ_DATA:
		case LpcAnalyzer::LPC_STATE::MEM_WRITE_DATA:
			snprintf(bubble_str, 100, "Data (0x%02x)", (U32)frame.mData1);
			AddResultString(bubble_str);
			return;

		case LpcAnalyzer::LPC_STATE::IO_READ_TAR:
		case LpcAnalyzer::LPC_STATE::IO_WRITE_TAR:
		case LpcAnalyzer::LPC_STATE::MEM_READ_TAR:
		case LpcAnalyzer::LPC_STATE::MEM_WRITE_TAR:
		case LpcAnalyzer::LPC_STATE::TAR:
			memcpy(state_str, "TAR", 4);
			break;

		case LpcAnalyzer::LPC_STATE::IO_READ_SYNC:
		case LpcAnalyzer::LPC_STATE::IO_WRITE_SYNC:
		case LpcAnalyzer::LPC_STATE::MEM_READ_SYNC:
		case LpcAnalyzer::LPC_STATE::MEM_WRITE_SYNC:
			memcpy(state_str, "SYNC", 5);
			break;

		case LpcAnalyzer::LPC_STATE::COMPLETE:
			memcpy(state_str, "COMPLETE", 9);
			break;

		default:
			memcpy(state_str, "UNKNOWN", 8);
			break;
		}

		if (frame.mFlags != 0) {
			snprintf(bubble_str, 30, "ERROR");
		}
		else {
			snprintf(bubble_str, 30, "%s (0b%c%c%c%c)", state_str, NIBBLE_TO_BINARY(frame.mData1));	
		}
		AddResultString(bubble_str);
	}

}

void LpcAnalyzerResults::GenerateExportFile(const char *file, DisplayBase display_base, U32 /*export_type_user_id*/)
{
    //export_type_user_id is only important if we have more than one export type.

    std::stringstream ss;
    void *f = AnalyzerHelpers::StartFile(file);

    U64 trigger_sample = mAnalyzer->GetTriggerSample();
    U32 sample_rate = mAnalyzer->GetSampleRate();

    ss << "Time [ms],Transaction separated by nibble LAD[3:0]" << std::endl;

    U64 num_frames = GetNumFrames();
    for (U32 i = 0; i < num_frames-1; i++) {
        Frame frame = GetFrame(i);
		LpcAnalyzer::LPC_STATE state = (LpcAnalyzer::LPC_STATE)frame.mData2;
		if (state == LpcAnalyzer::LPC_STATE::START) {
			char time_str[128];
			AnalyzerHelpers::GetTimeString(frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128);
			S32 lpc_nibble_cnt = -1;
			ss << time_str << ",";
			while (state != LpcAnalyzer::LPC_STATE::TAR && lpc_nibble_cnt < 20) {
				Frame nextFrame = GetFrame(lpc_nibble_cnt + i + 1);
				state = (LpcAnalyzer::LPC_STATE)nextFrame.mData2;
				char nibble_str[128];
				snprintf(nibble_str, 7, "0b%c%c%c%c", NIBBLE_TO_BINARY(nextFrame.mData1));
				ss << nibble_str << ",";
				lpc_nibble_cnt++;
			}
			ss << std::endl;
			AnalyzerHelpers::AppendToFile((U8*)ss.str().c_str(), ss.str().length(), f);
			ss.str(std::string());
			i += lpc_nibble_cnt;
		}

        if (UpdateExportProgressAndCheckForCancel(i, num_frames) == true) {
            AnalyzerHelpers::EndFile(f);
            return;
        }
    }

    UpdateExportProgressAndCheckForCancel(num_frames, num_frames);
    AnalyzerHelpers::EndFile(f);
}

void LpcAnalyzerResults::GenerateFrameTabularText(U64 frame_index, DisplayBase display_base)
{
#define NIBBLE_TO_BINARY_PATTERN "%c%c%c%c"
#define NIBBLE_TO_BINARY(nibble)  \
	  (nibble & 0x08 ? '1' : '0'), \
	  (nibble & 0x04 ? '1' : '0'), \
	  (nibble & 0x02 ? '1' : '0'), \
	  (nibble & 0x01 ? '1' : '0') 

	ClearTabularText();
	Frame frame = GetFrame(frame_index);

	char state_str[128];
	char tab_str[128];

	LpcAnalyzer::LPC_STATE state = (LpcAnalyzer::LPC_STATE)frame.mData2;

	switch (state) {
	case LpcAnalyzer::LPC_STATE::START:
		memcpy(state_str, "START", 6);
		break;

	case LpcAnalyzer::LPC_STATE::CYCTYPE:
		switch (frame.mData1) {
		case 0b0000:
			memcpy(state_str, "IO READ", 8);
			break;
		case 0b0010:
			memcpy(state_str, "IO WRITE", 9);
			break;
		case 0b0100:
			memcpy(state_str, "MEM READ", 9);
			break;
		case 0b0110:
			memcpy(state_str, "MEM WRITE", 10);
			break;
		}

		break;

	case LpcAnalyzer::LPC_STATE::IO_READ_ADD:
	case LpcAnalyzer::LPC_STATE::IO_WRITE_ADD:
		snprintf(tab_str, 100, "ADDRESS (0x%04x)", (U32)frame.mData1);
		AddTabularText(tab_str);
		return;
	case LpcAnalyzer::LPC_STATE::MEM_READ_ADD:
	case LpcAnalyzer::LPC_STATE::MEM_WRITE_ADD:
		snprintf(tab_str, 100, "ADDRESS (0x%08x)", (U32)frame.mData1);
		AddTabularText(tab_str);
		return;
	case LpcAnalyzer::LPC_STATE::IO_READ_DATA:
	case LpcAnalyzer::LPC_STATE::IO_WRITE_DATA:
	case LpcAnalyzer::LPC_STATE::MEM_READ_DATA:
	case LpcAnalyzer::LPC_STATE::MEM_WRITE_DATA:
		snprintf(tab_str, 100, "DATA (0x%02x)", (U32)frame.mData1);
		AddTabularText(tab_str);
		return;

	case LpcAnalyzer::LPC_STATE::IO_READ_TAR:
	case LpcAnalyzer::LPC_STATE::IO_WRITE_TAR:
	case LpcAnalyzer::LPC_STATE::MEM_READ_TAR:
	case LpcAnalyzer::LPC_STATE::MEM_WRITE_TAR:
	case LpcAnalyzer::LPC_STATE::TAR:
		memcpy(state_str, "TAR", 4);
		break;

	case LpcAnalyzer::LPC_STATE::IO_READ_SYNC:
	case LpcAnalyzer::LPC_STATE::IO_WRITE_SYNC:
	case LpcAnalyzer::LPC_STATE::MEM_READ_SYNC:
	case LpcAnalyzer::LPC_STATE::MEM_WRITE_SYNC:
		memcpy(state_str, "SYNC", 5);
		break;

	case LpcAnalyzer::LPC_STATE::COMPLETE:
		memcpy(state_str, "COMPLETE", 9);
		break;

	default:
		memcpy(state_str, "UNKNOWN", 8);
		break;
	}

	if (frame.mFlags != 0) {
		snprintf(tab_str, 30, "ERROR");
	}
	else {
		snprintf(tab_str, 30, "%s (0b%c%c%c%c)", state_str, NIBBLE_TO_BINARY(frame.mData1));
	}
	AddTabularText(tab_str);
}

void LpcAnalyzerResults::GeneratePacketTabularText(U64 /*packet_id*/, DisplayBase /*display_base*/)    //unrefereced vars commented out to remove warnings.
{
    ClearResultStrings();
    AddResultString("not supported");
}

void LpcAnalyzerResults::GenerateTransactionTabularText(U64 /*transaction_id*/, DisplayBase /*display_base*/)    //unrefereced vars commented out to remove warnings.
{
    ClearResultStrings();
    AddResultString("not supported");
}
