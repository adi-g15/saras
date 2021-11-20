#pragma once

// https://arne-mertz.de/2018/05/overload-build-a-variant-visitor-on-the-fly/

// Common pattern to allow overloading lambdas for use in std::visit
// https://devdocs.io/cpp/utility/variant/visit
template <class... Ts> struct overload : Ts... { using Ts::operator()...; };
template <class... Ts> overload(Ts...) -> overload<Ts...>;
