//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make card games                |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <data/update_cards_script.hpp>
#include <data/action/set.hpp>
#include <data/set.hpp>
#include <data/card.hpp>
#include <data/stylesheet.hpp>
#include <data/action/value.hpp>

// ----------------------------------------------------------------------------- : UpdateCardsScript

IMPLEMENT_REFLECTION_NO_SCRIPT(UpdateCardsScript) {
  REFLECT(before_version);
  REFLECT(description);
  REFLECT(enabled);
  REFLECT(script);
}


void UpdateCardsScript::perform(Set& set, CardP& card, vector<CardP>& out) {
  // Perform script
  Context& ctx = set.getContext(card);
  if (enabled.hasBeenRead()) {
    enabled.update(ctx);
    if (!enabled()) return;
  }
  ScriptValueP result = script.invoke(ctx);
  // Add cards to out
  ScriptValueP it = result->makeIterator();
  while (ScriptValueP item = it->next()) {
    CardP new_card = from_script<CardP>(item);
    // is this a new card?
    if (contains(set.cards,new_card) || contains(out,new_card)) {
      // make copy
      new_card = make_intrusive<Card>(&set, new_card);
    }
    out.push_back(new_card);
  }
}

vector<CardP> UpdateCardsScript::perform(Set& set, CardP card) {
  // Perform script
  vector<CardP> cards;
  perform(set, card, cards);
  // Add to set
  if (!cards.empty()) {
    set.actions.addAction(make_unique<UpdateCardAction>(set, cards, vector<CardP>{card}), false);
  }
  return cards;
}
