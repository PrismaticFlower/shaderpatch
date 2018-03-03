using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;

using System.Windows.Data;


namespace installer
{
   class SelectedIndexToBool : IValueConverter
   {
      public object Convert(object value, Type targetType,
                            object parameter, CultureInfo cultureInfo)
      {
         int index = (int) value;

         return index != -1;
      }
      
      public object ConvertBack(object value, Type targetType,
                                object parameter, CultureInfo cultureInfo)
      {
         throw new NotImplementedException();
      }
   }
}
