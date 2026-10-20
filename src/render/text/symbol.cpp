//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make card games                |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <render/text/element.hpp>
#include <data/symbol_font.hpp>

// ----------------------------------------------------------------------------- : SymbolTextElement

void SymbolTextElement::draw(RotatedDC& dc, double scale, const RealRect& rect, const double* xs, DrawWhat what, size_t start, size_t end, bool native_look) const {
  if (!(what & DRAW_NORMAL)) return;
  if (font.font) {
    // SymbolFont::draw internally computes its font size as (scale * font.size()),
    // so divide by font.size() so the result comes out as (active_font_size * scale)
    double relative_scale = scale * active_font_size / max(0.01, font.size());
    font.font->draw(dc, ctx, rect, relative_scale, font, content.substr(start - this->start, end-start), active_font_color);
  }
}

void SymbolTextElement::getCharInfo(RotatedDC& dc, double scale, vector<CharInfo>& out) const {
  if (font.font) {
    font.font->getCharInfo(dc, ctx, active_font_size * scale, content.substr(start - this->start, end-start), out, active_font_color);
  }
}

double SymbolTextElement::minScale() const {
  return min(active_font_size, font.scale_down_to) / max(0.01, active_font_size);
}
double SymbolTextElement::scaleStep() const {
  return 1. / max(active_font_size * 4, 1.);
}
