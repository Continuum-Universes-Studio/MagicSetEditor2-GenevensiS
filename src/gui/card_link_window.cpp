//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make card games                |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <data/game.hpp>
#include <data/card_link.hpp>
#include <data/action/value.hpp>
#include <gui/card_link_window.hpp>
#include <gui/control/select_card_list.hpp>
#include <util/window_id.hpp>
#include <data/action/set.hpp>
#include <wx/statline.h>
#include <unordered_set>

// ----------------------------------------------------------------------------- : ExportCardSelectionChoice

CardLinkWindow::CardLinkWindow(Window* parent, const SetP& set, const CardP& selected_card, bool sizer)
  : wxDialog(parent, wxID_ANY, _TITLE_("link cards"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
  , set(set), parent(parent), selected_card(selected_card)
{
  // init controls
  selected_relation = new wxTextCtrl(this, wxID_ANY, _(""));
  linked_relation = new wxTextCtrl(this, wxID_ANY, _(""));
  relation_type = new wxChoice(this, ID_CARD_LINK_TYPE, wxDefaultPosition, wxDefaultSize, 0, nullptr);
  relation_type->Clear();
  FOR_EACH(link, set->game->card_links) {
    relation_type->Append(link->name());
  }
  relation_type->Append(_LABEL_("custom link"));
  relation_type->SetSelection(0);
  setRelationType();
  list = new SelectCardList(this, wxID_ANY);
  list->setSet(set);
  list->selectNone();
  sel_none = new wxButton(this, ID_SELECT_NONE, _BUTTON_("select none"));
  // init sizers
  if (sizer) {
    wxSizer* s = new wxBoxSizer(wxVERTICAL);
    s->Add(new wxStaticText(this, -1, _LABEL_1_("linked cards relation", selected_card->identification())), 0, wxALL, 8);
    s->Add(relation_type, 0, wxEXPAND | (wxALL & ~wxTOP), 8);
    s->Add(new wxStaticText(this, -1, _("  ") + _LABEL_("selected card")), 0, wxALL, 4);
    s->Add(selected_relation, 0, wxEXPAND | (wxALL & ~wxTOP), 8);
    s->Add(new wxStaticText(this, -1, _("  ") + _LABEL_("linked cards")), 0, wxALL, 4);
    s->Add(linked_relation, 0, wxEXPAND | (wxALL & ~wxTOP), 8);
    s->Add(new wxStaticText(this, wxID_ANY, _LABEL_("select linked cards")), 0, wxALL & ~wxBOTTOM, 8);
    s->Add(list, 1, wxEXPAND | wxALL, 8);
    wxSizer* s2 = new wxBoxSizer(wxHORIZONTAL);
    s2->Add(sel_none, 0, wxEXPAND | wxRIGHT, 8);
    s2->Add(CreateButtonSizer(wxOK | wxCANCEL), 1, wxEXPAND, 8);
    s->Add(s2, 0, wxEXPAND | (wxALL & ~wxTOP), 8);
    SetSizer(s);
    s->SetSizeHints(this);
    SetSize(600,500);
  }
}

bool CardLinkWindow::isSelected(const CardP& card) const {
  return list->isSelected(card);
}

void CardLinkWindow::getSelection(vector<CardP>& out) const {
  list->getSelection(out);
}

void CardLinkWindow::setSelection(const vector<CardP>& cards) {
  list->setSelection(cards);
  queue_message(MESSAGE_WARNING, String("") << cards.size());
}
void CardLinkWindow::setRelationType() {
  int index = relation_type->GetSelection();
  if (index >= set->game->card_links.size()) { // Custom type
    selected_relation->ChangeValue(_LABEL_("custom link selected"));
    selected_relation->Enable();
    linked_relation->ChangeValue(_LABEL_("custom link linked"));
    linked_relation->Enable();
  }
  else {
    CardLinkP link = set->game->card_links[index];
    selected_relation->ChangeValue(link->selected.get());
    selected_relation->Enable(false);
    linked_relation->ChangeValue(link->linked.get());
    linked_relation->Enable(false);
  }
}

void CardLinkWindow::onSelectNone(wxCommandEvent&) {
  list->selectNone();
}

void CardLinkWindow::onRelationTypeChange(wxCommandEvent&) {
  setRelationType();
}

void CardLinkWindow::onOk(wxCommandEvent&) {
  wxBusyCursor wait;
  // get the context
  CardListBase* card_list_window = dynamic_cast<CardListBase*>(parent);
  if (!card_list_window) {
    queue_message(MESSAGE_ERROR, _("Bulk modification must be called from a card list window!"));
    EndModal(wxID_ABORT);
    return;
  }
  // Perform the linking
  // The selected_card is the one selected on the main cards tab
  // The linked_cards are the ones selected in this dialogue window
  vector<CardP> linked_cards;
  getSelection(linked_cards);
  // Check that we are not linking to self
  if (std::find(linked_cards.begin(), linked_cards.end(), selected_card) != linked_cards.end()) {
    wxMessageDialog dial = wxMessageDialog(this, _ERROR_("cant link to self"), _TITLE_("warning"), wxICON_WARNING | wxOK);
    dial.ShowModal();
    linked_cards.erase(std::remove(linked_cards.begin(), linked_cards.end(), selected_card), linked_cards.end());
  }
  if (linked_cards.empty()) {
    wxMessageDialog dial = wxMessageDialog(this, _ERROR_("no cards selected"), _TITLE_("warning"), wxICON_WARNING | wxOK);
    dial.ShowModal();
    return;
  }
  vector<String> linked_uids;
  for (size_t i = 0; i < linked_cards.size(); ++i) {
    linked_uids.push_back(linked_cards[i]->uid);
  }
  // Find free links
  vector<int> free_link_indexes = selected_card->findFreeLinks(linked_uids, set->card_uids);
  int free_link_count = 0;
  for (size_t i = 0; i < free_link_indexes.size(); ++i) {
    if (free_link_indexes[i] >= 0) free_link_count++;
  }
  if (free_link_count < linked_cards.size()) {
    wxMessageDialog dial = wxMessageDialog(this, _ERROR_1_("missing free links", wxString::Format(wxT("%i"), free_link_count)));
    dial.ShowModal();
    return;
  }
  // Get the relations
  String selected_relation_string;
  String linked_relation_string;
  int index = relation_type->GetSelection();
  if (index >= set->game->card_links.size()) { // Custom type
    selected_relation_string = selected_relation->GetValue();
    linked_relation_string = linked_relation->GetValue();
  }
  else {
    CardLinkP link = set->game->card_links[index];
    selected_relation_string = link->selected.default_;
    linked_relation_string = link->linked.default_;
  }
  // Make the actions
  vector<ActionP> actions;
  for (size_t i = 0; i < free_link_indexes.size(); ++i) {
    if (free_link_indexes[i] >= 0) {
      actions.push_back(make_intrusive<OneWayLinkCardsAction>(selected_card, linked_uids[i], linked_relation_string, free_link_indexes[i]));
    }
  }
  // Find reciprocal free slots and make actions
  String& selected_uid = selected_card->uid;
  for (size_t i = 0; i < linked_cards.size(); ++i) {
    int free_link_index = linked_cards[i]->findFreeLink(selected_uid, set->card_uids);
    if (free_link_index >= 0) {
      actions.push_back(make_intrusive<OneWayLinkCardsAction>(linked_cards[i], selected_uid, selected_relation_string, free_link_index));
    }
  }
  // Add action to set
  set->actions.addAction(make_unique<BulkAction>(actions, set, card_list_window, false), false);
  set->actions.tellListeners(DisplayChangeAction(),true);
  // Done
  EndModal(wxID_OK);
}

BEGIN_EVENT_TABLE(CardLinkWindow, wxDialog)
EVT_BUTTON       (ID_SELECT_NONE, CardLinkWindow::onSelectNone)
EVT_BUTTON       (wxID_OK, CardLinkWindow::onOk)
EVT_CHOICE       (ID_CARD_LINK_TYPE, CardLinkWindow::onRelationTypeChange)
END_EVENT_TABLE  ()
