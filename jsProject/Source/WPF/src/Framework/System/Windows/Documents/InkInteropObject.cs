//---------------------------------------------------------------------------
//
// <copyright file=InkInteropObject.cs company=Microsoft>
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: InkInteropObject is an image object that links IComDataObject.
//
// History:
//  09/25/2003 : yutakas
//
//---------------------------------------------------------------------------

#if ENABLE_INK_EMBEDDING

using System;
using System.Runtime.InteropServices;
using System.Windows.Threading;

using System.Diagnostics;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Input;
using System.Windows.Documents;
using System.Windows.Controls;
using MS.Win32;

using IComDataObject = System.Runtime.InteropServices.ComTypes.IDataObject;

namespace System.Windows.Documents
{
    //
    // InkInteropObject is Image object that links IComDataObject, which 
    // is insterted by ITextStoreACP::InsertEmbedded().
    //
    internal class InkInteropObject : Image
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        // Creates a new InkInteropObject instance.
        internal InkInteropObject(IComDataObject idataobject)
        {
            _idataobject = idataobject;
        }

        #endregion Constructors

        //------------------------------------------------------
        //
        //  Internal Methods
        //
        //------------------------------------------------------

        #region Internal Methods

        // Measure side to fit the current font size.
        protected override Size MeasureOverride(Size constraint)
        {
            ImageSource imageSource = Source;

            if (imageSource == null)
            {
                return new Size();
            }

            double fontheight = (double)GetValue(TextElement.FontSizeProperty);

            return new Size(imageSource.Width * fontheight / imageSource.Height, fontheight);
       }

        #endregion Internal Methods

        //------------------------------------------------------
        //
        //  Internal Properties
        //
        //------------------------------------------------------

        #region Internal Properties

        // TextStore::GetEmbedded() returns IComDataObject.
        internal IComDataObject OleDataObject
        {
            get  {return _idataobject;}
        }

        #endregion Internal Properties

        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        #region Private Fields

        // keep the reference to Win32 IDataObject.
        private IComDataObject _idataobject;

        #endregion Private Fields
    }
}

#endif // ENABLE_INK_EMBEDDING
