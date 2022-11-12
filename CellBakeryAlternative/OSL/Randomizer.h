#pragma once

namespace osl {
	class Randomaizer {
	public:

		inline uint64_t u64q() {
			return (seedu64[iziRAND()] * 0xCF64F50B731A51E3u) ^ 0x2DE599E383FB361Eu;
		}

		inline uint32_t u32q() {
			int index = iziRAND() * 2;
			return (seedu32[index] * 0xE1B27A83u) ^ seedu32[index + 1];
		}

		inline uint16_t u16q() {
			int index = iziRAND() * 4;
			return (seedu16[index] * 0x33C6DFEAu) ^ seedu16[index + 2] ^ seedu16[index / 3];
		}

		inline uint8_t u8q() {
			int index = iziRAND() * 8;
			return (seedu8[index] * 0x33C6DFEAu) ^ seedu8[index / 2] ^ seedu8[index / 5];
		}

		inline double pdq() { // [0.d .. 1.d]
			return u64q() / 18446744073709551615.;
		}

		inline float pfq() { // [0.f .. 1.f]
			return u32q() / 4294967295.f;
		}

		inline double ndq() { // [0.d .. 1.d]
			return (u64q() / 9223372036854775807.) - 1.;
		}

		inline float nfq() { // [0.f .. 1.f]
			return (u32q() / 2147483647.f) - 1.f;
		}

		inline uint64_t u64() {
			return seedu64[RAND()] ^ seedu64[RAND() >> 2];
		}

		inline uint32_t u32() {
			int index = RAND() * 2;
			return seedu32[index] ^ seedu32[index + 1];
		}

		inline uint16_t u16() {
			int index = RAND() * 4;
			return seedu16[index] ^ seedu16[index + 2] ^ seedu16[index + 3];
		}

		inline uint8_t u8() {
			int index = RAND();
			return seedu8[index * 8] ^ seedu8[index * 2 + 3] ^ seedu8[index * 3 + 4] ^ seedu8[index * 8 + 7];
		}

		inline double pd() { // [0.d .. 1.d]
			return u64() / 18446744073709551615.;
		}

		inline float pf() { // [0.f .. 1.f]
			return u32() / 4294967295.f;
		}

		inline double nd() { // [0.d .. 1.d]
			return (u64() / 9223372036854775807.) - 1.;
		}

		inline float nf() { // [0.f .. 1.f]
			return (u32() / 2147483647.f) - 1.f;
		}

		Randomaizer(int ns = 10000) {
			NSEED = ns;
			NSEED64 = ns;
			NSEED32 = ns * 2;
			NSEED16 = ns * 4;
			NSEED8 = ns * 8;
			seedu64 = new uint64_t[ns];
			seedu32 = reinterpret_cast<uint32_t*>(seedu64);
			seedu16 = reinterpret_cast<uint16_t*>(seedu64);
			seedu8 = reinterpret_cast<uint8_t*>(seedu64);
		}

		~Randomaizer() {
			delete[] seedu64;
		}

		void ini(uint64_t seed) { // инициализация по заданному сиду
			mseed = seed;
			for (uint64_t i = 0u; i < NSEED * 4; i++)
				seedu16[i] = uint16_t((i * 35u) ^ mseed);
			for (uint64_t i = 0u; i < NSEED * 40; i++)
				seedu64[i % NSEED] = ((seedu64[((i + 14124u) * 5u) % NSEED] + i * mseed) ^ seedu64[((i * 25161u) ^ (mseed - i)) % NSEED]);
			for (uint64_t i = 0u; i < NSEED * 10; i++)
				u64();

		}

		uint64_t ini() { // инициализация без сида, генерация сида
			uint64_t ps = uint64_t(&seedu64);
			uint8_t *u1 = new (uint8_t);
			ps ^= uint64_t(u1) << 32;
			ps ^= uint64_t(this);
			delete u1;
			for (uint64_t i = 0; i < NSEED; i++)
				ps ^= (seedu64[(i + ps) % NSEED]);
			ps = (ps ^ 0xA35D506F52CCD7AD);
			ini(ps);
			return mseed;
		}

		uint64_t getMainSeed() {
			return mseed;
		}

	private:
		uint64_t mseed = 0;
		uint64_t NSEED = 10000;
		uint64_t NSEED64 = 10000;
		uint64_t NSEED32 = 10000;
		uint64_t NSEED16 = 10000;
		uint64_t NSEED8 = 10000;

		uint64_t *seedu64;
		uint32_t *seedu32;
		uint16_t *seedu16;
		uint8_t *seedu8;
		uint32_t Gs = 0;
		uint64_t iteration = 0;

		int iziRAND() {
			iteration++;
			Gs = iteration % NSEED;
			seedu64[Gs] = ((iteration * 0xF0049FD71F747999u) + 0xB2E16F4F4AC4C623u) ^
				seedu64[seedu16[(iteration * 0x3DA5D0CC74961BD0u + mseed) % NSEED16] % NSEED64];
			return Gs;
		}

		int RAND() {
			iteration++;
			Gs = iteration % NSEED;
			seedu64[Gs] = (((seedu64[((iteration * 0x333E3F8E984B2123u) + seedu32[Gs + 1141u]) % NSEED64])
				* 0xC7EBE26262EF7C71u) ^ (iteration * 0x7AFA97FA31508F9Fu) ^ seedu64[(seedu16[Gs] + mseed) % NSEED64]) ^ mseed;
			return Gs;
		}
	};
}