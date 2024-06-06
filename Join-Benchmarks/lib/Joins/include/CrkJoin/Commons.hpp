#ifndef COMMONS_HPP
#define COMMONS_HPP

namespace Commons
{

    template<typename T> inline int
    check_bit(T var, int pos) __attribute__((always_inline));

    inline int
    set_bit(int var, int pos) __attribute__((always_inline));

    inline int
    clear_bit(int var, int pos) __attribute__((always_inline));

    template<typename T>
    static inline T nextPowerOfTwo(T n) __attribute__((always_inline));

    template<typename T> inline int
    check_bit(T var, int pos)
    {
        return (var >> pos) & 1;
    }

    inline int
    set_bit(int var, int pos)
    {
        var |= 1 << pos;
        return var;
    }

    inline int
    clear_bit(int var, int pos)
    {
        var &= ~(1 << pos);
        return var;
    }

    template<typename T>
    static inline T nextPowerOfTwo(T n) {
        --n;
        for(T k=1;!(k&(1<<(sizeof(n)+1)));k<<=1) n|=n>>k;
        return ++n;
    }
}

#endif //COMMONS_HPP
