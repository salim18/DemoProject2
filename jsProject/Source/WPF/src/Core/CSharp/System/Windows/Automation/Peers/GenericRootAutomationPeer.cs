using System;
using System.Runtime.InteropServices;
using System.Security;
using System.Security.Permissions;
using System.Text;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Media;
using System.ComponentModel;

using MS.Internal;
using MS.Win32;

// Used to support the warnings disabled below
#pragma warning disable 1634, 1691


namespace System.Windows.Automation.Peers
{

    /// 
    public class GenericRootAutomationPeer : UIElementAutomationPeer
    {
        ///
        public GenericRootAutomationPeer(UIElement owner): base(owner)
        {}
    
        ///
        override protected string GetClassNameCore()
        {
            return "Pane";
        }

        ///
        override protected AutomationControlType GetAutomationControlTypeCore()
        {
            return AutomationControlType.Pane;
        }

        ///
        /// <SecurityNote>
        ///     Critical: As this accesses Handle
        ///     TreatAsSafe: Returning the Window Title is considered safe - discussed on Automation TA review
        /// </SecurityNote>
        [SecurityCritical,SecurityTreatAsSafe]
        override protected string GetNameCore()
        {
            string name = base.GetNameCore();

            if(name == string.Empty)
            {
                IntPtr hwnd = this.Hwnd;
                if(hwnd != IntPtr.Zero)
                {
                    try
                    {
                        StringBuilder sb = new StringBuilder(512);

                        //This method elevates via SuppressUnmanadegCodeSecurity and throws Win32Exception on GetLastError
                        UnsafeNativeMethods.GetWindowText(new HandleRef(null, hwnd), sb, sb.Capacity);

                        name = sb.ToString();
                    }
// Allow empty catch statements.
#pragma warning disable 56502
                    catch(Win32Exception) {}
// Disallow empty catch statements.
#pragma warning restore 56502
                    
                    if (name == null)
                        name = string.Empty;
                }
            }

            return name;
        }

        ///
        ///<SecurityNote>
        ///     Critical as this method accesses critical data.
        ///     TreatAsSafe - window bounds by themselves is considered safe.
        ///</SecurityNote>
        [SecurityCritical, SecurityTreatAsSafe ]
        override protected Rect GetBoundingRectangleCore()
        {
            Rect bounds = new Rect(0,0,0,0);
            
            IntPtr hwnd = this.Hwnd;
            if(hwnd != IntPtr.Zero)
            {

                NativeMethods.RECT rc = new NativeMethods.RECT(0,0,0,0);
                try 
                { 
                    //This method elevates via SuppressUnmanadegCodeSecurity and throws Win32Exception on GetLastError
                    SafeNativeMethods.GetWindowRect(new HandleRef(null, hwnd), ref rc); 
                }
// Allow empty catch statements.
#pragma warning disable 56502
                catch(Win32Exception) {}
// Disallow empty catch statements.
#pragma warning restore 56502

                bounds = new Rect(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
            }

            return bounds;
        }
        
    }
}



