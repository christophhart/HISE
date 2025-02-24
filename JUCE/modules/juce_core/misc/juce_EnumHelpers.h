#define JUCE_DECLARE_SCOPED_ENUM_BITWISE_OPERATORS(EnumType)               \
    inline EnumType operator& (EnumType a, EnumType b) noexcept            \
    {                                                                      \
        return static_cast<EnumType> (static_cast<int> (a) & static_cast<int> (b)); \
    }                                                                      \
    inline EnumType operator| (EnumType a, EnumType b) noexcept            \
    {                                                                      \
        return static_cast<EnumType> (static_cast<int> (a) | static_cast<int> (b)); \
    }                                                                      \
    inline EnumType operator~ (EnumType a) noexcept                        \
    {                                                                      \
        return static_cast<EnumType> (~static_cast<int> (a));              \
    }                                                                      \
    inline EnumType& operator|= (EnumType& a, EnumType b) noexcept         \
    {                                                                      \
        a = a | b;                                                         \
        return a;                                                          \
    }                                                                      \
    inline EnumType& operator&= (EnumType& a, EnumType b) noexcept         \
    {                                                                      \
        a = a & b;                                                         \
        return a;                                                          \
    }

namespace juce
{

template <typename EnumType>
inline bool hasBitValueSet (EnumType enumValue, EnumType valueToLookFor) noexcept
{
    return (static_cast<int> (enumValue) & static_cast<int> (valueToLookFor)) != 0;
}

template <typename EnumType>
inline EnumType withBitValueSet (EnumType enumValue, EnumType valueToAdd) noexcept
{
    return static_cast<EnumType> (static_cast<int> (enumValue) | static_cast<int> (valueToAdd));
}

template <typename EnumType>
inline EnumType withBitValueCleared (EnumType enumValue, EnumType valueToRemove) noexcept
{
    return static_cast<EnumType> (static_cast<int> (enumValue) & ~static_cast<int> (valueToRemove));
}

} // namespace juce
