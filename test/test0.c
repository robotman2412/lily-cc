
// This is to make vscode not complain about types.
#define auto int
#define func int

// A function that can be written in the extremely limited scope.
func mul(a, b) {
	auto c;
	c = a + b - 2;
	return c;
}
