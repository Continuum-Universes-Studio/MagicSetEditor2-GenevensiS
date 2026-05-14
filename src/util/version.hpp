//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make card games                |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

#pragma once

/** @file util/version.hpp
 *
 *  @brief Utility functions related to version numbers.
 *  This header also stores the MSE version number.
 */

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>

// ----------------------------------------------------------------------------- : Version datatype

/// A version number
struct Version {
public:
  Version()                                      : major(0u),    minor(0u),    revision(0u)       {}
  Version(UInt major, UInt minor, UInt revision) : major(major), minor(minor), revision(revision) {}
  
  bool operator==(const Version& v) const {
    return std::tie(major, minor, revision)
      == std::tie(v.major, v.minor, v.revision);
  }
  bool operator!=(const Version& v) const {
    return std::tie(major, minor, revision)
      != std::tie(v.major, v.minor, v.revision);
  }
  bool operator<(const Version& v) const {
    return std::tie(major, minor, revision)
      < std::tie(v.major, v.minor, v.revision);
  }
  bool operator>(const Version& v) const {
    return std::tie(major, minor, revision)
      > std::tie(v.major, v.minor, v.revision);
  }
  bool operator<=(const Version& v) const {
    return std::tie(major, minor, revision)
      <= std::tie(v.major, v.minor, v.revision);
  }
  bool operator>=(const Version& v) const {
    return std::tie(major, minor, revision)
      >= std::tie(v.major, v.minor, v.revision);
  }

  /// Convert a version number to a string
  String toString() const;
  /// Get the version number as an integer number
  UInt   toNumber() const;
  
  /// Convert a string to a version number
  static Version fromString(const String& version);

  /// Did this version get properly initialized?
  bool   isZero() const;

private:
  UInt major = 0;
  UInt minor = 0;
  UInt revision = 0;
};

// ----------------------------------------------------------------------------- : Versions

/// The version number of MSE
extern const Version app_version;
extern const Char* version_suffix;

/// Which version of MSE are the files we write out compatible with?
/** When no files are changed the file version is not incremented
 */
extern const Version file_version_locale;
extern const Version file_version_set;
extern const Version file_version_game;
extern const Version file_version_stylesheet;
extern const Version file_version_symbol_font;
extern const Version file_version_export_template;
extern const Version file_version_installer;
extern const Version file_version_symbol;
extern const Version file_version_clipboard;
extern const Version file_version_script;

