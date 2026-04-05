#pragma once

#include <cstdint>

/**
 * @brief Header containing metadata for packets of audio sample data
 *
 */
struct PacketHeader
{
	uint32_t mSampleRate;
	uint32_t mNumFrames;
};