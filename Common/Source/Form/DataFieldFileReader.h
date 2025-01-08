/*
 * LK8000 Tactical Flight Computer -  WWW.LK8000.IT
 * Released under GNU/GPL License v.2 or later
 * See CREDITS.TXT file for authors and copyrights
 *
 * File:   DataFieldFileReader.h
 */

#ifndef _FORM_DATAFIELD_FILEREADER_H_
#define _FORM_DATAFIELD_FILEREADER_H_

#include "DataField.h"
#include <vector>
#include <tchar.h>

class DataFieldFileReader : public DataField {
 public:
  struct Entry {
    tstring mLabel;
    tstring mFilePath;
  };

 private:
  using file_list_t = std::vector<Entry>;

  file_list_t::size_type mValue = 0;
  file_list_t file_list = {{ _T(""), _T("") }};  // initialize with first blank entry

 public:
  DataFieldFileReader(WndProperty& Owner, const char* EditFormat, const char* DisplayFormat,
                      DataAccessCallback_t&& OnDataAccess);
  ~DataFieldFileReader();

  void Clear() override;

  void Inc() override;
  void Dec() override;
  int CreateComboList() override;

  void addFile(const TCHAR* fname, const TCHAR* fpname);

  int GetNumFiles();

  int GetAsInteger() override;
  const TCHAR* GetAsString() override;

  int GetLabelIndex(const TCHAR* label);

  bool Lookup(const TCHAR* text);

  const TCHAR* GetPathFile() const;

  void Set(int Value) override;

  int SetAsInteger(int Value) override;

  void Sort(int startindex = 0) override;

  gcc_nonnull_all
  void ScanDirectoryTop(const TCHAR* subdir, const TCHAR** suffix_filters, size_t filter_count,
                        size_t sort_start_index);

  template <size_t filter_count>
  gcc_nonnull_all
  void ScanDirectoryTop(const TCHAR* subdir, const TCHAR* (&suffix_filters)[filter_count],
                        size_t sort_start_index = 0) {
    ScanDirectoryTop(subdir, suffix_filters, filter_count, sort_start_index);
  }

  gcc_nonnull_all
  void ScanDirectoryTop(const TCHAR* subdir, const TCHAR* suffix_filter, size_t sort_start_index = 0) {
    const TCHAR* suffix_filters[] = {suffix_filter};
    ScanDirectoryTop(subdir, suffix_filters, sort_start_index);
  }

  gcc_nonnull_all
  void ScanSystemDirectoryTop(const TCHAR* subdir, const TCHAR** suffix_filters, size_t filter_count,
                              size_t sort_start_index);

  template <size_t filter_count>
  gcc_nonnull_all
  void ScanSystemDirectoryTop(const TCHAR* subdir, const TCHAR* (&suffix_filters)[filter_count],
                              size_t sort_start_index = 0) {
    ScanSystemDirectoryTop(subdir, suffix_filters, filter_count, sort_start_index);
  }

  gcc_nonnull_all
  void ScanSystemDirectoryTop(const TCHAR* subdir, const TCHAR* suffix_filter,
                              size_t sort_start_index = 0) {
    const TCHAR* suffix_filters[] = {suffix_filter};
    ScanSystemDirectoryTop(subdir, suffix_filters, sort_start_index);
  }

 protected:
#ifdef ANDROID
  gcc_nonnull_all void ScanZipDirectory(const TCHAR* subdir, const TCHAR** suffix_filters, size_t filter_count);
#endif

  gcc_nonnull_all BOOL ScanDirectories(const TCHAR* sPath, const TCHAR* subdir, const TCHAR** suffix_filters,
                                       size_t filter_count);
};

#endif  // _FORM_DATAFIELD_FILEREADER_H_
