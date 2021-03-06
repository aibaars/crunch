// SPDX-License-Identifier: LGPL-3.0-or-later
#include <cstdint>
#include <random>
#include <substrate/utility>
#include "ranlux.h"

struct ranlux64Base_t final
{
private:
	constexpr static uint8_t wordSize = 64;
	constexpr static uint8_t shortLag = 5;
	constexpr static uint8_t longLag = 62;
	constexpr static uint32_t defaultSeed = 19780503U;

	std::array<uint64_t, longLag> x;
	uint8_t carry;
	uint8_t p;

public:
	ranlux64Base_t(const uint64_t seed) noexcept : x{}, carry{}, p{0}
	{
		std::linear_congruential_engine<uint64_t, 40014U, 0U, 2147483563U> lcg
			{seed ? seed : defaultSeed};

		for (auto &stateWord : x)
			stateWord = (lcg() & 0xFFFFFFFFU) + ((lcg() & 0xFFFFFFFFU) << 32U);
		carry = !x.back();
	}

	uint64_t operator ()() noexcept
	{
		int16_t shortIndex = p - shortLag;
		if (shortIndex < 0)
			shortIndex += longLag;

		const uint64_t newX{x[p] - x[shortIndex] - carry};
		carry = x[p] < x[shortIndex] + carry;
		x[p] = newX;
		if (++p >= longLag)
			p = 0;
		return newX;
	}

	void discard(uint16_t values)
	{
		while (values--)
			operator()();
	}
};

struct ranlux64_t final
{
private:
	ranlux64Base_t baseState;
	uint16_t blockUsed;

	constexpr static uint16_t luxury = 1303;
	constexpr static uint8_t blockSize = 62;

public:
	ranlux64_t(const uint64_t seed) noexcept : baseState{seed}, blockUsed{0} { }

	uint64_t operator ()() noexcept
	{
		if (blockUsed >= blockSize)
		{
			baseState.discard(luxury - blockUsed);
			blockUsed = 0;
		}
		++blockUsed;
		return baseState();
	}
};

ranlux64_t *initRanlux64(const uint64_t seed) try
	{ return substrate::make_unique<ranlux64_t>(seed).release(); }
catch (...)
	{ return nullptr; }

void freeRanlux64(ranlux64_t *const state)
	{ std::unique_ptr<ranlux64_t> ranluxState{state}; }

uint64_t genRanlux64(ranlux64_t *const state) { return (*state)(); }
