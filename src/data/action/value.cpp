//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make card games                |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <util/tagged_string.hpp>
#include <data/action/value.hpp>
#include <data/field.hpp>
#include <data/card.hpp>
#include <data/set.hpp> // for ValueActionPerformer
#include <wx/imaglist.h>

// ----------------------------------------------------------------------------- : ValueAction

ValueAction::ValueAction(const ValueP& value)
  : valueP(value), card(nullptr), old_time_modified(wxDateTime::Now())
{}
ValueAction::~ValueAction() {} // here because we need the destructor of Card

String ValueAction::getName(bool to_undo) const {
  return _ACTION_1_("change", valueP->fieldP->name);
}

void ValueAction::perform(bool to_undo) {
  if (card) {
    swap(card->time_modified, old_time_modified);
  }
}

void ValueAction::setCard(CardP const& card) {
  const_cast<CardP&>(this->card) = card;
}

// ----------------------------------------------------------------------------- : Simple

unique_ptr<ValueAction> value_action(const TextValueP& value, const Defaultable<String>& new_value) {
  return make_unique<SimpleValueAction<TextValue, false>>(value, new_value);
}
unique_ptr<ValueAction> value_action(const ChoiceValueP& value, const Defaultable<String>& new_value) {
  return make_unique<SimpleValueAction<ChoiceValue, true>>(value, new_value);
}
unique_ptr<ValueAction> value_action(const ColorValueP& value, const Defaultable<Color>& new_value) {
  return make_unique<SimpleValueAction<ColorValue, true>>(value, new_value);
}
unique_ptr<ValueAction> value_action(const ImageValueP& value, const LocalFileName& new_value) {
  return make_unique<SimpleValueAction<ImageValue, false>>(value, new_value);
}
unique_ptr<ValueAction> value_action(const SymbolValueP& value, const LocalFileName& new_value) {
  return make_unique<SimpleValueAction<SymbolValue, false>>(value, new_value);
}
unique_ptr<ValueAction> value_action(const PackageChoiceValueP& value, const String& new_value) {
  return make_unique<SimpleValueAction<PackageChoiceValue, false>>(value, new_value);
}
unique_ptr<ValueAction> value_action(const MultipleChoiceValueP& value, const Defaultable<String>& new_value, const String& last_change) {
  MultipleChoiceValue::ValueType v = { new_value, last_change };
  return make_unique<SimpleValueAction<MultipleChoiceValue, false>>(value, v);
}


// ----------------------------------------------------------------------------- : Text

TextValueAction::TextValueAction(const TextValueP& value, size_t start, size_t end, size_t new_end, const Defaultable<String>& new_value, const String& name)
  : ValueAction(value)
  , selection_start(start), selection_end(end), new_selection_end(new_end)
  , new_value(new_value)
  , name(name)
{}

String TextValueAction::getName(bool to_undo) const { return name; }

void TextValueAction::perform(bool to_undo) {
  ValueAction::perform(to_undo);
  swap_value(value(), new_value);
  swap(selection_end, new_selection_end);
  valueP->onAction(*this, to_undo); // notify value
}

bool TextValueAction::merge(const Action& action) {
  TYPE_CASE(action, TextValueAction) {
    if (&action.value() == &value() && action.name == name) {
      if (action.selection_start == selection_end) {
        // adjacent edits, keep old value of this, it is older
        selection_end = action.selection_end;
        return true;
      } else if (action.new_selection_end == selection_start && name == _ACTION_("backspace")) {
        // adjacent backspaces
        selection_start = action.selection_start;
        selection_end   = action.selection_end;
        return true;
      }
    }
  }
  return false;
}

TextValue& TextValueAction::value() const {
  return static_cast<TextValue&>(*valueP);
}


