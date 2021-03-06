#include "Factory.h"
#include <float.h>
#include "DWriteInterfaces.h"

using namespace System::Runtime::InteropServices;
using namespace MS::Internal::Text::TextInterface::Interfaces;
using namespace System::Security::Permissions;
using namespace System::Threading;

typedef HRESULT (WINAPI *DWRITECREATEFACTORY)(DWRITE_FACTORY_TYPE factoryType, REFIID iid, IUnknown **factory);

/// <SecurityNote>
/// Critical - Returns a pointer to the DWriteCreateFactory method which
///            can be used to access the shared factory.
/// </SecurityNote>
[System::Security::SecurityCritical]
extern void *GetDWriteCreateFactoryFunctionPointer();

namespace MS { namespace Internal { namespace Text { namespace TextInterface
{
    /// <summary>
    /// static ctor to initialize the GUID of IDWriteFactory interface.
    /// </summary>
    /// <SecurityNote>
    /// Critical - Asserts unmanaged code permissions.
    ///          - Assigns security critical _guidForIDWriteFactory
    /// Safe     - The data used to initialize _guidForIDWriteFactory is const.
    /// </SecurityNote>
    [SecuritySafeCritical]
    [SecurityPermission(SecurityAction::Assert, UnmanagedCode=true)]
    static Factory::Factory()
    {
        System::Guid guid = System::Guid("b859ee5a-d838-4b5b-a2e8-1adc7d93db48");
        _GUID* pGuidForIDWriteFactory = new _GUID();
        *pGuidForIDWriteFactory = Util::ToGUID(guid);
        _guidForIDWriteFactory = gcnew NativePointerWrapper<_GUID>(pGuidForIDWriteFactory);  
    }

    /// <SecurityNote>
    /// Critical - Calls security critical Factory ctor().
    /// </SecurityNote>
    [SecurityCritical]
    Factory^ Factory::Create(
                            FactoryType                   factoryType,
                            IFontSourceCollectionFactory^ fontSourceCollectionFactory,
                            IFontSourceFactory^           fontSourceFactory
                            )
    {
        return gcnew Factory(factoryType, fontSourceCollectionFactory, fontSourceFactory);
    }

    /// <SecurityNote>
    /// Critical - references security critical member '_pFactory'.
    ///            references security critical method 'MS.Internal.Text.TextInterface.FontFileLoader..ctor(MS.Internal.Text.TextInterface.IFontSourceFactory)'.
    ///            references security critical method 'MS.Internal.Text.TextInterface.FontCollectionLoader..ctor(MS.Internal.Text.TextInterface.IFontSourceCollectionFactory, MS.Internal.Text.TextInterface.FontFileLoader)'.
    ///            references security critical method 'System.Runtime.InteropServices.Marshal.GetComInterfaceForObject(System.Object, System.Type)' & 
    ///            'System.Runtime.InteropServices.Marshal.Release(System.IntPtr)' but this is ok since they are called for objects that this method create.
    ///            Asserts unmanaged code permissions to call Marshal.*
    /// </SecurityNote>
    //[SecurityCritical] - tagged in header file
    //[SecurityPermission(SecurityAction::Assert, UnmanagedCode=true)]
    Factory::Factory(
                    FactoryType                   factoryType,
                    IFontSourceCollectionFactory^ fontSourceCollectionFactory,
                    IFontSourceFactory^           fontSourceFactory
                    ) : CriticalHandle(IntPtr::Zero)
    {
        Initialize(factoryType);
        _wpfFontFileLoader       = gcnew FontFileLoader(fontSourceFactory);
        _wpfFontCollectionLoader = gcnew FontCollectionLoader(
                                                           fontSourceCollectionFactory,
                                                           _wpfFontFileLoader
                                                           );

        _fontSourceFactory = fontSourceFactory;

        IntPtr pIDWriteFontFileLoaderMirror = Marshal::GetComInterfaceForObject(
                                                _wpfFontFileLoader,
                                                IDWriteFontFileLoaderMirror::typeid);
        
        // 



        HRESULT hr = _pFactory->RegisterFontFileLoader(
                                    (IDWriteFontFileLoader *)pIDWriteFontFileLoaderMirror.ToPointer()
                                    );

        Marshal::Release(pIDWriteFontFileLoaderMirror);

        ConvertHresultToException(hr, "Factory::Factory");

        IntPtr pIDWriteFontCollectionLoaderMirror = Marshal::GetComInterfaceForObject(
                                                _wpfFontCollectionLoader,
                                                IDWriteFontCollectionLoaderMirror::typeid);
        hr = _pFactory->RegisterFontCollectionLoader(
                                                    (IDWriteFontCollectionLoader *)pIDWriteFontCollectionLoaderMirror.ToPointer()
                                                    );

        Marshal::Release(pIDWriteFontCollectionLoaderMirror);
        
        ConvertHresultToException(hr, "Factory::Factory");
    }

