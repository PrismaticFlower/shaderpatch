#pragma once

template<typename... Funcs>
struct overloaded : Funcs... {
   using Funcs::operator()...;
};

template<typename... Funcs>
overloaded(Funcs...) -> overloaded<Funcs...>;
