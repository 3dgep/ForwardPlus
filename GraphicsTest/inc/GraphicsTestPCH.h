#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <shellapi.h>

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
#include <functional>


// BOOST
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
namespace fs = boost::filesystem;

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/signals2.hpp>
#include <boost/any.hpp>

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/vector.hpp>

// GLM
#define GLM_SWIZZLE
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

// Report an error to the Debug output in Visual Studio, display a message box with the error message and throw an exception.
inline void ReportErrorAndThrow( const std::string& file, int line, const std::string& function, const std::string& message )
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
    MessageBox( nullptr, message.c_str(), function.c_str(), MB_ICONERROR );
#if defined(_DEBUG)
    throw new std::exception( message.c_str() );
#endif
}

// Report an error message and throw an std::exception.
#define ReportError( msg ) ReportErrorAndThrow( __FILE__, __LINE__, __FUNCTION__, (msg) )
