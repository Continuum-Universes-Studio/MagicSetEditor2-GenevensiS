//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make card games                |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <data/card.hpp>
#include <data/set.hpp>
#include <data/game.hpp>
#include <data/stylesheet.hpp>
#include <data/field.hpp>
#include <util/error.hpp>
#include <util/reflect.hpp>
#include <util/delayed_index_maps.hpp>
#include <util/uid.hpp>
#include <unordered_set>

// ----------------------------------------------------------------------------- : Card

Card::Card()
// for files made before we saved these, set the time to 'yesterday', generate a uid
  : game(game_for_reading())
  , time_created (wxDateTime::Now().Subtract(wxDateSpan::Day()).ResetTime())
  , time_modified(wxDateTime::Now().Subtract(wxDateSpan::Day()).ResetTime())
  , uid(generate_uid())
  , has_styling(false)
{
  if (!game) {
    throw InternalError(_("game_for_reading not set"));
  }
  data.init(game->card_fields);
}

Card::Card(Game& game)
  : game(&game)
  , time_created (wxDateTime::Now())
  , time_modified(wxDateTime::Now())
  , uid(generate_uid())
  , has_styling(false)
{
  data.init(game.card_fields);
}

Card::Card(Set* set, const CardP& card)
  : game(card->game)
  , time_created (wxDateTime::Now())
  , time_modified(wxDateTime::Now())
  , notes(card->notes)
  , uid(generate_uid())
  , linked_card_1(card->linked_card_1)
  , linked_card_2(card->linked_card_2)
  , linked_card_3(card->linked_card_3)
  , linked_card_4(card->linked_card_4)
  , linked_relation_1(card->linked_relation_1)
  , linked_relation_2(card->linked_relation_2)
  , linked_relation_3(card->linked_relation_3)
  , linked_relation_4(card->linked_relation_4)
  , has_styling(card->has_styling)
  , stylesheet_version(card->stylesheet_version)
  , stylesheet(card->stylesheet)
{
  if (!stylesheet && set) {
    stylesheet = set->stylesheetForP(card);
  }
  if (has_styling) {
    styling_data.cloneFrom(card->styling_data);
  }
  else {
    if (stylesheet && set) styling_data.cloneFrom(set->stylingDataFor(*stylesheet));
  }
  data.cloneFrom(card->data);
  extra_data.cloneFrom(card->extra_data);
}

String Card::identification() const {
  // an identifying field
  FOR_EACH_CONST(v, data) {
    if (v->fieldP->identifying) {
      return v->toString();
    }
  }
  // otherwise the first field
  if (!data.empty()) {
    return data.at(0)->toString();
  } else {
    return _("");
  }
}

bool Card::contains(QuickFilterPart const& query) const {
  FOR_EACH_CONST(v, data) {
    if (query.match(v->fieldP->name, v->toString())) return true;
  }
  if (query.match(_("notes"), notes)) return true;
  return false;
}

vector<int> Card::findFreeLinks(vector<String>& linked_uids, const unordered_map<String, CardP>& all_existing_uids) {
  vector<int> freeIndexes;
  int count = min(4, (int)linked_uids.size());
  LINK_PAIRS(linked_pairs, this);

  for (int i = 0; i < count; ++i) {
    String& linked_uid = linked_uids[i];
    // Try to find an existing link
    for (int j = 0; j < linked_pairs.size(); ++j) {
      auto& linked_pair = linked_pairs[j];
      if (linked_pair.first.get() == linked_uid &&
          std::find(freeIndexes.begin(), freeIndexes.end(), j) == freeIndexes.end()
      ) {
        freeIndexes.push_back(j);
        goto continue_outer;
      }
    }
    // Try to find a free spot
    for (int j = 0; j < linked_pairs.size(); ++j) {
      auto& linked_pair = linked_pairs[j];
      if (linked_pair.first.get().empty() &&
          std::find(freeIndexes.begin(), freeIndexes.end(), j) == freeIndexes.end()
      ) {
        freeIndexes.push_back(j);
        goto continue_outer;
      }
    }
    // Try to find an erasable spot
    for (int j = 0; j < linked_pairs.size(); ++j) {
      auto& linked_pair = linked_pairs[j];
      if (all_existing_uids.find(linked_pair.first.get()) == all_existing_uids.end() &&
          std::find(freeIndexes.begin(), freeIndexes.end(), j) == freeIndexes.end()
      ) {
        freeIndexes.push_back(j);
        goto continue_outer;
      }
    }
    freeIndexes.push_back(-1);
    continue_outer:;
  }
  return freeIndexes;
}
int Card::findFreeLink(const String& linked_uid, const unordered_map<String, CardP>& all_existing_uids) {
  vector<String> linked_uids { linked_uid };
  return findFreeLinks(linked_uids, all_existing_uids)[0];
}

int Card::findUIDLink(const String& linked_uid) {
  if (linked_card_1 == linked_uid) return 0;
  if (linked_card_2 == linked_uid) return 1;
  if (linked_card_3 == linked_uid) return 2;
  if (linked_card_4 == linked_uid) return 3;
  return -1;
}

