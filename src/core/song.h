#pragma once

#include <tinydef.hpp>


/* Song struct
 * 
 * This contains all of the information about a Song that the program needs to know
 */

struct Song {
	// Definitions

	using Bpm = f32;

	struct TimeSig {
		i16 beatsPerMeasure;
		i16 noteValue;
	};


	Bpm bpm;
	TimeSig timeSig;


};