/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 */

#include <array>
#include <utility>
#include <cassert>
#include "MessageLog.h"
#include "DoInits.h"

namespace {

template <std::size_t... I>
constexpr auto Init_DoInit(std::index_sequence<I...>) {
  return std::array<bool, sizeof...(I)>({(void(I), true)...});
}

auto do_init = Init_DoInit(std::make_index_sequence<MDI_LAST_DOINIT>());

}  // namespace

void Reset_DoInit(MDI_t position) {
  assert(position < do_init.size());
  do_init[position] = true;
}

void Reset_DoInit(std::initializer_list<MDI_t> list) {
  for (MDI_t position : list) {
    Reset_DoInit(position);
  }
}

bool DoInit(MDI_t position) {
  assert(position < do_init.size());
  return std::exchange(do_init[position], false);
}
