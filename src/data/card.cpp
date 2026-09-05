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
  , time_created(card->time_created)
  , time_modified(card->time_modified)
  , notes(card->notes)
  , uid(card->uid)
  , has_styling(card->has_styling)
  , stylesheet_version(card->stylesheet_version)
  , stylesheet(card->stylesheet)
{
  // copy all the link slots
  LINK_PAIRS(dst_links, this);
  LINK_PAIRS(src_links, card);
  for (int i = 0; i < (int)dst_links.size(); ++i) {
    dst_links[i].first.get()  = src_links[i].first.get();
    dst_links[i].second.get() = src_links[i].second.get();
  }
  if (!stylesheet && set) {
    stylesheet = set->stylesheetForP(card);
  }
  if (has_styling) {
    if (stylesheet) {
      styling_data.init(stylesheet->styling_fields);
      styling_data.copyDataFrom(card->styling_data);
    } else {
      has_styling = false; // no stylesheet resolved, nothing to safely carry over
    }
  }
  else {
    if (stylesheet && set) styling_data.cloneFrom(set->stylingDataFor(*stylesheet));
  }
  data.init(game->card_fields);
  data.copyDataFrom(card->data);
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
  int count = (int)linked_uids.size();
  if (count > Card::MAX_LINKS) count = Card::MAX_LINKS; // avoid binding a reference to the static constant, just read its value
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
  LINK_PAIRS(linked_pairs, this);
  for (int i = 0; i < (int)linked_pairs.size(); ++i) {
    if (linked_pairs[i].first.get() == linked_uid) return i;
  }
  return -1;
}

vector<int> Card::findRelationLinks(const String& linked_relation) {
  vector<int> indexes;
  LINK_PAIRS(linked_pairs, this);
  for (int i = 0; i < (int)linked_pairs.size(); ++i) {
    if (linked_pairs[i].second.get() == linked_relation) indexes.push_back(i);
  }
  return indexes;
}

String& Card::getLinkedUID(int index) {
  LINK_PAIRS(linked_pairs, this);
  if (index < 0 || index >= (int)linked_pairs.size()) {
    throw ScriptError(String("getLinkedUID called with invalid index: ") << index);
  }
  return linked_pairs[index].first.get();
}
String& Card::getLinkedRelation(int index) {
  LINK_PAIRS(linked_pairs, this);
  if (index < 0 || index >= (int)linked_pairs.size()) {
    throw ScriptError(String("getLinkedRelation called with invalid index: ") << index);
  }
  return linked_pairs[index].second.get();
}

int Card::indexedFieldIndex(const String& key_name, const String& base_name, int max_count) {
  if (key_name == base_name) return 0;
  String prefix = base_name + _("_");
  if (!key_name.starts_with(prefix)) return -1;
  long n = 0;
  if (!key_name.substr(prefix.size()).ToLong(&n) || n < 1 || n > max_count) return -1;
  return (int)n - 1;
}

int Card::linkedCardFieldIndex(const String& key_name) {
  return indexedFieldIndex(key_name, _("linked_card"), MAX_LINKS);
}
int Card::linkedRelationFieldIndex(const String& key_name) {
  return indexedFieldIndex(key_name, _("linked_relation"), MAX_LINKS);
}
bool Card::isLinkFieldName(const String& name) {
  return linkedCardFieldIndex(name) >= 0 || linkedRelationFieldIndex(name) >= 0;
}

void Card::updateLinkedUID(const String& old_uid, const String& new_uid) {
  int index = findUIDLink(old_uid);
  if (index >= 0) getLinkedUID(index) = new_uid;
}

vector<CardP> Card::getLinkedRelationCards(const Set& set, const String& linked_relation) {
  vector<CardP> other_cards;
  vector<int> indexes = findRelationLinks(linked_relation);
  for (size_t i = 0; i < indexes.size(); ++i) {
    String& linked_uid = getLinkedUID(indexes[i]);
    CardP other_card = getUIDCard(set, linked_uid);
    if (other_card) other_cards.push_back(other_card);
  }
  return other_cards;
}

vector<pair<CardP, String>> Card::getLinkedCards(const Set& set) {
  vector<pair<CardP, String>> linked_cards;
  LINK_PAIRS(linked_pairs, this);
  for (int i = 0; i < (int)linked_pairs.size(); ++i) {
    CardP other_card = getUIDCard(set, linked_pairs[i].first.get());
    if (other_card) {
      linked_cards.push_back(make_pair(other_card, linked_pairs[i].second.get()));
    }
  }
  return linked_cards;
}

