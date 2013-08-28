#include <stdint.h>

//	A simple pseudo-random number generator
class MWCRand {
	public:
		static void Seed(int seed) {
			if (seed == 0) seed = (860 << 16) + 726;
			m_w = seed >> 16;
			m_z = seed & 65535;
			
			if (m_w == 0) m_w = seed + 860;
			if (m_z == 0) m_z = seed + 726;
		}

		static uint32_t GetRand() {
			m_z = 36969 * (m_z & 65535) + (m_z >> 16);
			m_w = 18000 * (m_w & 65535) + (m_w >> 16);
			return (m_z << 16) + m_w;
		}

	private:
		static uint32_t m_w;
		static uint32_t m_z;
};