    /// <SecurityNote>
    /// Critical - Calls security critical GetDWriteCreateFactoryFunctionPointer().
    ///            Assigns security critical member _pFactory.
    /// Safe     - Does not expose any critical info.
    /// </SecurityNote>
    [SecuritySafeCritical]
    __declspec(noinline) void Factory::Initialize(
                                                 FactoryType factoryType
                                                 )
    {
        IUnknown* factoryTemp;
        DWRITECREATEFACTORY pfnDWriteCreateFactory = (DWRITECREATEFACTORY)GetDWriteCreateFactoryFunctionPointer();

        HRESULT hr = (*pfnDWriteCreateFactory)(
            DWriteTypeConverter::Convert(factoryType),
            (REFIID)(*(_guidForIDWriteFactory->Value)),
            &factoryTemp
            );
        
        ConvertHresultToException(hr, "Factory::Initialize");

        _pFactory = (IDWriteFactory*)factoryTemp;
    }

    /// <SecurityNote>
    /// Critical - Manipulates security critical member _pFactory.
    ///          - Asserts Unmanaged code permissions to call Marshal.*
    /// Safe     - Just releases the interface.
    ///          - Marshal is called with trusted inputs.
    /// </SecurityNote>
    [SecuritySafeCritical]
    [ReliabilityContract(Consistency::WillNotCorruptState, Cer::Success)]
    [SecurityPermission(SecurityAction::Assert, UnmanagedCode=true)]
    __declspec(noinline) bool Factory::ReleaseHandle()
    {
        if (_wpfFontCollectionLoader != nullptr)
        {
            IntPtr pIDWriteFontCollectionLoaderMirror = Marshal::GetComInterfaceForObject(
                                                _wpfFontCollectionLoader,
                                                IDWriteFontCollectionLoaderMirror::typeid);
            
            _pFactory->UnregisterFontCollectionLoader((IDWriteFontCollectionLoader *)pIDWriteFontCollectionLoaderMirror.ToPointer());
            Marshal::Release(pIDWriteFontCollectionLoaderMirror);
            _wpfFontCollectionLoader = nullptr;
        }
        
        if (_wpfFontFileLoader != nullptr)
        {
            IntPtr pIDWriteFontFileLoaderMirror = Marshal::GetComInterfaceForObject(
                                                    _wpfFontFileLoader,
                                                    IDWriteFontFileLoaderMirror::typeid);
            
            _pFactory->UnregisterFontFileLoader((IDWriteFontFileLoader *)pIDWriteFontFileLoaderMirror.ToPointer());
            Marshal::Release(pIDWriteFontFileLoaderMirror);            
            _wpfFontFileLoader = nullptr;
        }

        if (_pFactory != NULL)
        {
            _pFactory ->Release();
            _pFactory  = NULL;
        }

        return true;        
    }

    /// <SecurityNote>
    /// Critical - Assumes that the user has permissions to access filePathUri.
    /// </SecurityNote>
    [SecurityCritical]
    __declspec(noinline) FontFile^ Factory::CreateFontFile(
                                     System::Uri^ filePathUri
                                     )
    {        
        IDWriteFontFile* dwriteFontFile = NULL;
        HRESULT hr = Factory::CreateFontFile(_pFactory, _wpfFontFileLoader, filePathUri, &dwriteFontFile);

        // If DWrite's CreateFontFileReference fails then try opening the file using WPF's logic.
        // The failures that WPF returns are more granular than the HRESULTs that DWrite returns
        // thus we use WPF's logic to open the file to throw the same exceptions that
        // WPF would have thrown before.
        if (FAILED(hr))
        {
            IFontSource^ fontSource = _fontSourceFactory->Create(filePathUri->AbsoluteUri);
            fontSource->TestFileOpenable();

        }

        //This call is made to prevent this object from being collected and hence get its finalize method called 
        //While there are others referencing it.
        System::GC::KeepAlive(this);

        ConvertHresultToException(hr, "FontFile^ Factory::CreateFontFile");

        return gcnew FontFile(dwriteFontFile);

    }

