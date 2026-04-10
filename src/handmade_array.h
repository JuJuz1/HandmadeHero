#ifndef HANDMADE_ARRAY_H
#define HANDMADE_ARRAY_H

template <typename T, memory_index N>
struct Array {
    T data_[N];
    NOT_BOUND constexpr memory_index size{ N };

    NODISCARD
    T&
    operator[](memory_index i) {
        ASSERT(i < size);
        return data_[i];
    }

    NODISCARD
    const T&
    operator[](memory_index i) const {
        ASSERT(i < size);
        return data_[i];
    }
};

#endif // HANDMADE_ARRAY_H
