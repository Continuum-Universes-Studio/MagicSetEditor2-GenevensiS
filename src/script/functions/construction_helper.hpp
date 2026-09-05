//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make card games                |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

#pragma once

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <data/field/text.hpp>
#include <data/field/choice.hpp>
#include <data/field/package_choice.hpp>
#include <data/field/color.hpp>
#include <data/field/image.hpp>
#include <data/field/symbol.hpp>
#include <data/action/set.hpp>
#include <data/game.hpp>
#include <data/set.hpp>
#include <data/stylesheet.hpp>
#include <data/card.hpp>
#include <wx/progdlg.h>

// ----------------------------------------------------------------------------- : Helper functions

inline static Value* get_card_field_container(Game& game, IndexMap<FieldP, ValueP>& map, String& key_name, bool ignore_field_not_found) {
  // find value container to update
  IndexMap<FieldP, ValueP>::const_iterator it = map.find(key_name);
  if (it == map.end()) {
    // look among alternate names
    std::map<String, String>::iterator alt_name_it = game.card_fields_alt_names.find(unified_form(key_name));
    if (alt_name_it != game.card_fields_alt_names.end()) {
      it = map.find(alt_name_it->second);
    }
  }
  if (it == map.end()) {
    if (ignore_field_not_found) return nullptr;
    throw ScriptError(_ERROR_2_("no field with name", _TYPE_("card"), key_name));
  }
  return it->get();
}

inline static Value* get_container(IndexMap<FieldP, ValueP>& map, const String& type, const String& key_name, bool ignore_field_not_found) {
  // find value container to update
  IndexMap<FieldP, ValueP>::const_iterator it = map.find(key_name);
  if (it == map.end()) {
    it = map.find(key_name.Lower());
    if (it == map.end()) {
      if (ignore_field_not_found) return nullptr;
      throw ScriptError(_ERROR_2_("no field with name", _TYPE_V_(type), key_name));
    }
  }
  return it->get();
}

inline static bool looks_like_url(const String& s) {
  // only the schemes we actually know how to fetch
  return s.starts_with(_("http://")) || s.starts_with(_("https://"))
      || s.starts_with(_("ftp://"))  || s.starts_with(_("ftps://"));
}

inline static bool looks_like_local_path(const String& s) {
  // unix absolute path
  if (s.starts_with(_("/"))) return true;
  // windows UNC / extended-length path:   \\server\share\... or \\?\C:\...
  if (s.starts_with(_("\\\\"))) return true;
  // windows absolute path
  if (s.size() >= 3 && wxIsalpha(s[0]) && s[1] == _(':') && (s[2] == _('\\') || s[2] == _('/'))) return true;
  return false;
}

inline static void set_image_from_external_string(Set* set, ImageValue* ivalue, const String& string, bool is_url) {
  try {
    GeneratedImageP loaded;
    if (is_url) {
      if (!settings.allow_image_download) {
        ivalue->filename.makeEmpty();
        return;
      }
      loaded = make_intrusive<DownloadedImage>(set, string);
    } else {
      loaded = make_intrusive<ImportedImage>(set, string);
    }
    ExternalImage* ext = dynamic_cast<ExternalImage*>(loaded.get());
    ivalue->filename = LocalFileName::fromReadString(ext->toString(), "");
  } catch (const ScriptError& e) {
    queue_message(MESSAGE_ERROR, e.what());
    ivalue->filename.makeEmpty();
  }
}

