//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make card games                |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <data/format/image_encoding.hpp>
#include <gfx/gfx.hpp>
#include <util/error.hpp>

// ----------------------------------------------------------------------------- : Linear Blend

// sqr(x) = x^2
template <typename T> inline T sqr(T x) { return x * x; }

void linear_blend(Image& img1, const Image& img2, double x1,double y1, double x2,double y2) {
  int width = img1.GetWidth(), height = img1.GetHeight();
  if (img2.GetWidth() != width || img2.GetHeight() != height) {
    throw Error(_ERROR_3_("blending different sizes", "linear_blend", String()<<width<<_("x")<<height, String()<<img2.GetWidth()<<_("x")<<img2.GetHeight()));
  }
  
  const int fixed = 1<<16; // fixed point multiplier
  // equation:
  //   x * xm + y * ym + d  ==  fixed * f(x,y)
  // xm and ym are multiples of delta x/y:
  //   xm = a w (x2-x1);  ym = a h (y2-y1)
  // known values
  //   f(x1*w, y1*h) = 0
  //   f(x2*w, y2*h) = 1
  // filling in:
  //   x1 * w * a * w * (x2-x1)  +  y1 * h * a * h * (y2-y1)  +  d  ==  0
  //   x2 * w * a * w * (x2-x1)  +  y2 * h * a * h * (y2-y1)  +  d  ==  fixed
  // solving for a and d:
  //   (using dx = x1-x2, dy = y1-y2)
  //   a = fixed / (w^2 dx^2 + h^2 dy^2)
  //   d = a * (w^2 x1 dx + h^2 y1 dy)
  if (x1==x2 && y1==y2) throw Error(_ERROR_("coordinates for blending overlap"));
  double a = fixed / (sqr(width) * sqr(x1-x2)  +  sqr(height) * sqr(y1-y2));
  int xm = to_int( (x2 - x1) * width  * a );
  int ym = to_int( (y2 - y1) * height * a );
  int d  = to_int( - (x1 * width * xm + y1 * height * ym) );
  
  Byte *data1 = img1.GetData(), *data2 = img2.GetData();
  // blend pixels
  for (int y = 0 ; y < height ; ++y) {
    for (int x = 0 ; x < width ; ++x) {
      int mult = x * xm + y * ym + d;
      if (mult < 0)      mult = 0;
      if (mult > fixed)  mult = fixed;
      data1[0] = data1[0] + mult * (data2[0] - data1[0]) / fixed;
      data1[1] = data1[1] + mult * (data2[1] - data1[1]) / fixed;
      data1[2] = data1[2] + mult * (data2[2] - data1[2]) / fixed;
      data1 += 3;
      data2 += 3;
    }
  }

  // Blend Alpha for the two images.
  if (img1.HasAlpha() && img2.HasAlpha()) {
    Byte *alpha1 = img1.GetAlpha(), *alpha2 = img2.GetAlpha();
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        int mult = x * xm + y * ym + d;
        if (mult < 0)      mult = 0;
        if (mult > fixed)  mult = fixed;
        alpha1[0] = alpha1[0] + mult * (alpha2[0] - alpha1[0]) / fixed;
        alpha1 += 1;
        alpha2 += 1;
      }
    }
  }

  //transfer metadata
  img1.SetOption(wxIMAGE_OPTION_PNG_DESCRIPTION, metadata_merge(img1, img2));
}

// ----------------------------------------------------------------------------- : Mask Blend

void mask_blend(Image& light, const Image& dark, const Image& mask) {
  int width = light.GetWidth(), height = light.GetHeight();
  if (dark.GetWidth() != width || dark.GetHeight() != height) {
    throw Error(_ERROR_3_("blending different sizes", "masked_blend", String()<<width<<_("x")<<height, String()<<dark.GetWidth()<<_("x")<<dark.GetHeight()));
  }
  if (mask.GetWidth() != width || mask.GetHeight() != height) {
    throw Error(_ERROR_3_("blending different mask", "masked_blend", String()<<width<<_("x")<<height, String()<<mask.GetWidth()<<_("x")<<mask.GetHeight()));
  }

  UInt size = width * height;
  // these have the following structure:
  // [pixel1red, pixel1green, pixel1blue, pixel2red, pixel2green, pixel2blue, pixel3red, etc...]
  Byte *data1 = light.GetData(), *data2 = dark.GetData(), *dataM = mask.GetData();
  // for each subpixel...
  for (UInt i = 0; i < (size * 3); ++i) {
    data1[i] = (data1[i] * dataM[i] + data2[i] * (255 - dataM[i])) / 255;
  }

  if (light.HasAlpha() && dark.HasAlpha()) {
    // these have the following structure:
    // [pixel1alpha, pixel2alpha, pixel3alpha, etc...]
    Byte *alphaLight = light.GetAlpha(), *alphaDark = dark.GetAlpha();
    for (UInt i = 0; i < size; ++i) {
      // use mask's red channel to blend alpha (all mask channels should be identical since it's grey scale)
      alphaLight[i] = (alphaLight[i] * dataM[i * 3] + alphaDark[i] * (255 - dataM[i * 3])) / 255;
    }
  }

  //transfer metadata
  light.SetOption(wxIMAGE_OPTION_PNG_DESCRIPTION, metadata_merge(light, dark));
}

// ----------------------------------------------------------------------------- : Alpha

void set_alpha(Image& img, const Image& img_alpha) {
  Image img_alpha_resampled = resample(img_alpha, img.GetWidth(), img.GetHeight());
  if (!img.HasAlpha()) img.InitAlpha();
  Byte *im = img.GetAlpha(), *al = img_alpha_resampled.GetData();
  size_t size = img.GetWidth() * img.GetHeight();
  for (size_t i = 0 ; i < size ; ++i) {
    im[i] = (im[i] * al[i*3]) / 255;
  }
}

void set_alpha(Image& img, Byte* al, const wxSize& alpha_size) {
  int width = img.GetWidth(), height = img.GetHeight();
  if (width != alpha_size.GetWidth() || height != alpha_size.GetHeight()) {
    throw Error(_ERROR_3_("blending different mask", "set_alpha", String()<<width<<_("x")<<height, String()<<alpha_size.GetWidth()<<_("x")<<alpha_size.GetHeight()));
  }
  if (!img.HasAlpha()) {
    // copy
    img.InitAlpha();
    memcpy(img.GetAlpha(), al, width * height);
  } else{
    // merge
    Byte *im = img.GetAlpha();
    size_t size = width * height;
    for (size_t i = 0 ; i < size ; ++i) {
      im[i] = (im[i] * al[i]) / 255;
    }
  }
}

void set_alpha(Image& img, double alpha) {
  size_t size = img.GetWidth() * img.GetHeight();
  if (alpha <= 0.0) {
    if (!img.HasAlpha()) img.InitAlpha();
    memset(img.GetAlpha(), Byte(0), size);
  }
  else if (alpha > 1.0) {
    if (!img.HasAlpha()) {
      img.InitAlpha();
      memset(img.GetAlpha(), Byte(255), size);
    } else {
      Byte *im = img.GetAlpha();
      for (size_t i = 0 ; i < size ; ++i) {
        im[i] = Byte(min(im[i] * alpha, 255.0));
      }
    }
  }
  else {
    Byte b_alpha = Byte(alpha * 255);
    if (!img.HasAlpha()) {
      img.InitAlpha();
      memset(img.GetAlpha(), b_alpha, size);
    } else {
      Byte *im = img.GetAlpha();
      for (size_t i = 0 ; i < size ; ++i) {
        im[i] = (im[i] * b_alpha) / 255;
      }
    }
  }
}
