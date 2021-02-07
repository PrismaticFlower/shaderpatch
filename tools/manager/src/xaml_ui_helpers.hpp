#pragma once

#include "framework.hpp"

enum class text_style {
   header,
   subheader,
   title,
   subtitle,
   base,
   body,
   caption
};

inline void apply_text_style(winrt::Windows::UI::Xaml::Controls::TextBlock& text,
                             const text_style style)
{
   using winrt::Windows::UI::Text::FontWeights;

   switch (style) {
   case text_style::header:
      text.FontWeight(FontWeights::Light());
      text.FontSize(46.0);
      text.LineHeight(56.0);
      break;
   case text_style::subheader:
      text.FontWeight(FontWeights::Light());
      text.FontSize(34.0);
      text.LineHeight(40.0);
      break;
   case text_style::title:
      text.FontWeight(FontWeights::SemiLight());
      text.FontSize(24.0);
      text.LineHeight(28.0);
      break;
   case text_style::subtitle:
      text.FontWeight(FontWeights::Normal());
      text.FontSize(20.0);
      text.LineHeight(24.0);
      break;
   case text_style::base:
      text.FontWeight(FontWeights::SemiBold());
      text.FontSize(14.0);
      text.LineHeight(20.0);
      break;
   case text_style::body:
      text.FontWeight(FontWeights::Normal());
      text.FontSize(14.0);
      text.LineHeight(20.0);
      break;
   case text_style::caption:
      text.FontWeight(FontWeights::Normal());
      text.FontSize(12.0);
      text.LineHeight(14.0);
      break;
   }
}

inline void set_tooltip(winrt::Windows::UI::Xaml::Controls::Control control,
                        winrt::Windows::Foundation::IInspectable tooltip)
{
   using namespace winrt::Windows::UI::Xaml;

   Controls::ToolTipService::SetToolTip(control.as<DependencyObject>(), tooltip);
}

inline void set_tooltip(winrt::Windows::UI::Xaml::Controls::Control control,
                        winrt::hstring tooltip)
{
   set_tooltip(control, winrt::box_value(tooltip));
}
