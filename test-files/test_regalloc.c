
int simple(int a, int b) {
    int c = a + b;
    int d = c + b;
    return d;
}

int loop(int a, int c) {
    int b = 1;

    while (a > 0) {
        b <<= c;
        a  -= 1;
    }

    return b;
}

int spill(int v0) {
    int v1  = v0 + 1;
    int v2  = v0 + 2;
    int v3  = v0 + 3;
    int v4  = v0 + 4;
    int v5  = v0 + 5;
    int v6  = v0 + 6;
    int v7  = v0 + 7;
    int v8  = v0 + 8;
    int v9  = v0 + 9;
    int v10 = v0 + 10;
    int v11 = v0 + 11;
    int v12 = v0 + 12;
    int v13 = v0 + 13;
    int v14 = v0 + 14;
    int v15 = v0 + 15;
    int v16 = v0 + 16;
    int v17 = v0 + 17;
    int v18 = v0 + 18;
    int v19 = v0 + 19;
    int v20 = v0 + 20;
    int v21 = v0 + 21;
    int v22 = v0 + 22;
    int v23 = v0 + 23;
    int v24 = v0 + 24;
    int v25 = v0 + 25;
    int v26 = v0 + 26;
    int v27 = v0 + 27;
    int v28 = v0 + 28;
    int v29 = v0 + 29;
    int v30 = v0 + 30;
    int v31 = v0 + 31;

    return v0 + v1 + v2 + v3 + v4 + v5 + v6 + v7 + v8 + v9 + v10 + v11 + v12 + v13 + v14 + v15 + v16 + v17 + v18 + v19
           + v20 + v21 + v22 + v23 + v24 + v25 + v26 + v27 + v28 + v29 + v30 + v31;
}
