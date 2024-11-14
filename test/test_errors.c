
int func() {
	// Should error.
	if (undefinedvar) {
		return 1;
	}
}

int next() {
	int q;
	// Should error.
	192 = q;
	3 <<= q;
	// Shouldn't error.
	*192 = 2;
	1[2] = 3;
}

void call() {
	// Should warn implicit declaration.
	undefinedfunc();
	// Shouldn't warn.
	next();
}

// Duplicate definition of call().
void call();

void arg_error(int q);
void arg_error();
void arg_error(int q, int p) {
	char c;
	// Long char constant.
	c = 'qqq';
	// Empty char constant.
	c = '';
}

// Unrecognised token.
`