    /// <SecurityNote>
    /// Critical - Calls security critical CreateFontFace.
    /// </SecurityNote>
    [SecurityCritical]
    FontFace^ Factory::CreateFontFace(
                                     System::Uri^ filePathUri,
                                     unsigned int faceIndex
                                     )
    {        
        return CreateFontFace(
                             filePathUri,
                             faceIndex,
                             FontSimulations::None
                             );
    }

    /// <SecurityNote>
    /// Critical - Calls security critical CreateFontFile.
    /// </SecurityNote>
    [SecurityCritical]
    FontFace^ Factory::CreateFontFace(
                                     System::Uri^    filePathUri,
                                     unsigned int    faceIndex,
                                     FontSimulations fontSimulationFlags
                                     )
    {
        FontFile^ fontFile = CreateFontFile(filePathUri);
        DWRITE_FONT_FILE_TYPE dwriteFontFileType;
        DWRITE_FONT_FACE_TYPE dwriteFontFaceType;
        unsigned int numberOfFaces = 0;

        HRESULT hr;
        if (fontFile->Analyze(
                             dwriteFontFileType,
                             dwriteFontFaceType,
                             numberOfFaces,
                             hr
                             ))
        {
            if (faceIndex >= numberOfFaces)
            {
                throw gcnew System::ArgumentOutOfRangeException("faceIndex");
            }

            unsigned char dwriteFontSimulationsFlags = DWriteTypeConverter::Convert(fontSimulationFlags);
            IDWriteFontFace* dwriteFontFace = NULL;
            IDWriteFontFile* dwriteFontFile = fontFile->DWriteFontFileNoAddRef;
            
            HRESULT hr = _pFactory->CreateFontFace(
                                                 dwriteFontFaceType,
                                                 1,
                                                 &dwriteFontFile,
                                                 faceIndex,
                                                 (DWRITE_FONT_SIMULATIONS) dwriteFontSimulationsFlags,
                                                 &dwriteFontFace
                                                 );
            System::GC::KeepAlive(fontFile);
            System::GC::KeepAlive(this);

            ConvertHresultToException(hr, "FontFace^ Factory::CreateFontFace");

            return gcnew FontFace(dwriteFontFace);
        }
        
        // This path is here because there is a behavior mismatch between DWrite and WPF.
        // If a directory was given instead of a font uri WPF previously throws 
        // System.UnauthorizedAccessException. We handle most of the exception behavior mismatch 
        // in FontFile^ Factory::CreateFontFile by opening the file using WPF's previous (prior to DWrite integration) logic if 
        // CreateFontFileReference fails (please see comments in FontFile^ Factory::CreateFontFile).
        // However in this special case DWrite's CreateFontFileReference will succeed if given
        // a directory instead of a font file and it is the Analyze() call will fail returning DWRITE_E_FILEFORMAT.
        // Thus, incase the hr returned from Analyze() was DWRITE_E_FILEFORMAT we do as we did in FontFile^ Factory::CreateFontFile
        // to try and open the file using WPF old logic and throw System.UnauthorizedAccessException as WPF used to do.
        // If a file format exception is expected then opening the file should succeed and ConvertHresultToException()
        // Should throw the correct exception.
        // A final note would be that this overhead is only incurred in error conditions and so the normal execution path should
        // not be affected.
        else
        {
            if (hr == DWRITE_E_FILEFORMAT)
            {
                IFontSource^ fontSource = _fontSourceFactory->Create(filePathUri->AbsoluteUri);
                fontSource->TestFileOpenable();
            }
            ConvertHresultToException(hr, "FontFace^ Factory::CreateFontFace");
        }

        return nullptr;
    }

    FontCollection^ Factory::GetSystemFontCollection()
    {
        return GetSystemFontCollection(false);
    }

    /// <SecurityNote>
    /// Critical - Uses security critical _pFactory pointer.
    /// Safe     - It does not expose the pointer it uses.
    /// </SecurityNote>
    [SecuritySafeCritical]
    __declspec(noinline) FontCollection^ Factory::GetSystemFontCollection(
                                                    bool checkForUpdates
                                                    )
    {
        IDWriteFontCollection* dwriteFontCollection = NULL;
        HRESULT hr                                  = _pFactory->GetSystemFontCollection(
                                                                                       &dwriteFontCollection,
                                                                                       checkForUpdates
                                                                                       );
        System::GC::KeepAlive(this);

        ConvertHresultToException(hr, "FontCollection^ Factory::GetSystemFontCollection");

        return gcnew FontCollection(dwriteFontCollection);
    }

