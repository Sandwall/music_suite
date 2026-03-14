#pragma once

#include <tinydef.hpp>

/* song.h
*  ======
*  This file declares the majority of the general-purpose song containers
* 
 */


// I think I'll try some things where the the input method determines the frequency
struct Note {
	f32 offset; // relative to start of NoteItem
	f32 length; // relative to start of Note

	// maybe for now we can use log2(freq) to determine y position on piano roll
	f32 freq;
};

// TODO: I think we'll take a linked list approach to this?
// If you want to 
struct NoteBlock {
	NoteBlock* next = nullptr; // 8 bytes
	i32 padding = 0;

	Note notes[84];
};

struct NoteItem {
	// header
	f32 offset;
	f32 length;

	// TODO: calculates length by traversing `block`
	void calculate_length();

	NoteBlock* block;
};

// TODO: implement this
struct NotePoolAllocator {
	tds::Slice<NoteBlock> pool;

	void init();
	NoteBlock* get();
	void free(NoteBlock* block);
};

struct Song {
	// Definitions

	using Bpm = f32;

	struct TimeSig {
		i16 beatsPerMeasure;
		i16 noteValue;
	};

	// Storage

	Bpm bpm;
	TimeSig timeSig;
	mem::PoolAllocator<NoteBlock> notePool;

};