
long fibonacci(int depth) {
	if (depth == 0) {
		return 0;
	} else if (depth == 1) {
		return 1;
	} else {
		return fibonacci(depth - 2)
			+ fibonacci(depth - 1);
	}
}

void _start() {
	long result = fibonacci(32);
}
