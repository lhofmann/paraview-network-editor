#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_STDEXTENSIONS_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_STDEXTENSIONS_H_

#include <memory>
#include <string>
#include <iterator>
#include <algorithm>
#include <functional>
#include <numeric>
#include <vector>
#include <type_traits>
#include <utility>
#include <tuple>

namespace util {

template <typename T, typename V>
auto find(T& cont, const V& elem) {
  using std::begin;
  using std::end;
  return std::find(begin(cont), end(cont), elem);
}

template <typename T, typename V>
bool contains(T& cont, const V& elem) {
  using std::begin;
  using std::end;
  return std::find(begin(cont), end(cont), elem) != end(cont);
}

template <typename T, typename V>
auto erase_remove(T& cont, const V& elem)
-> decltype(std::distance(std::declval<T>().begin(), std::declval<T>().end())) {
  using std::begin;
  using std::end;
  auto it = std::remove(begin(cont), end(cont), elem);
  auto nelem = std::distance(it, cont.end());
  cont.erase(it, cont.end());
  return nelem;
}


}

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_STDEXTENSIONS_H_
