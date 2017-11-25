#pragma once


template<class T>
struct add_pointer_const
{
    typedef T type;
};
template<class E>
struct add_pointer_const<E *>
{
    // recurse
    typedef typename add_pointer_const<E>::type E_const;
    typedef const E_const *type;
};

// TODO this probably should go away
template<class T>
struct is_string : std::integral_constant<bool, std::is_same<T, char *>::value || std::is_same<T, const char *>::value>
{
};