inline static void set_container(Set* set, Value* container, String type, ScriptValueP& value, String key_name) {
  // set the given value into the container
  if (TextValue* tvalue = dynamic_cast<TextValue*>(container)) {
    tvalue->value = value->toString();
  }
  else if (ChoiceValue* cvalue = dynamic_cast<ChoiceValue*>(container)) {
    cvalue->value = value->toString();
  }
  else if (PackageChoiceValue* pvalue = dynamic_cast<PackageChoiceValue*>(container)) {
    String package_name = value->toString();
    while (package_name.starts_with(_("/"))) package_name = package_name.substr(1);
    pvalue->package_name = package_name;
  }
  else if (ColorValue* cvalue = dynamic_cast<ColorValue*>(container)) {
    cvalue->value = value->toColor();
  }
  else if (ImageValue* ivalue = dynamic_cast<ImageValue*>(container)) {
    if (ExternalImage* img = dynamic_cast<ExternalImage*>(value.get())) {
      ivalue->filename = LocalFileName::fromReadString(img->toString(), "");
    } else if (value->type() == SCRIPT_STRING) {
      String str = value->toString();
      if (trim(str).empty()) {
        ivalue->filename.makeEmpty();
      } else if (looks_like_url(str)) {
        set_image_from_external_string(set, ivalue, str, true);
      } else if (looks_like_local_path(str)) {
        set_image_from_external_string(set, ivalue, str, false);
      } else {
        ivalue->filename = LocalFileName::fromReadString(str, "");
      }
    } else {
      throw ScriptError(_ERROR_1_("cant set image value", key_name));
    }
  }
  else if (SymbolValue* svalue = dynamic_cast<SymbolValue*>(container)) {
    if (value->type() == SCRIPT_STRING) {
      svalue->filename = LocalFileName::fromReadString(value->toString(), "");
    } else {
      throw ScriptError(_ERROR_1_("cant set symbol value", key_name));
    }
  }
  else {
    throw ScriptError(_ERROR_2_("cant set value", type, key_name));
  }
}

inline static bool set_stylesheet_container(const Game& game, CardP& card, ScriptValueP& value, String key_name, bool ignore_field_not_found) {
  // check if the given value is for a stylesheet, if found set it and return true
  key_name = unified_form(key_name);
  if (key_name == _("style") || key_name == _("stylesheet")) {
    if (!trim(value->toString()).empty()) {
      card->stylesheet = StyleSheet::byGameAndName(game, value->toString());
      if (card->stylesheet) {
        // Keep old styling data so matching fields (by name) can be carried over.
        IndexMap<FieldP, ValueP> old_styling_data = card->styling_data;
        card->styling_data.init(card->stylesheet->styling_fields);
        card->styling_data.copyDataFrom(old_styling_data);
        card->extraDataFor(*card->stylesheet).init(card->stylesheet->extra_card_fields);
      }
    }
    return true;
  }
  return false;
}

inline static bool set_builtin_container(const Game& game, Set* set, CardP& card, ScriptValueP& value, String key_name, bool ignore_field_not_found) {
  // check if the given value is for a built-in field, if found set it and return true
  key_name = unified_form(key_name);
  if (key_name == _("style") || key_name == _("stylesheet")) {
    return true; // we already took care of this
  }
  else if (key_name == _("style_version") || key_name == _("stylesheet_version")) {
    card->stylesheet_version = Version::fromString(value->toString());
    return true;
  }
  else if (key_name == _("notes") || key_name == _("note")) {
    card->notes = value->toString();
    return true;
  }
  else if (key_name == _("id") || key_name == _("uid")) {
    card->uid = value->toString();
    return true;
  }
  else if (Card::linkedCardFieldIndex(key_name) >= 0) {
    card->getLinkedUID(Card::linkedCardFieldIndex(key_name)) = value->toString();
    return true;
  }
  else if (Card::linkedRelationFieldIndex(key_name) >= 0) {
    card->getLinkedRelation(Card::linkedRelationFieldIndex(key_name)) = value->toString();
    return true;
  }
  else if          (key_name == _("styling_data")   || key_name == _("styling")
                 || key_name == _("style_data")     || key_name == _("stylesheet_data")
                 || key_name == _("extra_data")     || key_name == _("extra_card_data")) {
    bool is_extra = key_name == _("extra_data")     || key_name == _("extra_card_data");
    String type = is_extra ? _("extra") : _("styling");
    if (value->type() != SCRIPT_COLLECTION) {
      throw ScriptError(_ERROR_1_("styling data not map", type));
    }
    if (!card->stylesheet) {
      throw ScriptError(_ERROR_1_("styling data without stylesheet", type));
    }
    IndexMap<FieldP, ValueP>& data = is_extra ? card->extraDataFor(*card->stylesheet) : card->styling_data;
    ScriptValueP it = value->makeIterator();
    ScriptValueP key;
    while (ScriptValueP value = it->next(&key)) {
      assert(key);
      if (key == script_nil || value == script_nil) continue;
      String key_name = key->toString();
      Value* container = get_container(data, type, key_name, ignore_field_not_found);
      if (container == nullptr && ignore_field_not_found) continue;
      set_container(set, container, type, value, key_name);
      if (!is_extra) card->has_styling = true;
    }
    return true;
  }
  return false;
}

