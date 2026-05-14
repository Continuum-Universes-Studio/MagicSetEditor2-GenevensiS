//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make card games                |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

#pragma once

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <script/scriptable.hpp>

class Set;
DECLARE_POINTER_TYPE(Card);

// ----------------------------------------------------------------------------- : UpdateCardsScript

/// A script to add one or more cards to a set
class UpdateCardsScript : public IntrusivePtrBase<UpdateCardsScript> {
public:
  Version          before_version;
  String           description;
  Scriptable<bool> enabled;
  OptionalScript   script;

  bool operator < (const UpdateCardsScript& other) const {
    return before_version < other.before_version;
  }

  /// Perform the script; return the cards (if any)
  void perform(Set& set, CardP& card, vector<CardP>& out);
  /// Perform the script; add cards to the set
  vector<CardP> perform(Set& set, CardP card); // don't use CardP& here, because set.cards may get reallocated which would invalidate the ref
  
  DECLARE_REFLECTION();
};


