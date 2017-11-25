#pragma once

#include "internal.hpp"

#include <iterator>

#include "vgcc/tree.h"

// GCC 4.5 doesn't support `for (item : container)` ye
#if G(4, 6)
# define foreach(e, c) for (e : c)
#else // G(4, 6)
# include <boost/foreach.hpp>
// usually replaced below to use container_wrapper
# define foreach(e, c) BOOST_FOREACH(e, c)
#endif

// Define C++-style iterators for versions of GCC still written in C.
#if !V(4, 8)

# if V(4, 7)
#  define num prefix.num
# endif
# define GENERIC_VEC_ITER_BEGIN(v) ((v) ? &(v)->base.vec[0] : nullptr)
# define GENERIC_VEC_ITER_END(v) ((v) ? &(v)->base.vec[(v)->base.num] : nullptr)

# if G(4, 6)

#  define MAKE_VEC_ITERATOR(T, A)               \
    inline T *begin(VEC(T, A) *v)               \
    {                                           \
        return GENERIC_VEC_ITER_BEGIN(v);       \
    }                                           \
    inline T *end(VEC(T, A) *v)                 \
    {                                           \
        return GENERIC_VEC_ITER_END(v);         \
    }                                           \
    inline const T *begin(const VEC(T, A) *v)   \
    {                                           \
        return GENERIC_VEC_ITER_BEGIN(v);       \
    }                                           \
    inline const T *end(const VEC(T, A) *v)     \
    {                                           \
        return GENERIC_VEC_ITER_END(v);         \
    }

# else // G(4, 6)

# undef foreach
# define foreach(e, c) BOOST_FOREACH(e, container_wrapper(c))

#include <type_traits>

// Work around the fact that Boost can't customize iteration over pointers.
template<class C, typename=typename std::enable_if<!std::is_const<C>::value>::type>
struct ContainerWrapper
{
    C *container;

    ContainerWrapper(C *c)
    : container(c)
    {
    }
};

template<class C>
struct ConstContainerWrapper
{
    const C *container;

    ConstContainerWrapper(const C *c)
    : container(c)
    {
    }
};

template<class C>
ContainerWrapper<C> container_wrapper(C *c)
{
    return ContainerWrapper<C>(c);
}

template<class C>
ConstContainerWrapper<C> container_wrapper(const C *c)
{
    return ConstContainerWrapper<C>(c);
}

// but other containers are left alone
template<class C, typename=typename std::enable_if<!std::is_pointer<C>::value>::type>
C&& container_wrapper(C&& c)
{
    return std::forward<C>(c);
}

#  define MAKE_VEC_ITERATOR(T, A)                                       \
    inline T *range_begin(ContainerWrapper<VEC(T, A)> v)                \
    {                                                                   \
        return GENERIC_VEC_ITER_BEGIN(v.container);                     \
    }                                                                   \
    inline T *range_end(ContainerWrapper<VEC(T, A)> v)                  \
    {                                                                   \
        return GENERIC_VEC_ITER_END(v.container);                       \
    }                                                                   \
    inline const T *range_begin(ConstContainerWrapper<VEC(T, A)> v)     \
    {                                                                   \
        return GENERIC_VEC_ITER_BEGIN(v.container);                     \
    }                                                                   \
    inline const T *range_end(ConstContainerWrapper<VEC(T, A)> v)       \
    {                                                                   \
        return GENERIC_VEC_ITER_END(v.container);                       \
    }                                                                   \
    namespace boost                                                     \
    {                                                                   \
        /* Yes, always const, since it is a temporary. */               \
        template<>                                                      \
        struct range_const_iterator<ContainerWrapper<VEC(T, A)>>        \
        {                                                               \
            typedef T *type;                                            \
        };                                                              \
        template<>                                                      \
        struct range_const_iterator<ConstContainerWrapper<VEC(T, A)>>   \
        {                                                               \
            typedef const T *type;                                      \
        };                                                              \
    }

# endif // G(4, 6)

MAKE_VEC_ITERATOR(alias_pair, gc);
MAKE_VEC_ITERATOR(constructor_elt, gc);
MAKE_VEC_ITERATOR(tree, gc);

#undef num

#else // !V(4, 8)
// still allow iterating over pointer-to-vec

# if !V(7)
template<class T, class A>
T *begin(vec<T, A>& v)
{
    return v.address();
}
template<class T, class A>
T *end(vec<T, A>& v)
{
    return v.address() + v.length();
}
template<class T, class A>
const T *begin(const vec<T, A>& v)
{
    return v.address();
}
template<class T, class A>
const T *end(const vec<T, A>& v)
{
    return v.address() + v.length();
}
# endif

template<class T, class A>
T *begin(vec<T, A> *v)
{
    using std::begin;
    return v ? begin(*v) : nullptr;
}

template<class T, class A>
T *end(vec<T, A> *v)
{
    using std::end;
    return v ? end(*v) : nullptr;
}

template<class T, class A>
const T *begin(const vec<T, A> *v)
{
    using std::begin;
    return v ? begin(*v) : nullptr;
}

template<class T, class A>
const T *end(const vec<T, A> *v)
{
    using std::end;
    return v ? end(*v) : nullptr;
}

#define MAKE_VEC_ITERATOR(T, A) /* nothing */

#endif // !V(4, 8)