// Is key_name "linked_card"/"linked_card_N", "linked_relation"/"linked_relation_N", or the
// legacy "link_relation"/"link_relation_N" alias, for 1 <= N <= Card::MAX_LINKS?
inline static bool is_recognized_link_header(const String& key_name) {
  return Card::linkedCardFieldIndex(key_name) >= 0
      || Card::linkedRelationFieldIndex(key_name) >= 0
      || Card::indexedFieldIndex(key_name, _("link_relation"), Card::MAX_LINKS) >= 0;
}

inline static bool check_table_headers(GameP& game, std::vector<String>& headers, const String& file_extension, String& missing_fields_out) {
  if (headers.empty()) {
    queue_message(MESSAGE_ERROR, _("Empty headers given"));
    return false;
  }
  for (int x = 0; x < headers.size(); ++x) {
    String key_name = headers[x];
    if ( game->card_fields_alt_names.find(unified_form(key_name)) == game->card_fields_alt_names.end()
      && key_name != _("notes")
      && key_name != _("note")
      && key_name != _("style")
      && key_name != _("stylesheet")
      && key_name != _("id")
      && key_name != _("uid")
      && !is_recognized_link_header(key_name)
    ) {
      missing_fields_out += _("\n   ") + key_name;
    }
  }
  return true;
}

inline static bool cards_from_table(SetP& set, vector<String>& headers, std::vector<std::vector<ScriptValueP>>& table, bool ignore_field_not_found, const String& file_extension, vector<CardP>& cards_out) {
  // ensure table is square
  int count = headers.size();
  for (int y = 0; y < table.size(); ++y) {
    if (table[y].size() != count) {
      queue_message(MESSAGE_ERROR, _ERROR_1_("add card csv file malformed", wxString::Format(wxT("%i"), y+1)));
      return false;
    }
  }
  // set up context
  Context& ctx = set->getContext();
  ScriptValueP new_card_function = ctx.getVariable("new_card");
  ScriptValueP ctx_input = ctx.getVariableOpt(SCRIPT_VAR_input);
  ScriptValueP ctx_ignore = ctx.getVariableOpt("ignore_field_not_found");
  ctx.setVariable("ignore_field_not_found", to_script(ignore_field_not_found));
  int total = (int)table.size();
  // progress dialog, lets the user cancel
  std::unique_ptr<wxProgressDialog> progress;
  if (total > 0) {
    progress = std::make_unique<wxProgressDialog>(
      _TITLE_("importing cards"),
      wxString::Format(_LABEL_2_("importing cards", String()<<1, String()<<total)),
      total,
      nullptr,
      wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_CAN_ABORT
    );
  }
  bool cancelled = false;
  // produce cards from table
  for (int y = 0; y < total; ++y) {
    if (progress && !progress->Update(y, wxString::Format(_LABEL_2_("importing cards", String()<<(y+1), String()<<total)))) {
      cancelled = true;
      break;
    }
    ScriptCustomCollectionP field_map = make_intrusive<ScriptCustomCollection>();
    for (int x = 0; x < count; ++x) {
      // check if value is worth writing
      if (table[y][x] != script_nil) {
        field_map->key_value[headers[x]] = table[y][x];
      }
    }
    ctx.setVariable(SCRIPT_VAR_input, field_map);
    CardP card = from_script<CardP>(new_card_function->eval(ctx));
    // is this a new card?
    if (contains(set->cards, card) || contains(cards_out, card)) {
      // make copy
      card = make_intrusive<Card>(set.get(), card);
    }
    cards_out.push_back(card);
  }
  if (ctx_input) ctx.setVariable(SCRIPT_VAR_input, ctx_input);
  if (ctx_ignore) ctx.setVariable("ignore_field_not_found", ctx_ignore);
  return !cancelled;
}
