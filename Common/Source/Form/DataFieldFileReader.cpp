/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DataFieldFileReader.cpp
 */

#include "DataFieldFileReader.h"
#include "externs.h"
#include "utils/array_adaptor.h"
#include "LocalPath.h"
#include <algorithm>
#include <cassert>
#include "utils/stringext.h"
#include "Dialogs.h"
#include "Util/Clamp.hpp"
#include "WindowControls.h"
#include "ComboList.h"

DataFieldFileReader::DataFieldFileReader(WndProperty& Owner, const char* EditFormat, const char* DisplayFormat,
                                         DataAccessCallback_t&& OnDataAccess)
    : DataField(Owner, EditFormat, DisplayFormat, std::forward<DataAccessCallback_t>(OnDataAccess))
{
  SupportCombo = true;
  mOnDataAccess(this, daGet);
}

DataFieldFileReader::~DataFieldFileReader() {
  Clear();
}

void DataFieldFileReader::Clear() {
  file_list = {{ _T(""), _T("") }};  // initialize with first blank entry
  mValue = 0;
}

void DataFieldFileReader::Inc() {
  if (mValue < file_list.size() - 1) {
    ++mValue;
    if (!GetDetachGUI()) {
      mOnDataAccess(this, daChange);
    }
  }
}

void DataFieldFileReader::Dec() {
  if (mValue > 0) {
    --mValue;
    if (!GetDetachGUI()) {
      mOnDataAccess(this, daChange);
    }
  }
}

int DataFieldFileReader::CreateComboList() {
  size_t file_count = file_list.size();
  for (size_t i = 0; i < file_count; i++) {
    mComboList.push_back(i, file_list[i].mLabel.c_str(), file_list[i].mLabel.c_str());
  }
  mComboList.PropertyDataFieldIndexSaved = (mValue < file_count) ? mValue : 0;
  return mComboList.size();
}

void DataFieldFileReader::addFile(const TCHAR* fname, const TCHAR* fpname) {
  file_list.push_back({fname, fpname});
}

int DataFieldFileReader::GetNumFiles() {
  return file_list.size();
}

int DataFieldFileReader::GetAsInteger() {
  return mValue;
}

const TCHAR* DataFieldFileReader::GetAsString() {
  const TCHAR* label = nullptr;
  if (mValue < file_list.size()) {
    label = file_list[mValue].mLabel.c_str();
  }
  return label ? label : _T("");  // always return valid pointer
}

int DataFieldFileReader::GetLabelIndex(const TCHAR* label) {
  auto compare_label = [&](const Entry& entry) {
      return entry.mLabel == (label ? label : _T(""));
  };
  auto it = std::find_if(file_list.begin(), file_list.end(), compare_label);
  return (it != file_list.end()) ? std::distance(file_list.begin(), it) : -1;
}

bool DataFieldFileReader::Lookup(const TCHAR* text) {
  mValue = 0;
  auto compare_path = [&](const Entry& entry) {
      return entry.mFilePath == (text ? text : _T(""));
  };

  auto it = std::find_if(file_list.begin(), file_list.end(), compare_path);
  if (it != file_list.end()) {
    mValue = std::distance(file_list.begin(), it);
    return true;
  }
  return false;
}

const TCHAR* DataFieldFileReader::GetPathFile() const {
  const TCHAR* path = nullptr;
  if (mValue < file_list.size()) {
    path = file_list[mValue].mFilePath.c_str();
  }
  return path ? path : _T("");  // always return valid pointer
}

void DataFieldFileReader::Set(int Value) {
  if (static_cast<file_list_t::size_type>(Value) < file_list.size()) {
    mValue = Value;
  } else {
    mValue = 0;
  }
}

int DataFieldFileReader::SetAsInteger(int Value) {
  Set(Value);
  return mValue;
}

void DataFieldFileReader::Sort(int startindex) {
  auto begin = std::next(file_list.begin(), startindex);
  auto end = file_list.end();
  auto compare_label = [](const Entry& a, const Entry& b) {
    return a.mLabel == b.mLabel; 
  };
  std::sort(begin, end, compare_label);
}

