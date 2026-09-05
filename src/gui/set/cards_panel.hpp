//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make card games                |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

#pragma once

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <gui/set/panel.hpp>

class wxSplitterWindow;
class wxScrolledWindow;
class wxGridBagSizer;
class FilteredImageCardList;
class DataEditor;
class TextCtrl;
class CardViewer;
class wxSizer;
class wxButton;
class HoverButton;
class FindInfo;
class FilterCtrl;

// ----------------------------------------------------------------------------- : CardsPanel

/// A card list and card editor panel
class CardsPanel : public SetWindowPanel {
public:
  CardsPanel(Window* parent, int id);
  ~CardsPanel();
  
  void onChangeSet() override;
  
  // --------------------------------------------------- : UI
  
  void initUI   (wxToolBar* tb, wxMenuBar* mb) override;
  void destroyUI(wxToolBar* tb, wxMenuBar* mb) override;
  void onUpdateUI(wxUpdateUIEvent&) override;
  void onCommand(int id) override;
  void onMenuOpen(wxMenuEvent&) override;
  
  // --------------------------------------------------- : Actions
  
  bool wantsToHandle(const Action&, bool undone) const override;
  
  // --------------------------------------------------- : Clipboard
  bool canCut() const override;
  bool canCopy() const override;
  bool canPaste() const override;
  void doCut() override;
  void doCopy() override;
  void doPaste() override;

  // --------------------------------------------------- : Text selection

  bool canSelectAll() const override;
  void doSelectAll() override;

  // --------------------------------------------------- : Resetting

  bool canDefaultReset() const override;
  void doDefaultReset() override;

  // --------------------------------------------------- : Searching (find/replace)

  bool canFind()    const override { return true; }
  bool canReplace() const override { return true; }
  bool doFind      (wxFindReplaceData&) override;
  bool doReplace   (wxFindReplaceData&) override;
  bool doReplaceAll(wxFindReplaceData&) override;
private:
  /// Do a search or replace action for the given FindInfo in all cards
  bool search(FindInfo& find, bool from_start);
  class SearchFindInfo;
  class ReplaceFindInfo;
  friend class CardsPanel::SearchFindInfo;
  friend class CardsPanel::ReplaceFindInfo;
public:

  // --------------------------------------------------- : Selection
  CardP selectedCard() const override;
  void selectCard(const CardP& card) override;
  void selectFirstCard() override;
  void setCard(const CardP& card, bool event = false);
  void refreshCard(const CardP& card);

  void getCardLists(vector<CardListBase*>& out) override;

  void setFocusedEditor(DataEditor* editor);

  bool isUpdating() const override { return updating_card; }

private:
  // --------------------------------------------------- : Controls
  wxSizer*               s_left, *card_and_link;
  wxSplitterWindow*      splitter;
  DataEditor*            editor, *link_editor, *focused_editor;
  FilteredImageCardList* card_list;
  wxPanel*               nodes_panel;
  TextCtrl*              notes;
  wxScrolledWindow*      link_scroller;    ///< The linked cards area
  wxGridBagSizer*        link_boxes_sizer; ///< 2-column grid inside link_scroller
  vector<wxSizer*>       link_boxes;       ///< one box (wxStaticBoxSizer) per link slot
  vector<CardViewer*>    link_viewers;     ///< one card thumbnail per link slot
  vector<wxStaticText*>  link_relations;   ///< one relation label per link slot
  vector<wxButton*>      link_unlinks;     ///< one unlink button per link slot
  wxButton*              link_select;      ///< "select this card" button, only used for slot 0
  HoverButton*           collapse_notes;
  FilterCtrl*            filter;
  String                 filter_value;     ///< value of filter, need separate variable because the control is destroyed
  wxStaticText*          counts;
  bool                   notes_below_editor;
  bool                   updating_card = false;

  /// Update card counts
  void updateCardCounts();
  int selected_cards_count = 0;
  int filtered_cards_count = 0;
  int filtered_back_cards_count = 0;
  int total_cards_count = 0;
  /// Move the notes panel below the editor or below the card list
  void updateNotesPosition();
  /// Number of link boxes shown per row in link_boxes_sizer
  static const int LINK_BOX_COLUMNS = 2;
  /// Cap link_scroller's visible size so it uses a scroll bar if it gets bigger
  void updateLinkScrollerCap();
  /// Unlink the card shown in link slot `index` from the currently selected card
  void onUnlink(int index);
  // before Layout, call updateNotesPosition.
  // NOTE: docs say this function returns void, but the code says bool
  bool Layout() override;
  
  // --------------------------------------------------- : Menus & tools
  wxMenu* menuCard, *menuFormat;
  wxToolBarToolBase* toolAddCard;
  wxMenuItem* insertSymbolMenu;    // owned by menuFormat, but submenu owned by SymbolFont
  wxMenuItem* insertManyCardsMenu; // owned my menuCard, but submenu can be changed
  
  wxMenu* makeAddCardsSubmenu(bool add_single_card_option);
};

