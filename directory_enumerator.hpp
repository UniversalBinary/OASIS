#include <boost/filesystem.hpp>
#include <cerrno>
#if defined(__GNUC__) || defined(__clang__) || defined(__MINGW32__) || defined(__MINGW64__)
#include <dirent.h>
#elif defined(_MSC_VER)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 
#endif
#include <windows.h>
#include "win32_error.hpp"
#endif
#include "foundation.hpp"

#ifndef _DIRECTORY_ENUMERATOR_HPP_
#define _DIRECTORY_ENUMERATOR_HPP_

namespace oasis::filesystem
{
    /**********************************************************************************************//**
     * @class	directory_enumerator directory_enumerator.hpp
     *
     * @brief	Provides a basic enumerator for a directory, each entry will be returned as a path
     * 			object, with no further checking carried out as to whether the entry refers to a file,
     * 			a directory or a symbolic link. It is the responsibility of the caller to determine if
     * 			the returned path object refers to an existing file or directory.
     * 			
     * 			This class does not search recursively and will return all files and directories
     * 			found, with the exception of the special entries '.' and '..'
     **************************************************************************************************/

    class directory_enumerator
    {
    private:
        boost::filesystem::path _search_dir;
        bool _opened;
        boost::filesystem::path _current;
        bool _ended;
#if defined(__GNUC__) || defined(__clang__) || defined(__MINGW32__) || defined(__MINGW64__)
        DIR* _pdir;
#elif defined(_MSC_VER)
        WIN32_FIND_DATA _fdFileData;
        HANDLE _hFind;
#endif
        
    public:

        /**********************************************************************************************//**
         * @fn	explicit directory_enumerator::directory_enumerator(const boost::filesystem::path& p)
         *
         * @brief	Creates a new instance of the directory_enumerator class with the given search path.
         *
         * @exception	std::invalid_argument	Thrown if the given boost::filesystem::path object is
         * 										empty.
         * @exception	std::system_error	 	Thrown if a system error condition occurs while checking
         * 										if the given boost::filesystem::path object exists and is
         * 										a directory.
         *
         * @param 	p	A boost::filesystem::path for the directory to be enumerated, this can be a
         * 				relative path or a symlink to a directory.
         **************************************************************************************************/

        explicit directory_enumerator(const boost::filesystem::path& p)
        {
            if (p.empty()) throw std::invalid_argument("Invalid search path");
            _search_dir = boost::filesystem::canonical(p);
            if (!boost::filesystem::exists(_search_dir)) throw std::system_error(EEXIST, std::generic_category());
            if (!boost::filesystem::is_directory(_search_dir)) throw std::invalid_argument("Search path is not a directory");
            _opened = false;
            _ended = true;
#if defined(__GNUC__) || defined(__clang__) || defined(__MINGW32__) || defined(__MINGW64__)
            _pdir = nullptr;
#elif defined(_MSC_VER)
            _hFind = INVALID_HANDLE_VALUE;
#endif
            
        }

        ~directory_enumerator()
        {
#if defined(__GNUC__) || defined(__clang__) || defined(__MINGW32__) || defined(__MINGW64__)
            if (_pdir != nullptr) closedir(_pdir);
#elif defined(_MSC_VER)
            if (_hFind != INVALID_HANDLE_VALUE) FindClose(_hFind);
#endif
            
        }

        /**********************************************************************************************//**
         * Attempts to move this enumerator to the next directory entry, returning true if it succeeds.
         *
         * @param [in,out]	ec	An out-parameter for error reporting.
         *
         * @returns	true if the enumerator was successfully moved to the next entry in the directory or
         * 			false if not; if the position of the enumerator was not advanced due to an error, the
         * 			ec parameter should contain more information.
         *
         * ### remarks	If a temporary, recoverable error, such as low system resources or a device or
         * 				resource is busy, this function will block.
         **************************************************************************************************/

        bool move_next(boost::system::error_code& ec) noexcept;

        /**********************************************************************************************//**
         * @fn	boost::filesystem::path directory_enumerator::current()
         *
         * @brief	Gets the directory entry at the current position of this enumerator.
         *
         * @exception	std::system_error	Thrown if a call to this method has not been preceded by a
         * 									successful call to \link move_next()\endlink.
         *
         * @returns	A boost::filesystem::path object representing the directory entry at the current
         * 			position of this enumerator.
         **************************************************************************************************/

