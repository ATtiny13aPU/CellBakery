#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <math.h>
#include <cassert>
#include <type_traits>
#include <iostream>

// -std=c++14
namespace JIO {

    template<typename T, size_t size>
    class Vector;

    template<typename R, bool v>
    using p_enable_if_t = typename std::enable_if<v, R>::type;

#define p_enable_if(B) p_enable_if_t<bool, (B)> = false

    struct p_v_unused {
    };

    template<size_t size1, size_t size2, size_t index1, size_t index2,
    size_t length, size_t counter, typename T1, typename T2, p_enable_if(counter == length)>
    constexpr inline void p_copy_array(T1*, const T2*) { }

    template<size_t size1, size_t size2, size_t index1, size_t index2,
    size_t length, size_t counter, typename T1, typename T2, p_enable_if(counter != length)>
    constexpr inline void p_copy_array(T1 *array1, const T2 *array2) {
        array1[index1 + counter] = T1(array2[index2 + counter]);
        p_copy_array<size1, size2, index1, index2, length, counter + 1 > (
                array1, array2);
    }

    template<typename T, size_t size>
    class Vector {
    private:
        T data[size];

        template<size_t index>
        constexpr inline void assign() {
            static_assert(index == size, "Wrong arguments length");
        }

        template<size_t index, typename T2, size_t size2, typename... Tp>
        constexpr inline void assign(const Vector<T2, size2> &v, Tp... arr) {
            static_assert(index + size2 <= size, "Wrong arguments length");
            p_copy_array<size, size2, index, 0, size2, 0>(data, v.data);
            assign < index + size2 > (arr...);
        }

        template<size_t index, typename T2, typename... Tp,
        //Нужно, чтобы отсечь не приводимые к T типы
        typename V = decltype(T(std::declval<T2>()))>
        constexpr inline void assign(const T2 e, Tp... arr) {
            static_assert(index + 1 <= size, "Wrong arguments length");
            data[index] = T(e);
            assign < index + 1 > (arr...);
        }

        template<size_t index, p_enable_if(index == size)>
        constexpr inline void assignOne(T) { }

        template<size_t index, p_enable_if(index != size)>
        constexpr inline void assignOne(T e) {
            data[index] = e;
            assignOne < index + 1 > (e);
        }
    public:

        constexpr inline Vector(p_v_unused) { }

        constexpr inline Vector() {
            assignOne<0>(T());
        }

        template<typename Tp>
        constexpr explicit inline Vector(const Tp v) {
            assignOne<0>(v);
        }

        template<typename Tp>
        constexpr explicit inline Vector(const Vector<Tp, size> &v) {
            assign<0>(v);
        }

        template<typename... Tp>
        constexpr explicit inline Vector(Tp... arr) {
            assign<0>(arr...);
        }

        constexpr inline const T& operator[](size_t index) const {
            assert(index < size);
            return data[index];
        }

        constexpr inline T& operator[](size_t index) {
            assert(index < size);
            return data[index];
        }

        template<typename T1, size_t size1>
        friend class Vector;
    };

    template<typename T, size_t size>
    std::ostream& operator<<(std::ostream &out, Vector<T, size> v) {
        out << "{";
        if (size > 0) {
            out << v[0];
            for (size_t i = 1; i < size; i++) {
                out << ", " << v[i];
            }
        }
        out << "}";
        return out;
    }

#define OP_TYPE(op, T1, T2) decltype(std::declval<T1>() op std::declval<T2>())

#ifdef VECTOR_USE_RECURSIVE_OPERATIONS
#define BIN_VV_OPERATOR_H(hname, op)                              \
template<size_t index = 0, typename T1, typename T2, typename T3, \
size_t size, p_enable_if(index == size)>                          \
constexpr inline void hname(const Vector<T1, size> &v1,           \
        const Vector<T2, size> &v2, Vector<T3, size> &out) { }    \
template<size_t index = 0, typename T1, typename T2, typename T3, \
size_t size, p_enable_if(index != size)>                          \
constexpr inline void hname(const Vector<T1, size> &v1,           \
        const Vector<T2, size> &v2, Vector<T3, size> &out) {      \
    out[index] = v1[index] op v2[index];                          \
    hname < index + 1 > (v1, v2, out);                            \
}
#else
#define BIN_VV_OPERATOR_H(hname, op)                              \
template<typename T1, typename T2, typename T3, size_t size>      \
constexpr inline void hname(const Vector<T1, size> &v1,           \
        const Vector<T2, size> &v2, Vector<T3, size> &out) {      \
    for(size_t i = 0; i < size; i++){                             \
        out[i] = v1[i] op v2[i];                                  \
    }                                                             \
}
#endif

#define BIN_VV_OPERATOR(hname, op)                                \
BIN_VV_OPERATOR_H(hname, op)                                      \
template<typename T1, typename T2, size_t size>                   \
constexpr inline Vector<OP_TYPE(op, T1, T2), size> operator op(   \
        const Vector<T1, size> &v1,                               \
        const Vector<T2, size> &v2) {                             \
    Vector<OP_TYPE(op, T1, T2), size> out((p_v_unused()));        \
    hname(v1, v2, out);                                           \
    return out;                                                   \
}

