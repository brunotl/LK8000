/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   ComboList.h
 */

#ifndef _FORM_COMBOLIST_H_
#define _FORM_COMBOLIST_H_

#include <tchar.h>
#include <vector>
#include "Util/tstring.hpp"

class ComboList {
 public:
  ComboList() = default;

  constexpr static size_t LISTMAX = 2000;  // CAREFUL!
  constexpr static size_t ITEMMAX = 100;
  constexpr static size_t ReopenMOREDataIndex = -800001;
  constexpr static size_t ReopenLESSDataIndex = -800002;
  constexpr static size_t Null = -800003;

  void push_back(size_t DataFieldIndex, const TCHAR* Value, const TCHAR* ValueFormatted);
  void clear();

  size_t size() const {
    return ItemList.size();
  }

  size_t DrawListIndex = 0;
  size_t ItemIndex = ~0;

  struct Entry_t {
    Entry_t() = delete;
    Entry_t(const Entry_t&) = delete;
    Entry_t(Entry_t&&) = default;

    Entry_t(size_t DataFieldIdx, const TCHAR* _Value, const TCHAR* _ValueFormatted);

    size_t DataFieldIndex;

    tstring Value;
    tstring ValueFormatted;
  };

  std::vector<Entry_t> ItemList;  // RLD make this dynamic later

  size_t PropertyDataFieldIndexSaved = ~0;
};

#endif  // _FORM_COMBOLIST_H_
