// flacdump: Vec<T> - growable array of plain structs (heap)
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

template <class T> struct Vec {
    T* p = nullptr;
    size_t n = 0, cap = 0;
    void push(const T& v) {
        if (n == cap) {
            cap = cap ? cap * 2 : 64;
            p = (T*)xrealloc(p, cap * sizeof(T));
        }
        p[n++] = v;
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