vector<int> Card::findRelationLinks(const String& linked_relation) {
  vector<int> indexes;
  if (linked_relation_1 == linked_relation) indexes.push_back(0);
  if (linked_relation_2 == linked_relation) indexes.push_back(1);
  if (linked_relation_3 == linked_relation) indexes.push_back(2);
  if (linked_relation_4 == linked_relation) indexes.push_back(3);
  return indexes;
}

String& Card::getLinkedUID(int index) {
  switch (index) {
  case 0:  return linked_card_1;
  case 1:  return linked_card_2;
  case 2:  return linked_card_3;
  case 3:  return linked_card_4;
  default: throw ScriptError(_("getLinkedUID called with invalid index"));
  }
}
String& Card::getLinkedRelation(int index) {
  switch (index) {
  case 0:  return linked_relation_1;
  case 1:  return linked_relation_2;
  case 2:  return linked_relation_3;
  case 3:  return linked_relation_4;
  default: throw ScriptError(_("getLinkedRelation called with invalid index"));
  }
}

void Card::updateLinkedUID(const String& old_uid, const String& new_uid) {
  int index = findUIDLink(old_uid);
  if (index >= 0) getLinkedUID(index) = new_uid;
}

//vector<CardP> Card::getLinkedRelationCards(const vector<CardP>& cards, const String& linked_relation, bool erase_if_no_card) {
//  vector<CardP> other_cards;
//  vector<int> indexes = findRelationLinks(linked_relation);
//  for (size_t i = 0; i < indexes.size(); ++i) {
//    String& linked_uid = getLinkedUID(indexes[i]);
//    CardP other_card = getUIDCard(cards, linked_uid);
//    if (other_card) other_cards.push_back(other_card);
//    else if (erase_if_no_card) {
//      linked_uid = _("");
//      getLinkedRelation(indexes[i]) = _("");
//    }
//  }
//  return other_cards;
//}
vector<CardP> Card::getLinkedRelationCards(const Set& set, const String& linked_relation, bool erase_if_no_card) {
  vector<CardP> other_cards;
  vector<int> indexes = findRelationLinks(linked_relation);
  for (size_t i = 0; i < indexes.size(); ++i) {
    String& linked_uid = getLinkedUID(indexes[i]);
    CardP other_card = getUIDCard(set, linked_uid);
    if (other_card) other_cards.push_back(other_card);
    else if (erase_if_no_card) {
      linked_uid = _("");
      getLinkedRelation(indexes[i]) = _("");
    }
  }
  return other_cards;
}

//vector<pair<CardP, String>> Card::getLinkedCards(const vector<CardP>& cards) {
//  unordered_map<String, String> links{
//    { linked_card_1, linked_relation_1 },
//    { linked_card_2, linked_relation_2 },
//    { linked_card_3, linked_relation_3 },
//    { linked_card_4, linked_relation_4 }
//  };
//  vector<pair<CardP, String>> linked_cards;
//  FOR_EACH(other_card, cards) {
//    if (links.find(other_card->uid) != links.end()) {
//      linked_cards.push_back(make_pair(other_card, links.at(other_card->uid)));
//    }
//  }
//  return linked_cards;
//}
vector<pair<CardP, String>> Card::getLinkedCards(const Set& set) {
  vector<pair<CardP, String>> linked_cards;
  CardP other_card_1 = getUIDCard(set, linked_card_1);
  if (other_card_1) {
    linked_cards.push_back(make_pair(other_card_1, linked_relation_1));
  }
  CardP other_card_2 = getUIDCard(set, linked_card_2);
  if (other_card_2) {
    linked_cards.push_back(make_pair(other_card_2, linked_relation_2));
  }
  CardP other_card_3 = getUIDCard(set, linked_card_3);
  if (other_card_3) {
    linked_cards.push_back(make_pair(other_card_3, linked_relation_3));
  }
  CardP other_card_4 = getUIDCard(set, linked_card_4);
  if (other_card_4) {
    linked_cards.push_back(make_pair(other_card_4, linked_relation_4));
  }
  return linked_cards;
}

//CardP Card::getLinkedOtherFaceCard(const vector<CardP>& cards) {
//  unordered_set<String> faces;
//  if (linked_relation_1 == _("Front Face") || linked_relation_1 == _("Back Face")) faces.emplace(linked_card_1);
//  if (linked_relation_2 == _("Front Face") || linked_relation_2 == _("Back Face")) faces.emplace(linked_card_2);
//  if (linked_relation_3 == _("Front Face") || linked_relation_3 == _("Back Face")) faces.emplace(linked_card_3);
//  if (linked_relation_4 == _("Front Face") || linked_relation_4 == _("Back Face")) faces.emplace(linked_card_4);
//  FOR_EACH(other_card, cards) {
//    if (faces.find(other_card->uid) != faces.end()) return other_card;
//  }
//  return nullptr;
//}
CardP Card::getLinkedOtherFaceCard(const Set& set) {
  if (linked_relation_1 == _("Front Face") || linked_relation_1 == _("Back Face")) {
    CardP other_card_1 = getUIDCard(set, linked_card_1);
    if (other_card_1) return other_card_1;
  }
  if (linked_relation_2 == _("Front Face") || linked_relation_2 == _("Back Face")) {
    CardP other_card_2 = getUIDCard(set, linked_card_2);
    if (other_card_2) return other_card_2;
  }
  if (linked_relation_3 == _("Front Face") || linked_relation_3 == _("Back Face")) {
    CardP other_card_3 = getUIDCard(set, linked_card_3);
    if (other_card_3) return other_card_3;
  }
  if (linked_relation_4 == _("Front Face") || linked_relation_4 == _("Back Face")) {
    CardP other_card_4 = getUIDCard(set, linked_card_4);
    if (other_card_4) return other_card_4;
  }
  return nullptr;
}

