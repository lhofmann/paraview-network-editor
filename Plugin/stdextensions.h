/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2012-2019 Inviwo Foundation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *********************************************************************************/

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
