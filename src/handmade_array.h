#ifndef HANDMADE_ARRAY_H
#define HANDMADE_ARRAY_H

/**
 * A simple array wrapper to make life a bit easier with arrays
 * Simply made so that we have to worry about arrays decaying to pointers a bit less!
 * A rare case of template usage (here it is very useful!) in this codebase
 */
template <typename T, memory_index N>
struct Array {
    // Don't allow even non-standard extensions
    // MSVC checks for >= 0 by default at least
    static_assert(N > 0, "Array size must be greater than 0!");

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