    BIN_VV_OPERATOR(p_plus_h, +)
    BIN_VV_OPERATOR(p_sub_h, -)
    BIN_VV_OPERATOR(p_mul_h, *)
    BIN_VV_OPERATOR(p_div_h, /)
    BIN_VV_OPERATOR(p_rem_h, %)
    BIN_VV_OPERATOR(p_or_h, |)
    BIN_VV_OPERATOR(p_and_h, &)
    BIN_VV_OPERATOR(p_xor_h, ^)
    BIN_VV_OPERATOR(p_l_and_h, &&)
    BIN_VV_OPERATOR(p_l_or_h, ||)
    BIN_VV_OPERATOR(p_shl_h, <<)
    BIN_VV_OPERATOR(p_shr_h, >>)

#undef BIN_VV_OPERATOR_H
#undef BIN_VV_OPERATOR

#ifdef VECTOR_USE_RECURSIVE_OPERATIONS
#define BIN_VT_OPERATOR_H(hname, op)                              \
template<size_t index = 0, typename T1, typename T2, typename T3, \
size_t size, p_enable_if(index == size)>                          \
constexpr inline void hname(const Vector<T1, size> &v1,           \
        const T2 v2, Vector<T3, size> &out) { }                   \
template<size_t index = 0, typename T1, typename T2, typename T3, \
size_t size, p_enable_if(index != size)>                          \
constexpr inline void hname(const Vector<T1, size> &v1,           \
        const T2 v2, Vector<T3, size> &out) {                     \
    out[index] = v1[index] op v2;                                 \
    hname < index + 1 > (v1, v2, out);                            \
}
#else
#define BIN_VT_OPERATOR_H(hname, op)                              \
template<typename T1, typename T2, typename T3, size_t size>      \
constexpr inline void hname(const Vector<T1, size> &v1,           \
        const T2 v2, Vector<T3, size> &out) {                     \
    for(size_t i = 0; i < size; i++){                             \
        out[i] = v1[i] op v2;                                     \
    }                                                             \
}
#endif

#define BIN_VT_OPERATOR(hname, op)                                \
BIN_VT_OPERATOR_H(hname, op)                                      \
template<typename T1, typename T2, size_t size>                   \
constexpr inline Vector<OP_TYPE(op, T1, T2), size> operator op(   \
        const Vector<T1, size> &v1, const T2 v2) {                \
    Vector<OP_TYPE(op, T1, T2), size> out((p_v_unused()));        \
    hname(v1, v2, out);                                           \
    return out;                                                   \
}

