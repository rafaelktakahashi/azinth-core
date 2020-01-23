#pragma once

#include "pch.h"

// Scancode is a data type. No cpp implementation file exists.

namespace Azinth
{

	// This structure uniquely represents a physical key on a keyboard;
	// a scancode is represented by a single byte, optionally prefixed
	// by the 0xE0 flag (Pause/Break is the only key that uses the 0xE1 prefix instead)
	struct Scancode
	{
		bool flgE0 : 1;
		bool flgE1 : 1;
		BYTE makeCode;

		Scancode() :
			flgE0(false), flgE1(false), makeCode(0)
		{}

		Scancode(bool E1, bool E0, BYTE makeCode) :
			flgE0(E0), flgE1(E1), makeCode(makeCode)
		{}

		Scancode(BYTE prefix, BYTE makeCode) :
			Scancode(prefix == 0xe1, prefix == 0xe0, makeCode)
		{}

		Scancode(BYTE makeCode) : Scancode(false, false, makeCode)
		{}

	};

	// operators
	inline bool operator==(const Scancode& lhs, const Scancode& rhs)
	{
		return (
			lhs.flgE0 != rhs.flgE0 ? false :		// If the E0 flag is different, == returns false
			lhs.flgE1 != rhs.flgE1 ? false :		// else, if the E1 flag is different, == returns false
			lhs.makeCode == rhs.makeCode			// else, compare the scancodes.
			);
	}
	inline bool operator!=(const Scancode& lhs, const Scancode& rhs)
	{
		return (
			lhs.flgE0 == rhs.flgE0 ? false :
			lhs.flgE1 == rhs.flgE1 ? false :
			lhs.makeCode != rhs.makeCode
			);
	}
	inline bool operator<(const Scancode& lhs, const Scancode& rhs)
	{
		return ((lhs.makeCode) ^ (lhs.flgE0 << 8) ^ (lhs.flgE1 << 9))
			< ((rhs.makeCode) ^ (rhs.flgE0 << 8) ^ (rhs.flgE1 << 9));
	}
	inline bool operator>(const Scancode& lhs, const Scancode& rhs)
	{
		return ((lhs.makeCode) ^ (lhs.flgE0 << 8) ^ (lhs.flgE1 << 9))
	> ((rhs.makeCode) ^ (rhs.flgE0 << 8) ^ (rhs.flgE1 << 9));
	}
	inline bool operator<=(const Scancode& lhs, const Scancode& rhs) { return !operator>(lhs, rhs); }
	inline bool operator>=(const Scancode& lhs, const Scancode& rhs) { return !operator<(lhs, rhs); }

}


namespace std {
	// Hash specialization for Scancode so that it may be used in a hash map (std::unordered_map).
	// It's okay to change. Maps are constructed every time a config file is read.
	template<>
	struct hash<Azinth::Scancode>
	{
		size_t operator()(const Azinth::Scancode& x) const
		{
			return (hash<unsigned short>()(
				(x.makeCode) ^ (x.flgE0 << 8) ^ (x.flgE1 << 9)
				));
		}
	};
}