unique_ptr<TextValueAction> toggle_format_action(const TextValueP& value, vector<String>& tags, size_t start_i, size_t end_i, size_t start, size_t end, const String& action_name) {
  if (start > end) {
    swap(start, end);
    swap(start_i, end_i);
  }
  // Find absolute position of the selection
  String new_value = value->value();
  size_t untagged_start_i = to_untagged_pos(new_value, start_i);
  size_t untagged_end_i   = to_untagged_pos(new_value, end_i);
  int offset = 0;
  // Compute the changes
  for (size_t i = 0; i < tags.size() ; ++i) {
    const String& tag = tags[i];
    new_value = tag.Contains(":") ? compute_new_variable_value(new_value, tag,    start_i, end_i):
                tag == _("li")    ? compute_new_bullet_value  (new_value, offset, start_i, end_i):
                                    compute_new_simple_value  (new_value, tag,    start_i, end_i);
    // Erase redundant tags
    if (start != end) {
      // Simplify
      new_value = simplify_tagged(new_value);
      // Adjust selection
      start_i = to_tagged_pos(new_value, untagged_start_i, true, true, true);
      end_i   = to_tagged_pos(new_value, untagged_end_i,   false, false, false);
    }
    else {
      // Don't simplify if start == end, this way we insert <b></b>,
      // allowing the user to press Ctrl+B and start typing bold text
      // Adjust selection
      start_i = to_tagged_pos(new_value, untagged_start_i, true, false, true);
      end_i   = to_tagged_pos(new_value, untagged_end_i,   true, false, true);
    }
  }
  // Build action
  if (value->value() == new_value) {
    return nullptr; // no changes
  } else {
    return make_unique<TextValueAction>(value, start+offset, end+offset, end+offset, new_value, action_name);
  }
}
unique_ptr<TextValueAction> toggle_format_action(const TextValueP& value, const String& tag, size_t start_i, size_t end_i, size_t start, size_t end, const String& action_name) {
  if (start > end) {
    swap(start, end);
    swap(start_i, end_i);
  }
  String new_value = value->value();
  int offset = 0;
  // Compute the changes
  new_value = tag.Contains(":") ? compute_new_variable_value(new_value, tag,    start_i, end_i):
              tag == _("li")    ? compute_new_bullet_value  (new_value, offset, start_i, end_i):
                                  compute_new_simple_value  (new_value, tag,    start_i, end_i);
  // Erase redundant tags
  if (start != end) {
    // don't simplify if start == end, this way we insert <b></b>, allowing the
    // user to press Ctrl+B and start typing bold text
    new_value = simplify_tagged(new_value);
  }
  // Build action
  if (value->value() == new_value) {
    return nullptr; // no changes
  } else {
    return make_unique<TextValueAction>(value, start+offset, end+offset, end+offset, new_value, action_name);
  }
}

String compute_new_simple_value(const String& str, const String& tag, size_t start_i, size_t end_i) {
  String prefix(substr(str, 0, start_i));
  String suffix(substr(str, end_i));
  String selection(substr(str, start_i, end_i - start_i));
  // if we are inside the tag we are toggling, 'remove' it
  if (in_tag(str, _("<") + tag, start_i, end_i) != String::npos) selection = anti_wrap_tag(selection, _("<") + tag + _(">"));
  // otherwise add it
  else                                                           selection =      wrap_tag(selection, _("<") + tag + _(">"));
  return prefix + selection + suffix;
}