    BIN_VT_OPERATOR(p_plus_h, +)
    BIN_VT_OPERATOR(p_sub_h, -)
    BIN_VT_OPERATOR(p_mul_h, *)
    BIN_VT_OPERATOR(p_div_h, /)
    BIN_VT_OPERATOR(p_rem_h, %)
    BIN_VT_OPERATOR(p_or_h, |)
    BIN_VT_OPERATOR(p_and_h, &)
    BIN_VT_OPERATOR(p_xor_h, ^)
    BIN_VT_OPERATOR(p_l_and_h, &&)
    BIN_VT_OPERATOR(p_l_or_h, ||)
    BIN_VT_OPERATOR(p_shl_h, <<)
    BIN_VT_OPERATOR(p_shr_h, >>)

#undef BIN_VT_OPERATOR_H
#undef BIN_VT_OPERATOR

#ifdef VECTOR_USE_RECURSIVE_OPERATIONS
#define BIN_TV_OPERATOR_H(hname, op)                              \
template<size_t index = 0, typename T1, typename T2, typename T3, \
size_t size, p_enable_if(index == size)>                          \
constexpr inline void hname(const T1 v1,                          \
        const Vector<T2, size> &v2, Vector<T3, size> &out) { }    \
template<size_t index = 0, typename T1, typename T2, typename T3, \
size_t size, p_enable_if(index != size)>                          \
constexpr inline void hname(const T1 v1,                          \
        const Vector<T2, size> &v2, Vector<T3, size> &out) {      \
    out[index] = v1 op v2[index];                                 \
    hname < index + 1 > (v1, v2, out);                            \
}
#else
#define BIN_TV_OPERATOR_H(hname, op)                              \
template<typename T1, typename T2, typename T3, size_t size>      \
constexpr inline void hname(const T1 v1,                          \
        const Vector<T2, size> &v2, Vector<T3, size> &out) {      \
    for(size_t i = 0; i < size; i++){                             \
        out[i] = v1 op v2[i];                                     \
    }                                                             \
}
#endif

#define BIN_TV_OPERATOR(hname, op)                                \
BIN_TV_OPERATOR_H(hname, op)                                      \
template<typename T1, typename T2, size_t size>                   \
constexpr inline Vector<OP_TYPE(op, T1, T2), size> operator op(   \
        const T1 v1, const Vector<T2, size> &v2) {                \
    Vector<OP_TYPE(op, T1, T2), size> out((p_v_unused()));        \
    hname(v1, v2, out);                                           \
    return out;                                                   \
}

    BIN_TV_OPERATOR(p_plus_h, +)
    BIN_TV_OPERATOR(p_sub_h, -)
    BIN_TV_OPERATOR(p_mul_h, *)
    BIN_TV_OPERATOR(p_div_h, /)
    BIN_TV_OPERATOR(p_rem_h, %)
    BIN_TV_OPERATOR(p_or_h, |)
    BIN_TV_OPERATOR(p_and_h, &)
    BIN_TV_OPERATOR(p_xor_h, ^)
    BIN_TV_OPERATOR(p_l_and_h, &&)
    BIN_TV_OPERATOR(p_l_or_h, ||)
    BIN_TV_OPERATOR(p_shl_h, <<)
    BIN_TV_OPERATOR(p_shr_h, >>)

#undef BIN_TV_OPERATOR_H
#undef BIN_TV_OPERATOR

#ifdef VECTOR_USE_RECURSIVE_OPERATIONS
#define UNARY_V_OPERATOR_H(hname, op)                             \
template<size_t index = 0, typename T1, typename T2,              \
size_t size, p_enable_if(index == size)>                          \
constexpr inline void hname(const Vector<T1, size> &v1,           \
        Vector<T2, size> &out) { }                                \
template<size_t index = 0, typename T1, typename T2,              \
size_t size, p_enable_if(index != size)>                          \
constexpr inline void hname(const Vector<T1, size> &v1,           \
        Vector<T2, size> &out) {                                  \
    out[index] = op v1[index];                                    \
    hname < index + 1 > (v1, out);                                \
}
#else
#define UNARY_V_OPERATOR_H(hname, op)                             \
template<typename T1, typename T2, size_t size>                   \
constexpr inline void hname(const Vector<T1, size> &v1,           \
        Vector<T2, size> &out) {                                  \
    for(size_t i = 0; i < size; i++){                             \
        out[i] = op v1[i];                                        \
    }                                                             \
}
#endif

#define UNARY_V_OPERATOR(hname, op)                               \
UNARY_V_OPERATOR_H(hname, op)                                     \
template<typename T, size_t size>                                 \
constexpr inline Vector<T, size> operator op(                     \
        const Vector<T, size> &v1) {                              \
    Vector<T, size> out((p_v_unused()));                          \
    hname(v1, out);                                               \
    return out;                                                   \
}

    template<typename T, size_t size>
    constexpr inline Vector<T, size> operator+(
            const Vector<T, size> &v1) {
        return v1;
    }

