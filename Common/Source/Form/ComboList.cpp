/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   ComboList.cpp
 */

#include "ComboList.h"

ComboList::Entry_t::Entry_t(size_t DataFieldIdx, const TCHAR* _Value, const TCHAR* _ValueFormatted)
    : DataFieldIndex(DataFieldIdx), Value(_Value), ValueFormatted(_ValueFormatted)
{
}

void ComboList::push_back(size_t DataFieldIndex, const TCHAR* Value, const TCHAR* ValueFormatted) {
  ItemList.emplace_back(DataFieldIndex, Value, ValueFormatted);
}

void ComboList::clear() {
  ItemList.clear();
}