String compute_new_variable_value(const String& str, const String& tag, size_t start_i, size_t end_i) {
  size_t start_tag = in_tag(str, _("<") + tag, start_i, end_i, true);
  // if we are inside the tag we are toggling, 'remove' it
  if (start_tag != String::npos) {
    String prefix(substr(str, 0, start_i));
    String suffix(substr(str, end_i));
    String selection(substr(str, start_i, end_i - start_i));
    selection = anti_wrap_tag(selection, _("<") + tag + _(">"));
    return prefix + selection + suffix;
  }
  // we are not inside this tag, add it around the selection, and
  // move all instances of variants of this tag to the sides of the selection
  
  // first, include surrounding formatting tags to the selection
  int start_temp = str.find_last_of("<", start_i - 1);
  while (start_temp != String::npos &&
         str.GetChar(start_i - 1) == '>' &&
         is_formatting_tag(str, start_temp)) {
    start_i = start_temp;
    start_temp = str.find_last_of("<", start_i - 1);
  }
  while (end_i < str.size() &&
         is_formatting_tag(str, end_i)) {
    end_i = str.find_first_of(">", end_i);
    if (end_i != String::npos) end_i++;
  }
  String prefix(substr(str, 0, start_i));
  String suffix = end_i == String::npos ? String() : String(substr(str, end_i));
  String selection(substr(str, start_i, end_i - start_i));

  // tally open tags that are variants of this tag
  vector<String> tag_list;
  map<String, int> tag_map;
  int start = 0;
  int end = 0;
  String tag_variants = _("<") + tag.substr(0, tag.find_first_of(":"));
  size_t size = tag_variants.size();
  String temp; temp.reserve(selection.size());
  for (; end < selection.size() ; ++end) {
    if (is_substr(selection, end, tag_variants)) {
      temp += selection.substr(start, end - start);
      start = end + 1;
      end = selection.find(">", end + size);
      String new_tag = selection.substr(start, end+1 - start);
      if (tag_map.find(new_tag) != tag_map.end()) {
        tag_map[new_tag] = tag_map[new_tag]+1;
      }
      else {
        tag_map[new_tag] = 1;
        tag_list.push_back(new_tag);
      }
      start = end+1;
    }
  }
  temp += selection.substr(start, end - start);

  // tally close tags
  selection.Clear(); selection.reserve(temp.size());
  String cejected_tag = close_tag(tag_variants);
  size_t csize = size + 1;
  for (start = 0, end = 0; end < temp.size() ; ++end) {
    if (is_substr(temp, end, cejected_tag)) {
      selection += temp.substr(start, end - start);
      start = end + 2;
      end = temp.find(">", end + csize);
      String new_tag = temp.substr(start, end+1 - start);
      if (tag_map.find(new_tag) != tag_map.end()) {
        tag_map[new_tag] = tag_map[new_tag]-1;
      }
      else {
        tag_map[new_tag] = -1;
        tag_list.push_back(new_tag);
      }
      start = end+1;
    }
  }
  selection += temp.substr(start, end - start);

  // add the actual tag we are toggling
  selection = wrap_tag(selection, _("<") + tag + _(">"));

  // add the tallied open and close tags
  for (size_t i = 0; i < tag_list.size() ; ++i) {
    String& new_tag = tag_list[i];
    int count = tag_map[new_tag];
    if (count > 0) {
      selection = selection + _("<") + new_tag;
    }
    else if (count < 0) {
      selection = _("</") + new_tag + selection;
    }
  }

  // add the stuff around the selection
  return prefix + selection + suffix;
}

String compute_new_bullet_value(const String& str, int& offset, size_t start_i, size_t end_i) {
  // Are we inside the tag we are toggling?
  size_t start_tag = in_tag(str, _("<li"), start_i, end_i);
  if (start_tag == String::npos) {
    // we are not inside this tag, add it
    // first, expand the selection to the end of the line
    end_i = min(str.find(_("<li"), end_i),
            min(str.find(_("<sep"), end_i),
            min(str.find(_("<suffix"), end_i),
            min(str.find(_("\n"), end_i),
            min(str.find(_("<line"), end_i),
                str.find(_("<soft-line"), end_i))))));
    String prefix(substr(str, 0, start_i));
    String suffix = end_i == String::npos ? String() : String(substr(str, end_i));
    String selection(substr(str, start_i, end_i - start_i));
    selection = wxString::FromUTF8("<li><bullet>• </bullet>") + selection + _("</li>");
    offset += 1;
    return prefix + selection + suffix;
  }
  // we are inside this tag, 'remove' it
  // first, expand the selection to include the tags
  start_i = start_tag;
  end_i = str.find(_("</li>"), start_tag);
  if (end_i != String::npos) end_i += 5;
  String prefix(substr(str, 0, start_i));
  String suffix = end_i == String::npos ? String() : String(substr(str, end_i));
  String selection(substr(str, start_i, end_i - start_i));
  selection = remove_tag(remove_tag_contents(selection, _("<bullet>")), _("<bullet>"));
  selection = remove_tag(selection, _("<li>"));
  offset -= 1;
  return prefix + selection + suffix;
}

