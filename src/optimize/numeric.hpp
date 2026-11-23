#ifndef OPTIMIZE_NUMERIC_HPP
#define OPTIMIZE_NUMERIC_HPP

#include <limits>
#include <string>
#include <type_traits>

// Tipos numéricos de distância usados pelas versões otimizadas.
//
// Para tipos de ponto flutuante a "distância infinita" é +inf (e não
// numeric_limits<T>::max()), porque inf + w == inf: o laço interno fica sem
// desvio para testar inf e o compilador consegue vetorizá-lo.
// Para tipos inteiros continua sendo max(), com teste explícito.

namespace optimize{

#if defined(__FLT16_MAX__)
using half = _Float16;
#define OPTIMIZE_HAS_HALF 1
#else
#define OPTIMIZE_HAS_HALF 0
#endif

template<typename T>
struct NumTraits{
    static constexpr bool branchless =
        std::numeric_limits<T>::has_infinity;

    static T inf(){
        if constexpr(std::numeric_limits<T>::has_infinity){
            return std::numeric_limits<T>::infinity();
        } else{
            return std::numeric_limits<T>::max();
        }
    }

    static std::string name(){
        if constexpr(std::is_same_v<T, double>) return "double";
        else if constexpr(std::is_same_v<T, float>) return "float";
        else if constexpr(std::is_same_v<T, long double>) return "long double";
        else if constexpr(std::is_same_v<T, int>) return "int";
        else if constexpr(std::is_same_v<T, long long>) return "long long";
        else return "?";
    }
};

#if OPTIMIZE_HAS_HALF
// numeric_limits<_Float16> não é especializado em C++17.
template<>
struct NumTraits<half>{
    static constexpr bool branchless = true;
    static half inf(){ return __builtin_inff16(); }
    static std::string name(){ return "float16"; }
};
#endif

// Soma "saturada": com inf inteiro (max) evita overflow.
template<typename T>
inline T add_dist(T a, T b, T inf){
    if constexpr(NumTraits<T>::branchless){
        (void)inf;
        return a + b;
    } else{
        if(a == inf || b == inf) return inf;
        return a + b;
    }
}

}

#endif
