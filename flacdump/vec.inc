// flacdump: Vec<T> - growable array of plain structs (heap); Buf = Vec<u8>
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

template <class T> struct Vec {
    T* p = nullptr;
    size_t n = 0, cap = 0;
    void reserve(size_t c) {
        if (c <= cap) return;
        size_t nc = Max(c, Max(cap * 2, (size_t)64));
        p = (T*)xrealloc(p, nc * sizeof(T));
        cap = nc;
    }
    void push(const T& v) {
        if (n == cap) reserve(n + 1);
        p[n++] = v;
    }
    void append(const T* s, size_t k) {
        reserve(n + k);
        memcpy(p + n, s, k * sizeof(T));
        n += k;
    }
    T& operator[](size_t i) { return p[i]; }
    const T& operator[](size_t i) const { return p[i]; }
    T& back() { return p[n - 1]; }
    void release() {
        free(p);
        p = nullptr;
        n = cap = 0;
    }
};
typedef Vec<u8> Buf;
