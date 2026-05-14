//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make card games                |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

#pragma once

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <util/reflect.hpp>
#include <util/error.hpp>
#include <data/filter.hpp>
#include <data/field.hpp> // for Card::value
#include <unordered_set>

class Game;
class Dependency;
class Keyword;
DECLARE_POINTER_TYPE(Set);
DECLARE_POINTER_TYPE(Card);
DECLARE_POINTER_TYPE(Field);
DECLARE_POINTER_TYPE(Value);
DECLARE_POINTER_TYPE(StyleSheet);

#define LINK_PAIRS(var, card) vector<pair<reference_wrapper<String>, reference_wrapper<String>>> var { make_pair(ref(card->linked_card_1), ref(card->linked_relation_1)), make_pair(ref(card->linked_card_2), ref(card->linked_relation_2)), make_pair(ref(card->linked_card_3), ref(card->linked_relation_3)), make_pair(ref(card->linked_card_4), ref(card->linked_relation_4)) }

// ----------------------------------------------------------------------------- : Card

/// A card from a card Set
class Card : public IntrusivePtrVirtualBase, public IntrusiveFromThis<Card> {
public:
  /// Default constructor, uses game_for_new_cards to make the game
  Card();
  /// Creates a card using the given game
  Card(Game& game);
  /// Copy constructor, makes a deep copy
  Card(Set* set, const CardP& card);

  /// The game this card is made for
  Game* game;

  /// The values on the fields of the card.
  /** The indices should correspond to the card_fields in the Game */
  IndexMap<FieldP, ValueP> data;
  /// Notes for this card
  String notes;
  /// Unique identifier for this card, so other cards can refer to it, and be linked to it
  String uid;
  /// Up to four uid of other cards, to encode relations such as front face/back face, or generator/token, etc...
  String linked_card_1;
  String linked_card_2;
  String linked_card_3;
  String linked_card_4;
  /// Nature of the relatation with the respective linked card, such as back face, or token, etc...
  String linked_relation_1;
  String linked_relation_2;
  String linked_relation_3;
  String linked_relation_4;
  /// Time the card was created/last modified
  wxDateTime time_created, time_modified;
  /// Alternative style to use for this card
  /** Optional; if not set use the card style from the set */
  StyleSheetP stylesheet;
  /// What version of the stylesheet was this card using when it was last saved?
  Version stylesheet_version;
  /// Alternative options to use for this card, for this card's stylesheet
  /** Optional; if not set use the styling data from the set.
  *  If stylesheet is set then contains data for the this->stylesheet, otherwise for set->stylesheet
  */
  IndexMap<FieldP,ValueP> styling_data;
  /// Is the styling_data set?
  bool has_styling;

  /// Extra values for specitic stylesheets, indexed by stylesheet name
  DelayedIndexMaps<FieldP,ValueP> extra_data;
  /// Styling information for a particular stylesheet
  IndexMap<FieldP, ValueP>& extraDataFor(const StyleSheet& stylesheet);

  /// Keyword usage statistics
  vector<pair<const Value*,const Keyword*>> keyword_usage;
  
  /// Get the identification of this card, an identification is something like a name, title, etc.
  /** May return "" */
  String identification() const;
  /// Does any field contains the given query string?
  bool contains(QuickFilterPart const& query) const;

  /// Find a value in the data by name and type
  template <typename T> T& value(const String& name) {
    for(IndexMap<FieldP, ValueP>::iterator it = data.begin() ; it != data.end() ; ++it) {
      if ((*it)->fieldP->name == name) {
        T* ret = dynamic_cast<T*>(it->get());
        if (!ret) throw InternalError(_("Card field with name '")+name+_("' doesn't have the right type"));
        return *ret;
      }
    }
    throw InternalError(_("Expected a card field with name '")+name+_("'"));
  }
  template <typename T> const T& value(const String& name) const {
    for(IndexMap<FieldP, ValueP>::const_iterator it = data.begin() ; it != data.end() ; ++it) {
      if ((*it)->fieldP->name == name) {
        const T* ret = dynamic_cast<const T*>(it->get());
        if (!ret) throw InternalError(_("Card field with name '")+name+_("' doesn't have the right type"));
        return *ret;
      }
    }
    throw InternalError(_("Expected a card field with name '")+name+_("'"));
  }

  /// Find the index of a free link slot to write to. Returns -1 if not found.
  int         findFreeLink (const String&   linked_uid,  const unordered_map<String, CardP>& all_existing_uids);
  vector<int> findFreeLinks(vector<String>& linked_uids, const unordered_map<String, CardP>& all_existing_uids);
  
  /// Find the index of a link slot that references the linked_uid. Returns -1 if not found.
  int findUIDLink(const String& linked_uid);
  /// Find all indexes of link slots that references the linked_relation.
  vector<int> findRelationLinks(const String& linked_relation);

  /// Get a reference to the linked uid slot.
  String& getLinkedUID     (int index);
  /// Get a reference to the linked relation slot.
  String& getLinkedRelation(int index);

  /// Make all links that point to old_uid point to new_uid instead.
  void updateLinkedUID(const String& old_uid, const String& new_uid);
  /// Make all links that have old_relation have new_relation instead. Not needed apparently.
  //void updateLinkedRelation(const String& old_relation, const String& new_relation);

  /// Get the card with the given uid.
  //static CardP getUIDCard(const vector<CardP>& cards, const String& uid);
  static CardP getUIDCard(const Set& set,             const String& uid);
  /// Get all the cards linked to this card with the given relation.
  //vector<CardP> getLinkedRelationCards(const vector<CardP>& cards, const String& linked_relation, bool erase_if_no_card = true);
  vector<CardP> getLinkedRelationCards(const Set& set,             const String& linked_relation, bool erase_if_no_card = true);

  /// Get all the cards linked to this card.
  //vector<pair<CardP, String>> getLinkedCards(const vector<CardP>& cards);
  vector<pair<CardP, String>> getLinkedCards(const Set& set);

  /// Get the back face or front face of this card.
  //CardP getLinkedOtherFaceCard(const vector<CardP>& cards);
  CardP getLinkedOtherFaceCard(const Set& set);

  /// Link a card to this card.
  void addLink(const Set& set, CardP& linked_card, const String& selected_relation, const String& linked_relation);

  /// Unlink a card from this card.
  void removeLink(const CardP& linked_card);

  DECLARE_REFLECTION();
};

inline String type_name(const Card&) {
  return _TYPE_("card");
}
inline String type_name(const vector<CardP>&) {
  return _TYPE_("cards"); // not actually used, only for locale.pl script
}

void mark_dependency_member(const Card& value, const String& name, const Dependency& dep);