void DataFieldFileReader::ScanDirectoryTop(const TCHAR* subdir, const TCHAR** suffix_filters, size_t filter_count,
                                           size_t sort_start_index) {
  TCHAR buffer[MAX_PATH] = TEXT("\0");
  LocalPath(buffer, subdir);
  ScanDirectories(buffer, _T(""), suffix_filters, filter_count);
  Sort(sort_start_index);
}

void DataFieldFileReader::ScanSystemDirectoryTop(const TCHAR* subdir, const TCHAR** suffix_filters, size_t filter_count,
                                                 size_t sort_start_index) {
#ifndef ANDROID
  TCHAR buffer[MAX_PATH] = TEXT("\0");
  SystemPath(buffer, subdir);
  ScanDirectories(buffer, _T(""), suffix_filters, filter_count);
#else
  ScanZipDirectory(subdir, suffix_filters, filter_count);
#endif
  Sort(sort_start_index);
}

static bool checkFilter(const TCHAR* filename, const TCHAR** suffix_filters, size_t filter_count) {
  size_t filename_size = _tcslen(filename);

  for (const TCHAR* suffix : make_array(suffix_filters, filter_count)) {
    if (!suffix || !suffix[0]) {
      return true;
    }

    size_t suffix_size = _tcslen(suffix);
    if (filename_size < suffix_size) {
      continue;
    }

    const TCHAR* filename_suffix = &filename[filename_size - suffix_size];
    if (_tcsicmp(filename_suffix, suffix) == 0) {
      return true;
    }
  }
  return false;
}

#ifdef ANDROID
void DataFieldFileReader::ScanZipDirectory(const TCHAR* subdir, const TCHAR** suffix_filters, size_t filter_count) {
  const zzip_strings_t ext[] = {".zip", ".ZIP", "", 0};
  zzip_error_t zzipError;

  tstring sRootPath = LKGetSystemPath();
  if (sRootPath.empty()) {
    return;
  }
  sRootPath.pop_back();  // remove trailing directory separator

  size_t subdir_size = _tcslen(subdir);
  ZZIP_DIR* dir = zzip_dir_open_ext_io(sRootPath.c_str(), &zzipError, ext, nullptr);
  if (dir) {
    ZZIP_DIRENT dirent;
    while (zzip_dir_read(dir, &dirent)) {
      if (_tcsnicmp(subdir, dirent.d_name, subdir_size) == 0) {
        if (checkFilter(dirent.d_name, suffix_filters, filter_count)) {
          TCHAR* szFileName = _tcsrchr(dirent.d_name, _T('/')) + 1;
          if (GetLabelIndex(szFileName) <= 0) {
            TCHAR* szFilePath = dirent.d_name + _tcslen(subdir);
            while ((*szFilePath) == _T('/') || (*szFilePath) == _T('\\')) {
              ++szFilePath;
            }
            addFile(szFileName, szFilePath);
          }
        }
      }
    }
    zzip_dir_close(dir);
  }
}
#endif

BOOL DataFieldFileReader::ScanDirectories(const TCHAR* sPath, const TCHAR* subdir, const TCHAR** suffix_filters,
                                          size_t filter_count) {
  assert(sPath);
  assert(subdir);
  assert(suffix_filters);

  TCHAR FileName[MAX_PATH];
  _tcscpy(FileName, sPath);
  _tcscat(FileName, TEXT(DIRSEP));
  if (_tcslen(subdir) > 0) {
    _tcscat(FileName, subdir);
    _tcscat(FileName, TEXT(DIRSEP));
  }
  _tcscat(FileName, TEXT("*"));
  lk::filesystem::fixPath(FileName);

  for (lk::filesystem::directory_iterator It(FileName); It; ++It) {
    if (It.isDirectory()) {
      _tcscpy(FileName, subdir);
      if (_tcslen(FileName) > 0) {
        _tcscat(FileName, TEXT(DIRSEP));
      }
      _tcscat(FileName, It.getName());

      ScanDirectories(sPath, FileName, suffix_filters, filter_count);
    } else if (checkFilter(It.getName(), suffix_filters, filter_count)) {
      _tcscpy(FileName, subdir);
      if (_tcslen(FileName) > 0) {
        _tcscat(FileName, TEXT(DIRSEP));
      }
      _tcscat(FileName, It.getName());
      if (GetLabelIndex(FileName) <= 0) {  // do not add same file twice...
        addFile(It.getName(), FileName);
      }
    }
  }
  return TRUE;
}