        boost::filesystem::path current()
        {
            if (!_opened || _ended) throw std::system_error(EINVAL, std::generic_category());
            return _current;
        }
    };

#if defined(__GNUC__) || defined(__clang__) || defined(__MINGW32__) || defined(__MINGW64__)

    bool directory_enumerator::move_next(boost::system::error_code& ec)
    {
        ec.clear();

        if (!_opened)
        {
            for (;;)
            {
                _pdir = opendir(_search_dir.string().c_str());
                if (_pdir != nullptr) break;
                if ((errno == ENFILE) || (errno == EMFILE) || (errno == ENOSR) || (errno == EAGAIN) || (errno == ENOMEM))
                {
                    boost::this_thread::sleep_for(boost::chrono::seconds(5));
                    continue;
                }
                else
                {
                    ec = boost::system::error_code(errno, boost::system::generic_category();
                    return false;
                }
            }
            _opened = true;
        }

        char *name = nullptr;
        for (;;)
        {
            errno = 0;
            auto de = readdir(_pdir);
            if (de == nullptr)
            {
                ec = boost::system::error_code(errno, boost::system::generic_category();
                _ended = true;
                closedir(_pdir);
                _pdir = nullptr;
                return false;
            }
            name = de->d_name;
            if (std::strcmp(name, ".") == 0) continue;
            if (std::strcmp(name, "..") == 0) continue;
            break;
        }

        boost::filesystem::path out(name);
        _current = _search_dir / out;
        _ended = false;

        return true;
    }

#elif defined(_MSC_VER)

    bool directory_enumerator::move_next(boost::system::error_code& ec) noexcept
    {
        ec.clear();
        DWORD error = 0;

        if (!_opened)
        {
            boost::filesystem::path sp = _search_dir / "*";
            for (;;)
            {
                _hFind = FindFirstFile(sp.wstring().c_str(), &_fdFileData);
                if (_hFind == INVALID_HANDLE_VALUE)
                {
                    error = GetLastError();
                    if (error == ERROR_FILE_NOT_FOUND) return false;
                    if ((error == ERROR_TOO_MANY_OPEN_FILES) || (error == ERROR_NOT_ENOUGH_MEMORY) || (error == ERROR_OUTOFMEMORY) || (error == ERROR_NOT_READY) || (error == ERROR_SHARING_VIOLATION) || (error == ERROR_LOCK_VIOLATION) || (error == ERROR_BUSY) || (error == ERROR_NETWORK_BUSY) || (error == ERROR_PATH_BUSY)) continue;
                    ec = make_win32_error_code(error);
                    return false;
                }
                else
                {
                    _opened = true;
                    _ended = false;
                    boost::filesystem::path out(_fdFileData.cFileName);
                    if ((out.wstring() != L".") && (out.wstring() != L"..")) // Unlikely, but all eventualities must be accounted for.
                    {
                        _current = _search_dir / out;
                        return true;
                    }
                    break;
                }
            }
        }

        for (;;)
        {
            if (FindNextFile(_hFind, &_fdFileData))
            {
                boost::filesystem::path out(_fdFileData.cFileName);
                if ((out.wstring() == L".") || (out.wstring() == L"..")) continue;
                _current = _search_dir / out;
                _ended = false;
                return true;
            }
            _ended = true;
            error = GetLastError();
            if (error == ERROR_NO_MORE_FILES) return false;
            if ((error == ERROR_TOO_MANY_OPEN_FILES) || (error == ERROR_NOT_ENOUGH_MEMORY) || (error == ERROR_OUTOFMEMORY) || (error == ERROR_NOT_READY) || (error == ERROR_SHARING_VIOLATION) || (error == ERROR_LOCK_VIOLATION) || (error == ERROR_BUSY) || (error == ERROR_NETWORK_BUSY) || (error == ERROR_PATH_BUSY)) continue;
            ec = make_win32_error_code(error);
            return false;
        }
    }

#endif
}

#endif //_DIRECTORY_ENUMERATOR_HPP_