    UNARY_V_OPERATOR(p_sub_h, -)
    UNARY_V_OPERATOR(p_neg_h, ~)
    UNARY_V_OPERATOR(p_not_h, !)

#undef UNARY_V_OPERATOR_H
#undef UNARY_V_OPERATOR

#ifdef VECTOR_USE_RECURSIVE_OPERATIONS
#define ASSIGN_VV_OPERATOR_H(hname, op)                           \
template<size_t index = 0, typename T1, typename T2,              \
size_t size, p_enable_if(index == size)>                          \
constexpr inline void hname(Vector<T1, size> &v1,                 \
        const Vector<T2, size> &v2) { }                           \
template<size_t index = 0, typename T1, typename T2,              \
size_t size, p_enable_if(index != size)>                          \
constexpr inline void hname(Vector<T1, size> &v1,                 \
        const Vector<T2, size> &v2) {                             \
    v1[index] op v2[index];                                       \
    hname < index + 1 > (v1, v2);                                 \
}
#else
#define ASSIGN_VV_OPERATOR_H(hname, op)                           \
template<typename T1, typename T2, size_t size>                   \
constexpr inline void hname(Vector<T1, size> &v1,                 \
        const Vector<T2, size> &v2) {                             \
    for(size_t i = 0; i < size; i++){                             \
        v1[i] op v2[i];                                           \
    }                                                             \
}
#endif

#define ASSIGN_VV_OPERATOR(hname, op)                             \
ASSIGN_VV_OPERATOR_H(hname, op)                                   \
template<typename T1, typename T2, size_t size>                   \
constexpr inline Vector<T1, size>& operator op(                   \
        Vector<T1, size> &v1, const Vector<T2, size> &v2) {       \
    hname(v1, v2);                                                \
    return v1;                                                    \
}

    ASSIGN_VV_OPERATOR(p_a_plus_h, +=)
    ASSIGN_VV_OPERATOR(p_a_sub_h, -=)
    ASSIGN_VV_OPERATOR(p_a_mul_h, *=)
    ASSIGN_VV_OPERATOR(p_a_div_h, /=)
    ASSIGN_VV_OPERATOR(p_a_rem_h, %=)
    ASSIGN_VV_OPERATOR(p_a_or_h, |=)
    ASSIGN_VV_OPERATOR(p_a_and_h, &=)
    ASSIGN_VV_OPERATOR(p_a_xor_h, ^=)
    ASSIGN_VV_OPERATOR(p_a_shl_h, <<=)
    ASSIGN_VV_OPERATOR(p_a_shr_h, >>=)

#undef ASSIGN_VV_OPERATOR_H
#undef ASSIGN_VV_OPERATOR

#ifdef VECTOR_USE_RECURSIVE_OPERATIONS
#define ASSIGN_VT_OPERATOR_H(hname, op)                           \
template<size_t index = 0, typename T1, typename T2,              \
size_t size, p_enable_if(index == size)>                          \
constexpr inline void hname(Vector<T1, size> &v1, const T2 v2) { }\
template<size_t index = 0, typename T1, typename T2,              \
size_t size, p_enable_if(index != size)>                          \
constexpr inline void hname(Vector<T1, size> &v1, const T2 v2) {  \
    v1[index] op v2;                                              \
    hname < index + 1 > (v1, v2);                                 \
}
#else
#define ASSIGN_VT_OPERATOR_H(hname, op)                           \
template<typename T1, typename T2, size_t size>                   \
constexpr inline void hname(Vector<T1, size> &v1,                 \
        const T2 v2) {                                            \
    for(size_t i = 0; i < size; i++){                             \
        v1[i] op v2;                                              \
    }                                                             \
}
#endif

#define ASSIGN_VT_OPERATOR(hname, op)                             \
ASSIGN_VT_OPERATOR_H(hname, op)                                   \
template<typename T1, typename T2, size_t size>                   \
constexpr inline Vector<T1, size>& operator op(                   \
        Vector<T1, size> &v1, const T2 v2) {                      \
    hname(v1, v2);                                                \
    return v1;                                                    \
}