CardP Card::getFrontFaceCard(Set& set) {
  // use game script logic if defined
  if (set.game->get_front_face_script) {
    const CardP& this_card = CardP(this);
    Context& ctx = set.getContext(this_card);
    ctx.setVariable(SCRIPT_VAR_input, to_script(this_card));
    ScriptValueP result = set.game->get_front_face_script.invoke(ctx);
    if (result == script_nil) return CardP();
    if (ScriptObject<CardP>* ic = dynamic_cast<ScriptObject<CardP>*>(result.get())) {
      return ic->getValue();
    }
    queue_message(MESSAGE_ERROR, _ERROR_1_("result not card or nil", "get_front_face_script"));
    return CardP();
  }
  // fallback on regular Front Face/Back Face logic
  vector<CardP> other_cards = getLinkedRelationCards(set, "Front Face");
  if (other_cards.size() == 0) return CardP();
  return other_cards[0];
}

CardP Card::getBackFaceCard(Set& set) {
  // use game script logic if defined
  if (set.game->get_back_face_script) {
    const CardP& this_card = CardP(this);
    Context& ctx = set.getContext(this_card);
    ctx.setVariable(SCRIPT_VAR_input, to_script(this_card));
    ScriptValueP result = set.game->get_back_face_script.invoke(ctx);
    if (result == script_nil) return CardP();
    if (ScriptObject<CardP>* ic = dynamic_cast<ScriptObject<CardP>*>(result.get())) {
      return ic->getValue();
    }
    queue_message(MESSAGE_ERROR, _ERROR_1_("result not card or nil", "get_back_face_script"));
    return CardP();
  }
  // fallback on regular Front Face/Back Face logic
  vector<CardP> other_cards = getLinkedRelationCards(set, "Back Face");
  if (other_cards.size() == 0) return CardP();
  return other_cards[0];
}

// Find the other face, keep track of which is the front
pair<CardP, CardP> Card::getFrontFaceBackFacePair(Set& set) {
  const CardP& back = getBackFaceCard(set);
  if (back) return make_pair(CardP(this), back);
  const CardP& front = getFrontFaceCard(set);
  if (front) return make_pair(front, CardP(this));
  return make_pair(CardP(), CardP());
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
  if (Card::isLinkFieldName(name)) {
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
  REFLECT(linked_card_5);
  REFLECT(linked_card_6);
  REFLECT(linked_card_7);
  REFLECT(linked_card_8);
  REFLECT(linked_card_9);
  REFLECT(linked_card_10);
  REFLECT(linked_card_11);
  REFLECT(linked_card_12);
  REFLECT(linked_card_13);
  REFLECT(linked_card_14);
  REFLECT(linked_card_15);
  REFLECT(linked_card_16);
  REFLECT(linked_card_17);
  REFLECT(linked_card_18);
  REFLECT(linked_card_19);
  REFLECT(linked_card_20);
  REFLECT(linked_card_21);
  REFLECT(linked_card_22);
  REFLECT(linked_card_23);
  REFLECT(linked_card_24);
  REFLECT(linked_card_25);
  REFLECT(linked_card_26);
  REFLECT(linked_card_27);
  REFLECT(linked_card_28);
  REFLECT(linked_card_29);
  REFLECT(linked_card_30);
  REFLECT(linked_card_31);
  REFLECT(linked_card_32);
  REFLECT(linked_relation_1);
  REFLECT(linked_relation_2);
  REFLECT(linked_relation_3);
  REFLECT(linked_relation_4);
  REFLECT(linked_relation_5);
  REFLECT(linked_relation_6);
  REFLECT(linked_relation_7);
  REFLECT(linked_relation_8);
  REFLECT(linked_relation_9);
  REFLECT(linked_relation_10);
  REFLECT(linked_relation_11);
  REFLECT(linked_relation_12);
  REFLECT(linked_relation_13);
  REFLECT(linked_relation_14);
  REFLECT(linked_relation_15);
  REFLECT(linked_relation_16);
  REFLECT(linked_relation_17);
  REFLECT(linked_relation_18);
  REFLECT(linked_relation_19);
  REFLECT(linked_relation_20);
  REFLECT(linked_relation_21);
  REFLECT(linked_relation_22);
  REFLECT(linked_relation_23);
  REFLECT(linked_relation_24);
  REFLECT(linked_relation_25);
  REFLECT(linked_relation_26);
  REFLECT(linked_relation_27);
  REFLECT(linked_relation_28);
  REFLECT(linked_relation_29);
  REFLECT(linked_relation_30);
  REFLECT(linked_relation_31);
  REFLECT(linked_relation_32);
  REFLECT(time_created);
  REFLECT(time_modified);
  REFLECT(extra_data); // don't allow scripts to depend on style specific data
  REFLECT_NAMELESS(data);
}