    /// <SecurityNote>
    /// Critical - The caller of this method should own the verification of 
    ///            the access permissions to the given Uri.
    ///
    ///          Other reasons why this method should be critical (but safe)
    ///          ----------------------------------------------------------
    ///          - Uses security critical _pFactory pointer. But
    ///            It does not expose the pointer it uses.
    ///          - Asserts Unmanaged code permissions to call Marshal.* But
    ///            Marshal is called with trusted inputs.
    /// </SecurityNote>
    [SecurityCritical]
    [SecurityPermission(SecurityAction::Assert, UnmanagedCode=true)]
    __declspec(noinline) FontCollection^ Factory::GetFontCollection(System::Uri^ uri)
    {
        System::String^ uriString = uri->AbsoluteUri;
        IDWriteFontCollection* dwriteFontCollection = NULL;
        pin_ptr<const WCHAR> uriPathWChar           = CriticalPtrToStringChars(uriString);

        IntPtr pIDWriteFontCollectionLoaderMirror = Marshal::GetComInterfaceForObject(
                                                _wpfFontCollectionLoader,
                                                IDWriteFontCollectionLoaderMirror::typeid);

        IDWriteFontCollectionLoader * pIDWriteFontCollectionLoader = 
            (IDWriteFontCollectionLoader *)pIDWriteFontCollectionLoaderMirror.ToPointer();
                        
        HRESULT hr = _pFactory->CreateCustomFontCollection(
                           pIDWriteFontCollectionLoader,
                           uriPathWChar,
                           (uriString->Length+1) * sizeof(WCHAR),
                           &dwriteFontCollection
                           );

        Marshal::Release(pIDWriteFontCollectionLoaderMirror);
        
        System::GC::KeepAlive(this);

        ConvertHresultToException(hr, "FontCollection^ Factory::GetFontCollection");

        return gcnew FontCollection(dwriteFontCollection);
    }  