    ASSIGN_VT_OPERATOR(p_a_plus_h, +=)
    ASSIGN_VT_OPERATOR(p_a_sub_h, -=)
    ASSIGN_VT_OPERATOR(p_a_mul_h, *=)
    ASSIGN_VT_OPERATOR(p_a_div_h, /=)
    ASSIGN_VT_OPERATOR(p_a_rem_h, %=)
    ASSIGN_VT_OPERATOR(p_a_or_h, |=)
    ASSIGN_VT_OPERATOR(p_a_and_h, &=)
    ASSIGN_VT_OPERATOR(p_a_xor_h, ^=)
    ASSIGN_VT_OPERATOR(p_a_shl_h, <<=)
    ASSIGN_VT_OPERATOR(p_a_shr_h, >>=)

#undef ASSIGN_VT_OPERATOR_H
#undef ASSIGN_VT_OPERATOR

#undef OP_TYPE

    template<typename T, size_t size>
    constexpr inline T dot(const Vector<T, size> &v) {
        T out = T();
        for (size_t i = 0; i < size; i++) {
            T tmp(v[i]);
            out += tmp * tmp;
        }
        return out;
    }

	template<typename T1, size_t size, typename T2, typename T3>
	constexpr inline Vector<T1, size> clamp(const Vector<T1, size> &v, T2 lo2, T3 hi2) {
		T1 lo = T1(lo2), hi = T1(hi2);
		Vector<T1, size> out((p_v_unused()));
		for (size_t i = 0; i < size; i++) {
			T1 value = v[i];
			out[i] = value < lo ? lo : (value > hi ? hi : value);
		}
		return out;
	}

    template<typename T1, size_t size, typename T2>
    constexpr inline Vector<T1, size> mix(
            const Vector<T1, size> &v1, const Vector<T1, size> &v2, T2 m) {
        T1 k2 = T1(1) - m;
        Vector<T1, size> out((p_v_unused()));
        for (size_t i = 0; i < size; i++) {
            out[i] = v1[i] * k2 + v2[i] * m;
        }
        return out;
    }

    template<typename T, size_t size>
    constexpr inline Vector<T, size> remap(const Vector<T, size> &v,
            T lo_in, T hi_in, T lo_out, T hi_out) {
        return (clamp(v, lo_in, hi_in) - lo_in) *
                (hi_out - lo_out) / (hi_in - lo_in) + lo_out;
    }

    template<typename T, size_t size>
    constexpr inline T length(const Vector<T, size> &v) {
        return sqrt(dot(v));
    }

#define SIMPLE_V_FUNCTION(fname, std_fname)                       \
template<typename T, size_t size>                                 \
constexpr inline Vector<T, size> fname(const Vector<T, size> &v) {\
    Vector<T, size> out((p_v_unused()));                            \
    for (size_t i = 0; i < size; i++) {                           \
        out[i] = std_fname(v[i]);                                 \
    }                                                             \
    return out;                                                   \
}
	

    SIMPLE_V_FUNCTION(abs, abs);
    SIMPLE_V_FUNCTION(sqrt, sqrt);
    SIMPLE_V_FUNCTION(sin, sin);
    SIMPLE_V_FUNCTION(cos, cos);
    SIMPLE_V_FUNCTION(tan, tan);
    SIMPLE_V_FUNCTION(asin, asin);
    SIMPLE_V_FUNCTION(acos, acos);
	SIMPLE_V_FUNCTION(atan, atan);
	SIMPLE_V_FUNCTION(floor, floor);
	SIMPLE_V_FUNCTION(fract, v[i] - floor);

    template<size_t size>
    constexpr inline bool any(const Vector<bool, size> &v) {
        bool out = false;
        for (size_t i = 0; (!out) && (i < size); i++) {
            out |= v[i];
        }
        return out;
    }

    template<size_t size>
    constexpr inline bool all(const Vector<bool, size> &v) {
        bool out = true;
        for (size_t i = 0; out && (i < size); i++) {
            out &= v[i];
        }
        return out;
    }

#undef SIMPLE_V_FUNCTION

#undef p_enable_if
}

#ifdef VECTOR_USE_RECURSIVE_OPERATIONS
#undef VECTOR_USE_RECURSIVE_OPERATIONS
#endif

#endif /* VECTOR_HPP */

