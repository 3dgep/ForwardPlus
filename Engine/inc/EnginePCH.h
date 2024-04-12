// Include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

// Stringize macro for numerical macro types.
// Inserts a macro as a string.
// Used by the STRINGIZE macro and should not be used directly.
#define STRINGIZE2(x) #x
// Expand the macro into a string.
#define STRINGIZE(x) STRINGIZE2(x)


#if defined(_MSC_VER) && defined(_WIN32_WINNT)
// Output some informational messages so we know what OS and compiler we're using.
#pragma message( "Microsoft Windows Version " STRINGIZE(_WIN32_WINNT) )
#pragma message( "Microsoft (R) C/C++ Optimizing Compiler Version " STRINGIZE(_MSC_VER) )
#endif

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <process.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#include <comdef.h>
#include <CommCtrl.h> // For windows controls (progress bar)
#include <mmsystem.h> // For joystick controls
#pragma comment(lib,"winmm.lib")

// Windows Runtime library (needed for Microsoft::WRL::ComPtr<> template class)
#include <wrl.h>

// DirectX 12 specific headers.
// DirectX 12 headers and libraries are included in Windows 10 SDK
#if defined(_WIN32_WINNT_WIN10) 
#   include <d3d12.h>
#   include <dxgi1_4.h>
#	include <d3d12sdklayers.h>
#	include <d3d12shader.h>
#	pragma comment(lib, "d3d12.lib")
#else
#   include <dxgi1_3.h>
#endif

#include <d3d11_2.h> // <d3d11_3.h> is only supported on Windows 10 (but then I'd rather use DX12!?)
#include <d3dcompiler.h>
#pragma warning( disable: 4838 )
#include <DirectXMath.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

// STL
#include <locale>
#include <codecvt>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <vector>
#include <map>
#include <ctime>
#include <algorithm>
#include <random>
#include <atomic>
#include <mutex>
#include <thread>

// BOOST
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <boost/math/special_functions/round.hpp>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
namespace fs = boost::filesystem;

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/signals2.hpp>
#include <boost/any.hpp>

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>
#include <boost/archive/xml_woarchive.hpp>
#include <boost/serialization/vector.hpp>

// Report an error to the Debug output in Visual Studio, display a message box with the error message and throw an exception.
inline void ReportErrorAndThrow( const std::string& file, int line, const std::string& function, const std::string& message)
{
    std::stringstream ss;
    
    DWORD errorCode = GetLastError();
    LPTSTR errorMessage = nullptr;

    FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errorCode, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), errorMessage, 0, nullptr );

    if ( errorMessage )
    {
        ss << file << "(" << line << "): " << "error " << errorCode << ": " << errorMessage << std::endl;
        LocalFree( errorMessage );
    }
    else
    {
        ss << file << "(" << line << "): " << message << std::endl;
    }

    OutputDebugStringA( ss.str().c_str() );
	MessageBoxA(nullptr, message.c_str(), function.c_str(), MB_ICONERROR);
	throw new std::exception(message.c_str());
}

// Report an error message and throw an std::exception.
#define ReportError( msg ) ReportErrorAndThrow( __FILE__, __LINE__, __FUNCTION__, (msg) )

template<typename T>
inline void SafeDelete( T& ptr )
{
	if ( ptr != NULL )
	{
		delete ptr;
		ptr = NULL;
	}
}

template<typename T>
inline void SafeDeleteArray( T& ptr )
{
	if ( ptr != NULL )
	{
		delete [] ptr;
		ptr = NULL;
	}
}

// Convert a multi-byte character string (UTF-8) to a wide (UTF-16) encoded string.
inline std::wstring ConvertString( const std::string& string )
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes( string );
}

// Converts a wide (UTF-16) encoded string into a multi-byte (UTF-8) character string.
inline std::string ConvertString( const std::wstring& wstring )
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes( wstring );
}

// Gets a string resource from the module's resources.
inline std::string GetStringResource( int ID, const std::string& type )
{
    HMODULE hModule = GetModuleHandle( nullptr );
    HRSRC hResource = FindResourceA( hModule, MAKEINTRESOURCE( ID ), type.c_str() );
    if ( hResource )
    {
        HGLOBAL hResourceData = LoadResource( hModule, hResource );
        DWORD resourceSize = SizeofResource( hModule, hResource );
        if ( hResourceData && resourceSize > 0 )
        {
            const char* resourceData = static_cast<const char*>( LockResource( hResourceData ) );
            std::string strData( resourceData, resourceSize );
            return strData;
        }
    }
    // Just return an empty string.
    return std::string();
}

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/matrix_operation.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>

// AntTweakBar
#include <AntTweakBar.h>

#if defined(_WIN64)
#pragma comment(lib, "AntTweakBar64.lib")
#else
#pragma comment(lib, "AntTweakBar.lib")
#endif
// Don't forget to include the DLL

// Assimp
#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/ProgressHandler.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>

#pragma comment(lib, "assimp.lib")

// FreeImage
#include <FreeImage.h>

#pragma comment(lib, "FreeImage.lib")
