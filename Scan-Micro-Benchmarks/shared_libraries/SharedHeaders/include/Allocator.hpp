#ifndef SGX_ENCRYPTEDDB_ALLOCATOR_HPP
#define SGX_ENCRYPTEDDB_ALLOCATOR_HPP

#include <limits>
#include <memory>
#include <array>
#include <functional>
#include <random>

#include <limits>
#include <new>

/**
 * Returns aligned pointers when allocations are requested. Default alignment
 * is 64B = 512b, sufficient for AVX-512 and most cache line sizes.
 *
 * @tparam ALIGNMENT_IN_BYTES Must be a positive power of 2.
 */
template<typename    ElementType,
        std::size_t ALIGNMENT_IN_BYTES = 64>
class AlignedAllocator
{
private:
    static_assert(
            ALIGNMENT_IN_BYTES >= alignof( ElementType ),
            "Beware that types like int have minimum alignment requirements "
            "or access will result in crashes."
    );

public:
    using value_type = ElementType;

    static std::align_val_t constexpr ALIGNMENT{ALIGNMENT_IN_BYTES };

    /**
     * This is only necessary because AlignedAllocator has a second template
     * argument for the alignment that will make the default
     * std::allocator_traits implementation fail during compilation.
     * @see https://stackoverflow.com/a/48062758/2191065
     */
    template<class OtherElementType>
    struct rebind
    {
        using other = AlignedAllocator<OtherElementType, ALIGNMENT_IN_BYTES>;
    };

public:
    constexpr AlignedAllocator() noexcept = default;

    constexpr AlignedAllocator( const AlignedAllocator& ) noexcept = default;

    constexpr AlignedAllocator(AlignedAllocator && ) noexcept = default;

    constexpr AlignedAllocator& operator=(AlignedAllocator& other) noexcept = default;

    template<typename U>
    constexpr AlignedAllocator( AlignedAllocator<U, ALIGNMENT_IN_BYTES> const& ) noexcept
    {}

    [[nodiscard]] ElementType*
    allocate( std::size_t nElementsToAllocate )
    {
        if ( nElementsToAllocate
             > std::numeric_limits<std::size_t>::max() / sizeof( ElementType ) ) {
            throw std::bad_alloc();
        }

        auto const nBytesToAllocate = nElementsToAllocate * sizeof( ElementType );
        return reinterpret_cast<ElementType*>(
                ::operator new[]( nBytesToAllocate, ALIGNMENT ) );
    }

    void
    deallocate(ElementType* allocatedPointer, [[maybe_unused]] std::size_t  nBytesAllocated )
    {
        /* According to the C++20 draft n4868 § 17.6.3.3, the delete operator
         * must be called with the same alignment argument as the new expression.
         * The size argument can be omitted but if present must also be equal to
         * the one used in new. */
        ::operator delete[]( allocatedPointer, ALIGNMENT );
    }

    template<typename U, std::size_t OTHER_ALIGNMENT>
    constexpr bool operator==(const AlignedAllocator<U, OTHER_ALIGNMENT>&) const {
        return std::is_same_v<ElementType, U> && ALIGNMENT_IN_BYTES == OTHER_ALIGNMENT;
    }

    template<typename U, std::size_t OTHER_ALIGNMENT>
    constexpr bool operator!=(const AlignedAllocator<U, OTHER_ALIGNMENT>& other) const {
        return !operator!=(other);
    }
};

template<typename T, int alignment=64>
T* allocate_data_array_aligned(size_t num_total_entries) {
    static_assert(std::is_arithmetic_v<T>, "Data-Array can only be allocated for numeric types");
    T* data = static_cast<T*>(aligned_alloc(alignment, num_total_entries * sizeof(T)));
    if constexpr (std::is_same_v<uint8_t, T>) {
        std::array<uint8_t, 256> tmp {0};
        for (uint8_t i = 0; i < 255; ++i) {
            tmp[i] = i;
        }
        tmp[255] = static_cast<uint8_t>(255);

        auto num_copies = num_total_entries / 256;

        for (size_t i = 0; i < num_copies; ++i) {
            std::copy(tmp.begin(), tmp.end(), data.get() + i * 256);
        }

    } else {
        for (uint64_t i = 0; i < num_total_entries; ++i) {
            data[i] = static_cast<T>(i % (static_cast<uint64_t>(std::numeric_limits<T>::max()) + 1ULL));
        }
    }
    return data;
}

template<typename T, int alignment=64>
std::unique_ptr<T[]> allocate_data_array_aligned(size_t num_total_entries, std::function<T(uint64_t)> data_generator) {
    static_assert(std::is_arithmetic_v<T>, "Data-Array can only be allocated for numeric types");
    std::unique_ptr<T[]> data {new (std::align_val_t{alignment}) T[num_total_entries]};
    if constexpr (std::is_same_v<uint8_t, T>) {
        std::array<uint8_t, 256> tmp {0};
        for (uint8_t i = 0; i < 255; ++i) {
            tmp[i] = data_generator(i);
        }
        tmp[255] = static_cast<uint8_t>(255);

        auto num_copies = num_total_entries / 256;

        for (size_t i = 0; i < num_copies; ++i) {
            std::copy(tmp.begin(), tmp.end(), data.get() + i * 256);
        }

    } else {
        for (uint64_t i = 0; i < num_total_entries; i++) {
            data[i] = data_generator(i);
        }
    }
    return data;
}

template<typename T, int alignment=64>
std::unique_ptr<T[]> allocate_random_uniform(size_t num_total_entries, std::uint_fast64_t seed = 0) {
    std::random_device rnd_device;
    std::mt19937_64 mersenne_engine{seed ? seed : rnd_device()};  // Generates random integers
    std::uniform_int_distribution<T> uniform_distribution{std::numeric_limits<T>::min(), std::numeric_limits<T>::max()};

    auto uniform_int_generator = [&uniform_distribution, &mersenne_engine](uint64_t) {
        return uniform_distribution(mersenne_engine);
    };

    return allocate_data_array_aligned<T, alignment>(num_total_entries, uniform_int_generator);
}

#endif //SGX_ENCRYPTEDDB_ALLOCATOR_HPP
