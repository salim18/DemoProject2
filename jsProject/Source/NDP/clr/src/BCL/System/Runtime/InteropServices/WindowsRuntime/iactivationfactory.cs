﻿// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// <OWNER>[....]</OWNER>
// <OWNER>[....]</OWNER>
// <OWNER>[....]</OWNER>

using System;
using System.Runtime.InteropServices;

namespace System.Runtime.InteropServices.WindowsRuntime
{
    [ComImport]
    [Guid("00000035-0000-0000-C000-000000000046")]
    [WindowsRuntimeImport]
    public interface IActivationFactory
    {
        object ActivateInstance();
    }
}
