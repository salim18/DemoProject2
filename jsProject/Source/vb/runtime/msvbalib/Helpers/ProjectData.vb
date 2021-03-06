'*****************************************************************************/
'* ProjectData.vb
'*
'*  Copyright (c) Microsoft Corporation.  All rights reserved.
'*  Information Contained Herein Is Proprietary and Confidential.
'*
'* Purpose:
'*  Holds all project level information
'*  This class is used by the compiler to throw exceptions at runtime.
'*  Inside the catch statement the compiler generates code to call:
'*  ProjectData::SetProjectError( exception)  That call
'*  in turns sets the ErrObject which is accessed via the VB Err statement.
'*  So a VB6 programmer would typically then do something like:
'*    if err.Number = * do something where err accesses the ErrObject that
'*  is set by this class.
'*****************************************************************************/

Imports System
Imports System.Globalization
Imports System.Threading
Imports System.Security
Imports System.Security.Permissions
Imports System.Runtime.ConstrainedExecution

Imports Microsoft.VisualBasic.CompilerServices.Utils

Namespace Microsoft.VisualBasic.CompilerServices

    <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    Friend NotInheritable Class AssemblyData
        Friend Sub New()
            Dim i As Integer
            Dim o As Object
            Dim files As Collections.ArrayList = New Collections.ArrayList(256)

            o = Nothing

            For i = 0 To 255
                files.Add(o)
            Next
            m_Files = files
        End Sub

        Friend Function GetChannelObj(ByVal lChannel As Integer) As VB6File
            Dim o As Object

            If (lChannel < m_Files.Count) Then
                o = m_Files.Item(lChannel)
            Else
                o = Nothing
            End If

            Return CType(o, VB6File)
        End Function

        Friend Sub SetChannelObj(ByVal lChannel As Integer, ByVal oFile As VB6File)
            If m_Files Is Nothing Then
                m_Files = New Collections.ArrayList(256)
            End If

            Dim o As Object

            If oFile Is Nothing Then
                Dim f As VB6File
                f = CType(m_Files.Item(lChannel), VB6File)
                If (Not f Is Nothing) Then
                    f.CloseFile()
                End If
                m_Files.Item(lChannel) = Nothing
            Else
                o = oFile
                m_Files.Item(lChannel) = o
            End If
        End Sub

        Public m_Files As Collections.ArrayList
        Friend m_DirFiles() As IO.FileSystemInfo
        Friend m_DirNextFileIndex As Integer
        Friend m_DirAttributes As IO.FileAttributes

    End Class

    <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    Public NotInheritable Class ProjectData

        Friend m_Err As ErrObject
        Friend m_rndSeed As Integer = &H50000I
        Friend m_numprsPtr() As Byte
        Friend m_DigitArray() As Byte
        Friend m_AssemblyData As Collections.Hashtable

        'm_oProject is per-Thread for each AppDomain
        <System.ThreadStaticAttribute()> Private Shared m_oProject As ProjectData

        Private Sub New()
            MyBase.New()

            m_AssemblyData = New System.Collections.Hashtable

            Const DIGIT_ARRAY_SIZE As Integer = 30
            Const NUMPRS_SIZE As Integer = 24

            ReDim m_numprsPtr(NUMPRS_SIZE - 1)
            ReDim m_DigitArray(DIGIT_ARRAY_sIZE - 1)

        End Sub

        Private m_CachedMSCoreLibAssembly As System.Reflection.Assembly = GetType(System.Int32).Assembly

        Friend Function GetAssemblyData(ByVal assem As System.Reflection.Assembly) As AssemblyData
            'The first time, we will get an exception, but the remainder of the time there will be less overhead
            '
            'BEGIN: SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY
            '
            If assem Is Utils.VBRuntimeAssembly OrElse assem Is m_CachedMSCoreLibAssembly Then
                'Must have been from a latebound call to our own apis (potentially through context of mscorlib)
                'This must not be allowed, as it would cause files to be shared across assemblies
                Throw New Security.SecurityException(GetResourceString(ResID.Security_LateBoundCallsNotPermitted))
            End If
            '
            'END: SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY
            '

            Dim AssemData As AssemblyData = CType(m_AssemblyData.Item(assem), AssemblyData)
            If (AssemData Is Nothing) Then
                AssemData = New AssemblyData
                m_AssemblyData.Item(assem) = AssemData
            End If

            Return AssemData
        End Function

        Friend Shared Function GetProjectData() As ProjectData
            '*************************
            '*** PERFORMANCE NOTE: ***
            '*************************
            ' m_oProject is <ThreadStatic>
            ' and is pretty expensive to access so we cache to a local
            ' to cut the number of accesses in half
            GetProjectData = m_oProject
            If GetProjectData Is Nothing Then
                GetProjectData = New ProjectData
                m_oProject = GetProjectData
            End If
        End Function

        ''' <summary>
        ''' This function is called by the compiler in response to err code, e.g. err 123
        ''' It is also called when the compiler encounters a resume that isn't preceded by an On Error command
        ''' </summary>
        ''' <param name="hr"></param>
        ''' <returns></returns>
        ''' <remarks></remarks>
        Public Shared Function CreateProjectError(ByVal hr As Integer) As System.Exception
            '*************************
            '*** PERFORMANCE NOTE: ***
            '*************************
            ' Err Object is <ThreadStatic> and is pretty expensive to access so we cache to a local to cut the number of accesses
            Dim ErrObj As ErrObject = Err()
            ErrObj.Clear()
            Dim ErrNumber As Integer = ErrObj.MapErrorNumber(hr)
            Return ErrObj.CreateException(hr, GetResourceString(CType(ErrNumber, vbErrors)))
        End Function

        ''' <summary>
        ''' Called by the compiler in response to falling into a catch block.
        ''' Inside the catch statement the compiler generates code to call:
        '''  ProjectData::SetProjectError(exception)  That call
        '''  in turns sets the ErrObject which is accessed via the VB Err statement.
        '''  So a VB6 programmer would typically then do something like:
        '''    if err.Number = * do something where err accesses the ErrObject that
        '''  is set by this method.
        ''' </summary>
        ''' <param name="ex"></param>
        ''' <remarks></remarks>
        <SecuritySafeCritical()> _
        <ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)> _
        Public Overloads Shared Sub SetProjectError(ByVal ex As Exception)
            ' The Try/Finally and constrained regions calls guarantee success under high
            ' stress conditions by enabling eager jitting of the finally block
            System.Runtime.CompilerServices.RuntimeHelpers.PrepareConstrainedRegions()
            Try
            Finally
                Err.CaptureException(ex)
            End Try
        End Sub

        ''' <summary>
        ''' Called by the compiler in response to falling into a catch block.
        ''' Inside the catch statement the compiler generates code to call:
        '''  ProjectData::SetProjectError(exception, lineNumber)  This call
        ''' differs from SetProjectError(ex as Exception)because it is called
        ''' when the exception is thrown from a specific line number, e.g:
        '''   123:  Throw new Exception
        '''   123:  Error x80004003
        '''  This method in turn sets the ErrObject which is accessed via the
        '''  VB "Err" statement.
        '''  So a VB6 programmer could then do something like:
        '''    if err.Number = * 
        '''    err.Erl will also be set
        '''  is set by this class.
        ''' </summary>
        ''' <param name="ex"></param>
        ''' <param name="lErl"></param>
        ''' <remarks></remarks>
        <SecuritySafeCritical()> _
        <ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)> _
        Public Overloads Shared Sub SetProjectError(ByVal ex As Exception, ByVal lErl As Integer)
            ' The Try/Finally and constrained regions calls guarantee success under high
            ' stress conditions by enabling eager jitting of the finally block
            System.Runtime.CompilerServices.RuntimeHelpers.PrepareConstrainedRegions()
            Try
            Finally
                Err.CaptureException(ex, lErl)
            End Try
        End Sub

        <SecuritySafeCritical()> _
        <ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)> _
        Public Shared Sub ClearProjectError()
            ' The Try/Finally and constrained regions calls guarantee success under high
            ' stress conditions by enabling eager jitting of the finally block
            System.Runtime.CompilerServices.RuntimeHelpers.PrepareConstrainedRegions()
            Try
            Finally
                Err.Clear()
            End Try
        End Sub

        <SecuritySafeCritical()> _
        <HostProtection(Resources:=HostProtectionResource.SelfAffectingProcessMgmt)> _
        <SecurityPermissionAttribute(SecurityAction.Demand, Flags:=SecurityPermissionFlag.UnmanagedCode)> _
        <System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining)> _
        Public Shared Sub EndApp()
            FileSystem.CloseAllFiles(System.Reflection.Assembly.GetCallingAssembly())
            System.Environment.Exit(0) 'System.Environment.Exit will cause finalizers to be run at shutdown
        End Sub
    End Class
End Namespace