void Card::addLink(const Set& set, CardP& linked_card, const String& selected_relation, const String& linked_relation) {
  int index = findFreeLink(linked_card->uid, set.card_uids);
  if (index < 0) {
    queue_message(MESSAGE_ERROR, _ERROR_1_("not enough free links", identification()));
    return;
  }
  getLinkedUID(index) = linked_card->uid;
  getLinkedRelation(index) = linked_relation;

  index = linked_card->findFreeLink(uid, set.card_uids);
  if (index < 0) {
    queue_message(MESSAGE_ERROR, _ERROR_1_("not enough free links", linked_card->identification()));
  }
  else {
    linked_card->getLinkedUID(index) = uid;
    linked_card->getLinkedRelation(index) = selected_relation;
  }
}

void Card::removeLink(const CardP& linked_card)
{
  int index = findUIDLink(linked_card->uid);
  if (index >= 0) {
    getLinkedUID(index) = _("");
    getLinkedRelation(index) = _("");
  }

  index = linked_card->findUIDLink(uid);
  if (index >= 0) {
    linked_card->getLinkedUID(index) = _("");
    linked_card->getLinkedRelation(index) = _("");
  }
}

//CardP Card::getUIDCard(const vector<CardP>& cards, const String& uid) {
//  FOR_EACH(card, cards) {
//    if (card->uid == uid) return card;
//  }
//  return nullptr;
//}
CardP Card::getUIDCard(const Set& set, const String& uid) {
  auto it = set.card_uids.find(uid);
  if (it != set.card_uids.end()) {
    return it->second;
  }
  return nullptr;
}

IndexMap<FieldP, ValueP>& Card::extraDataFor(const StyleSheet& stylesheet) {
  return extra_data.get(stylesheet.name(), stylesheet.extra_card_fields);
}

void mark_dependency_member(const Card& card, const String& name, const Dependency& dep) {
  // is it the uid?
  if (name == _("uid")) {
    if (card.game) {
      card.game->dependent_scripts_uid.add(dep);
    }
    return;
  }
  // is it the notes?
  if (name == _("notes")) {
    if (card.game) {
      card.game->dependent_scripts_notes.add(dep);
    }
    return;
  }
  // is it a link?
  if (
    name == _("linked_card_1") ||
    name == _("linked_card_2") ||
    name == _("linked_card_3") ||
    name == _("linked_card_4") ||
    name == _("linked_relation_1") ||
    name == _("linked_relation_2") ||
    name == _("linked_relation_3") ||
    name == _("linked_relation_4")
  ) {
    if (card.game) {
      card.game->dependent_scripts_links.add(dep);
    }
    return;
  }
  mark_dependency_member(card.data, name, dep);
}

void reflect_version_check(Reader& handler, const Char* key, intrusive_ptr<Packaged> const& package);
void reflect_version_check(Writer& handler, const Char* key, intrusive_ptr<Packaged> const& package);
void reflect_version_check(GetMember& handler, const Char* key, intrusive_ptr<Packaged> const& package);
void reflect_version_check(GetDefaultMember& handler, const Char* key, intrusive_ptr<Packaged> const& package);

IMPLEMENT_REFLECTION(Card) {
  REFLECT(stylesheet);
  if (Handler::isReading) {
    REFLECT_NO_SCRIPT(stylesheet_version);
  }
  else {
    reflect_version_check(handler, _("stylesheet_version"), stylesheet);
  }
  REFLECT(has_styling);
  if (has_styling) {
    if (stylesheet) {
      REFLECT_IF_READING styling_data.init(stylesheet->styling_fields);
      REFLECT(styling_data);
    } else if (stylesheet_for_reading()) {
      REFLECT_IF_READING styling_data.init(stylesheet_for_reading()->styling_fields);
      REFLECT(styling_data);
    } else if (Handler::isReading) {
      has_styling = false; // We don't know the style, this can be because of copy/pasting
    }
  }
  REFLECT(notes);
  REFLECT(uid);
  REFLECT(linked_card_1);
  REFLECT(linked_card_2);
  REFLECT(linked_card_3);
  REFLECT(linked_card_4);
  REFLECT(linked_relation_1);
  REFLECT(linked_relation_2);
  REFLECT(linked_relation_3);
  REFLECT(linked_relation_4);
  REFLECT(time_created);
  REFLECT(time_modified);
  REFLECT(extra_data); // don't allow scripts to depend on style specific data
  REFLECT_NAMELESS(data);
}
