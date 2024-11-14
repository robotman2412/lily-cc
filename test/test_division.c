
unsigned int divide(unsigned int remainder, unsigned int divisor) {
	// Do some left shifting.
	unsigned int max, out;
	max = 1;
	out = 0;
	while (divisor < remainder) {
		divisor <<= 1;
		max += 1;
	}
	
	// Division loop.
	while (max > 0) {
		out <<= 1;
		max -= 1;
		if (remainder >= divisor) {
			remainder -= divisor;
			out |= 1;
		}
		divisor >>= 1;
	}
	
	return out;
}