unique_ptr<TextValueAction> typing_action(const TextValueP& value, size_t start_i, size_t end_i, size_t start, size_t end, const String& replacement, const String& action_name)  {
  bool reverse = start > end;
  if (reverse) {
    swap(start, end);
    swap(start_i, end_i);
  }
  String new_value = tagged_substr_replace(value->value(), start_i, end_i, replacement);
  if (value->value() == new_value) {
    // no change
    return nullptr;
  } else {
    if (reverse) {
      return make_unique<TextValueAction>(value, end, start, start+untag(replacement).size(), new_value, action_name);
    } else {
      return make_unique<TextValueAction>(value, start, end, start+untag(replacement).size(), new_value, action_name);
    }
  }
}

// ----------------------------------------------------------------------------- : Reminder text

TextToggleReminderAction::TextToggleReminderAction(const TextValueP& value, size_t pos_in)
  : ValueAction(value)
{
  pos = in_tag(value->value(), _("<kw"), pos_in, pos_in);
  if (pos == String::npos) {
    throw InternalError(_("TextToggleReminderAction: not in <kw- tag"));
  }
  Char c = value->value().GetChar(pos + 4);
  enable = !(c == _('1') || c == _('A')); // if it was not enabled, then enable it
  old = enable ? _('1') : _('0');
}
String TextToggleReminderAction::getName(bool to_undo) const {
  return enable ? _ACTION_("show reminder text") : _ACTION_("hide reminder text");
}

void TextToggleReminderAction::perform(bool to_undo) {
  ValueAction::perform(to_undo);
  TextValue& value = static_cast<TextValue&>(*valueP);
  String& val = value.value.mutate();
  assert(pos + 4 < val.size());
  size_t end = match_close_tag(val, pos);
  wxUniChar c = old;
  old = val[pos + 4];
  val[pos + 4] = c;
  if (end != String::npos && end + 5 < val.size()) {
    val[end + 5] = c; // </kw-c>
  }
  value.last_update.update();
  value.onAction(*this, to_undo); // notify value
}


// ----------------------------------------------------------------------------- : Event

String ScriptValueEvent::getName(bool) const {
  assert(false); // this action is just an event, getName shouldn't be called
  throw InternalError(_("ScriptValueEvent::getName"));
}
void ScriptValueEvent::perform(bool) {
  assert(false); // this action is just an event, it should not be performed
}


String ScriptStyleEvent::getName(bool) const {
  assert(false); // this action is just an event, getName shouldn't be called
  throw InternalError(_("ScriptStyleEvent::getName"));
}
void ScriptStyleEvent::perform(bool) {
  assert(false); // this action is just an event, it should not be performed
}

// ----------------------------------------------------------------------------- : Bulk action

BulkAction::BulkAction(const vector<ActionP>& actions, const SetP& set, CardListBase* card_list_window, bool change_name)
  : actions(actions), set(set), card_list_window(card_list_window)
{
  if (actions.empty()) throw ScriptError(_("BulkAction created with no actions"));
  if (change_name) {
    name_do = _ACTION_1_("bulk", actions.front()->getName(false));
    name_undo = _ACTION_1_("bulk", actions.front()->getName(true));
  }
  else {
    name_do = actions.front()->getName(false);
    name_undo = actions.front()->getName(true);
  }
}
BulkAction::~BulkAction() {}

String BulkAction::getName(bool to_undo) const {
  return to_undo ? name_undo : name_do;
}

void BulkAction::perform(bool to_undo) {
  size_t size = actions.size();
  for (size_t i = 0 ; i < size ; ++i) {
    ActionP& action = actions[to_undo ? size - i - 1 : i];
    action->perform(to_undo);
    set->actions.tellListeners(*action, to_undo);
  }
  card_list_window->Refresh();
}

bool BulkAction::merge(const Action& action) {
  return false;
}

// ----------------------------------------------------------------------------- : Action performer

ValueActionPerformer::ValueActionPerformer(const ValueP& value, CardP const& card, const SetP& set)
  : value(value), card(card), set(set)
{}
ValueActionPerformer::~ValueActionPerformer() {}

void ValueActionPerformer::addAction(unique_ptr<ValueAction>&& action) {
  action->setCard(card);
  set->actions.addAction(move(action));
}

Package& ValueActionPerformer::getLocalPackage() {
  return *set;
}