    /// <SecurityNote>
    /// Critical - Receives and returns native pointers.
    ///          - References security critical method 'System.Runtime.InteropServices.Marshal.GetComInterfaceForObject(System.Object, System.Type)'.
    ///          - References security critical method 'System.Runtime.InteropServices.Marshal.Release(System.IntPtr)'.
    ///          - Asserts unmanaged code permissions to call Marshal.* However the call to Marshal is safe
    ///            because it is called with trusted inputs.
    /// </SecurityNote>
    [SecurityCritical]
    [SecurityPermission(SecurityAction::Assert, UnmanagedCode=true)]
    HRESULT Factory::CreateFontFile(
                                   IDWriteFactory*         factory,
                                   FontFileLoader^         fontFileLoader,
                                   System::Uri^            filePathUri,
                                   __out IDWriteFontFile** dwriteFontFile
                                   )
    {
        bool isLocal = Factory::IsLocalUri(filePathUri);
        HRESULT hr = E_FAIL;

        if (isLocal)
        {
            //Protect the pointer to the filepath from being moved around by the garbage collector.
            pin_ptr<const WCHAR> uriPathWChar = CriticalPtrToStringChars(filePathUri->LocalPath);

            // DWrite currently has a slow lookup for the last write time, which
            // introduced a noticable perf regression when we switched over.
            // To mitigate this scenario, we will fetch the timestamp ourselves
            // and cache it for future calls.
            //
            // Note: we only do this if a Dispatcher exists for the current
            // thread.  There is a seperate cache for each thread.
            ::FILETIME timeStamp;
            ::FILETIME * pTimeStamp = NULL; // If something fails, do nothing and let DWrite sort it out.
            System::Runtime::InteropServices::ComTypes::FILETIME cachedTimeStamp;
            Dispatcher^ currentDispatcher = Dispatcher::FromThread(Thread::CurrentThread);
            if(currentDispatcher != nullptr)
            {
                // One-time initialization per thread.
                if(_timeStampCache == nullptr)
                {
                    _timeStampCache = gcnew Dictionary<System::Uri^,System::Runtime::InteropServices::ComTypes::FILETIME>();                
                }
                
                if(_timeStampCache->TryGetValue(filePathUri, cachedTimeStamp))
                {
                    // Convert managed FILETIME to native FILETIME, pass to DWrite
                    timeStamp.dwLowDateTime = cachedTimeStamp.dwLowDateTime;
                    timeStamp.dwHighDateTime = cachedTimeStamp.dwHighDateTime;
                    pTimeStamp = &timeStamp;
                }
                else
                {
                    // Nothing was in the cache for this font URI, so try to open
                    // the file to fetch the timestamp.  We open the file, rather
                    // than calling APIs like GetFileAttributesEx, so that symbolic
                    // links will resolve and also to ensure the timestamp is
                    // accurate.
                    //
                    // As of 10/7/2011 these flags we the same as DWrite uses.
                    HANDLE hFile = ::CreateFile(
                        uriPathWChar,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_RANDOM_ACCESS, // hint to the OS that the file is accessed randomly
                        NULL);
                    if(hFile != INVALID_HANDLE_VALUE)
                    {
                        ::BY_HANDLE_FILE_INFORMATION fileInfo;
                        if(::GetFileInformationByHandle(hFile, &fileInfo))
                        {
                            // Convert native FILETIME to managed FILETIME and cache.
                            cachedTimeStamp.dwLowDateTime = fileInfo.ftLastWriteTime.dwLowDateTime;
                            cachedTimeStamp.dwHighDateTime = fileInfo.ftLastWriteTime.dwHighDateTime;
                            _timeStampCache->Add(filePathUri, cachedTimeStamp);
                            
                            // We don't want to hold this cached value for a long time since
                            // all font references will be tied to this timestamp, and any font
                            // update during the lifetime of the application will cause is to
                            // encounter errors.  So we use a dispatcher operation to clear
                            // the cache as soon as we get back to pumping messages.
                            if(_timeStampCacheCleanupOp == nullptr)
                            {
                                _timeStampCacheCleanupOp = currentDispatcher->BeginInvoke(gcnew Action(CleanupTimeStampCache));
                            }
                            
                            // Pass this write time to DWrite
                            timeStamp = fileInfo.ftLastWriteTime;
                            pTimeStamp = &timeStamp;
                        }

                        CloseHandle(hFile);
                        hFile = NULL;
                    }
                }
            }

            hr = factory->CreateFontFileReference(
                                                  uriPathWChar,
                                                  pTimeStamp,
                                                  dwriteFontFile
                                                  );
        }
        else
        {
            System::String^ filePath = filePathUri->AbsoluteUri;
            //Protect the pointer to the filepath from being moved around by the garbage collector.
            pin_ptr<const WCHAR> uriPathWChar = CriticalPtrToStringChars(filePath);

            IntPtr pIDWriteFontFileLoaderMirror = Marshal::GetComInterfaceForObject(
                                                    fontFileLoader,
                                                    IDWriteFontFileLoaderMirror::typeid);
            
            hr = factory->CreateCustomFontFileReference(
                                                        uriPathWChar,
                                                        (filePath->Length + 1) * sizeof(WCHAR),
                                                        (IDWriteFontFileLoader *)pIDWriteFontFileLoaderMirror.ToPointer(),
                                                        dwriteFontFile
                                                        );

            Marshal::Release(pIDWriteFontFileLoaderMirror);
        }
        return hr;
    }

    __declspec(noinline) void Factory::CleanupTimeStampCache()
    {
        _timeStampCacheCleanupOp = nullptr;
        _timeStampCache->Clear();
    }

    /// <SecurityNote>
    /// Critical - Uses security critical _pFactory pointer.
    ///          - Calls security critical TextAnalyzer ctor()
    /// Safe     - It does not expose the pointer it uses.
    ///          - TextAnalyzer ctor() is called with a trusted pointer.
    /// </SecurityNote>
    [SecuritySafeCritical]
    __declspec(noinline) TextAnalyzer^ Factory::CreateTextAnalyzer()
    {
        IDWriteTextAnalyzer* textAnalyzer = NULL;
        HRESULT hr = _pFactory->CreateTextAnalyzer(&textAnalyzer);
        System::GC::KeepAlive(this);
        ConvertHresultToException(hr, "TextAnalyzer^ Factory::CreateTextAnalyzer");
        return gcnew TextAnalyzer(textAnalyzer);
    }

    bool Factory::IsInvalid::get()
    {
        return (_pFactory == NULL);
    }
}}}}//MS::Internal::Text::TextInterface
