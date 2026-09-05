//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make card games                |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <gui/control/card_viewer.hpp>
#include <gui/control/image_card_list.hpp>
#include <gui/set/cards_panel.hpp>
#include <data/stylesheet.hpp>
#include <data/settings.hpp>
#include <render/value/viewer.hpp>
#include <wx/dcbuffer.h>
#include <util/window_id.hpp>

// ----------------------------------------------------------------------------- : Events

DEFINE_EVENT_TYPE(EVENT_SIZE_CHANGE);

// ----------------------------------------------------------------------------- : CardViewer

CardViewer::CardViewer(Window* parent, int id, long style)
  : wxControl(parent, id, wxDefaultPosition, wxDefaultSize, style)
  , up_to_date(false), is_focused(true), redraw_pending(false), full_repaint(false)
{
  redraw_timer.SetOwner(this, ID_REDRAW_TIMER);
  SetBackgroundStyle(wxBG_STYLE_PAINT);
}

wxSize CardViewer::DoGetBestSize() const {
  wxSize ws = GetSize(), cs = GetClientSize();
  if (set) {
    if (!stylesheet) stylesheet = set->stylesheet;
    StyleSheetSettings& ss = settings.stylesheetSettingsFor(*stylesheet);
    double dpi_factor = stylesheet->card_dpi <= 150.0 ? 1.0 : 150.0 / stylesheet->card_dpi;
    double width = stylesheet->card_width * dpi_factor * ss.card_zoom();
    double height = stylesheet->card_height * dpi_factor * ss.card_zoom();
    double link_factor = GetId() == ID_CARD_LINK_VIEWER ? (height * 0.5 - 41.0) / height : GetId() == ID_CARD_LINK_EDITOR ? (height * 0.96 - 41.0) / height : 1.0; // Subtract 41 pixels for the link title
    wxSize size(int(link_factor * width), int(link_factor * height));
    if (is_sideways(deg_to_rad(ss.card_angle()))) swap(size.x, size.y);
    return size + ws - cs;
  }
  return cs;
}

void CardViewer::redraw(const ValueViewer& v, bool force) {
  // Don't refresh if we OR ANOTHER CardViewer is drawing
  // drawing another viewer causes styles to be updated for its active card, which may be different,
  // causing the two viewers to continously refresh.
  if (drawing_card()) return;
  up_to_date = false;
  // Don't refresh if we are out of focus. We will be refreshed in the next onIdle call.
  if (!force && !is_focused) {
    redraw_pending = true;
    return;
  }
  RefreshRect(getRotation().trRectToBB(v.boundingBoxBorder()), false);
}

void CardViewer::onChange(bool force) {
  redraw(force);
}

void CardViewer::redraw(bool force) {
  if (drawing_card()) return;
  up_to_date = false;
  if (!force && !is_focused) {
    redraw_pending = true;
    return;
  }
  Refresh(false);
}

void CardViewer::onChangeSize() {
  InvalidateBestSize();
  wxSize ws = GetSize(), cs = GetClientSize();
  wxSize desired_cs = (wxSize)getRotation().getExternalSize() + ws - cs;
  if (desired_cs != cs) {
    wxCommandEvent ev(EVENT_SIZE_CHANGE, GetId());
    ProcessEvent(ev);
  }
}

#ifdef _DEBUG
  DECLARE_DYNAMIC_ARG(bool, inOnPaint);
  IMPLEMENT_DYNAMIC_ARG(bool, inOnPaint, false);
#endif

void CardViewer::onPaint(wxPaintEvent&) {
  #ifdef _DEBUG
    // we don't want recursion
    if (inOnPaint()) {
      wxTrap();
    }
    WITH_DYNAMIC_ARG(inOnPaint, true);
  #endif
  wxSize cs = GetClientSize();
  if (cs.GetWidth() == 0 || cs.GetHeight() == 0) {
    return; // empty bitmaps are not allowed because some idiots think that 0 is not a number
  }
  if (!buffer.Ok() || buffer.GetWidth() != cs.GetWidth() || buffer.GetHeight() != cs.GetHeight()) {
    buffer = Bitmap(cs.GetWidth(), cs.GetHeight());
    up_to_date = false;
  }
  wxBufferedPaintDC dc(this, buffer);
  // scrolling
//  int dx = GetScrollPos(wxHORIZONTAL), dy = GetScrollPos(wxVERTICAL);
//  dc.SetDeviceOrigin(-dx, -dy);
  wxRegion clip = GetUpdateRegion();
//  clip.Offset(dx, dy);
  if (!up_to_date) {
    // The buffer may contain regions that were never actually painted
    clip = wxRegion(0, 0, cs.GetWidth(), cs.GetHeight());
  }
  dc.SetDeviceClippingRegion(clip);
  // draw
  if (!up_to_date) {
    up_to_date = true;
    full_repaint = true;
    try {
      draw(dc);
    } CATCH_ALL_ERRORS(false); // don't show message boxes in onPaint!
    full_repaint = false;
  }
}

void CardViewer::onClick(wxMouseEvent& ev) {
  if (GetId() == ID_CARD_LINK_VIEWER) {
    // walk up to the owning CardsPanel -- can't assume it's the immediate parent, since
    // link viewers may be nested inside e.g. a scrolled window
    CardsPanel* panel = nullptr;
    for (wxWindow* w = GetParent(); w && !panel; w = w->GetParent()) {
      panel = dynamic_cast<CardsPanel*>(w);
    }
    if (panel) {
      panel->setCard(getCard(), true);
    }
  }
  else {
    ev.Skip(); // allow DataEditor::onLeftDown to process this event
  }
}

void CardViewer::drawViewer(RotatedDC& dc, ValueViewer& v) {
  if (shouldDraw(v)) v.draw(dc);
}

bool CardViewer::shouldDraw(const ValueViewer& v) const {
//  int dx = GetScrollPos(wxHORIZONTAL), dy = GetScrollPos(wxVERTICAL);
//  wxRegion clip = GetUpdateRegion();
//  clip.Offset(dx, dy);
  if (full_repaint) return true;
  return GetUpdateRegion().Contains(getRotation().trRectToBB(v.boundingBoxBorder().toRect()).toRect()) != wxOutRegion;
}

void CardViewer::onIdle(wxIdleEvent&) {
  if (is_focused) return;
  if (redraw_pending) {
    redraw_pending = false;
    if (!redraw_timer.IsRunning()) redraw_timer.Start(1500, wxTIMER_ONE_SHOT);
  }
}

void CardViewer::onTimer(wxTimerEvent& ev) {
  Refresh(false);
}

// helper class for overdrawDC()
class CardViewer::OverdrawDC_aux : private wxClientDC {
protected:
  wxBufferedDC bufferedDC;
  
  OverdrawDC_aux(CardViewer* window)
    : wxClientDC(window)
  {
    bufferedDC.Init((wxClientDC*)this, window->buffer);
  }
};
class CardViewer::OverdrawDC : private OverdrawDC_aux, public RotatedDC {
public:
  OverdrawDC(CardViewer* window)
    : OverdrawDC_aux(window)
    , RotatedDC(bufferedDC, window->getRotation(), QUALITY_LOW)
  {}
};

shared_ptr<RotatedDC> CardViewer::overdrawDC() {
  #ifdef _DEBUG
    // don't call from onPaint
    if (inOnPaint()) {
      wxTrap();
    }
  #endif
  return shared_ptr<RotatedDC>(new OverdrawDC(this));
}

Rotation CardViewer::getRotation() const {
  // Same as DataViewer::getRotation, only taking into account scrolling
  if (!stylesheet) stylesheet = set->stylesheet;
  StyleSheetSettings& ss = settings.stylesheetSettingsFor(*stylesheet);
  int dx = CanScroll(wxHORIZONTAL) ? GetScrollPos(wxHORIZONTAL) : 0;
  int dy = CanScroll(wxVERTICAL) ? GetScrollPos(wxVERTICAL) : 0;
  double dpi_factor = stylesheet->card_dpi <= 150.0 ? 1.0 : 150.0 / stylesheet->card_dpi;
  double height = stylesheet->card_height * dpi_factor * ss.card_zoom();
  double link_factor = GetId() == ID_CARD_LINK_VIEWER ? (height * 0.5 - 41.0) / height : GetId() == ID_CARD_LINK_EDITOR ? (height * 0.96 - 41.0) / height : 1.0; // Subtract 41 pixels for the link title
  return Rotation(deg_to_rad(ss.card_angle()), stylesheet->getCardRect().move(-dx,-dy,0,0), link_factor * dpi_factor * ss.card_zoom(), 1.0, ROTATION_ATTACH_TOP_LEFT);
}

// ----------------------------------------------------------------------------- : Event table

BEGIN_EVENT_TABLE(CardViewer, wxControl)
  EVT_PAINT(CardViewer::onPaint)
  EVT_LEFT_DOWN(CardViewer::onClick)
  EVT_IDLE(CardViewer::onIdle)
  EVT_TIMER(ID_REDRAW_TIMER, CardViewer::onTimer)
END_EVENT_TABLE  ()
