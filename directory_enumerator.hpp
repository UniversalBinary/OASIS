#include <boost/filesystem.hpp>
#include <dirent.h>
#include <cerrno>

#ifndef _DIRECTORY_ENUMERATOR_HPP_
#define _DIRECTORY_ENUMERATOR_HPP_

namespace oasis::filesystem
{
    class directory_enumerator
    {
    private:
        boost::filesystem::path _search_dir;
        bool _opened;
        boost::filesystem::path _current;
        bool _ended;
        DIR *_pdir;
    public:
        explicit directory_enumerator(const boost::filesystem::path& p)
        {
            if (p.empty()) throw std::invalid_argument("Invalid search path");
            _search_dir = boost::filesystem::canonical(p);
            if (!boost::filesystem::exists(_search_dir)) throw std::system_error(EEXIST, std::generic_category());
            if (!boost::filesystem::is_directory(_search_dir)) throw std::invalid_argument("Search path is not a directory");
            _opened = false;
            _ended = true;
            _pdir = nullptr;
        }

        ~directory_enumerator()
        {
            if (_pdir != nullptr) closedir(_pdir);
        }

        bool move_next(std::error_code& ec);

        boost::filesystem::path current()
        {
            if (!_opened || _ended) throw std::system_error(EINVAL, std::generic_category());
            return _current;
        }
    };

#if defined(__linux) || defined(__linux__) || defined(__gnu_linux__)

    bool directory_enumerator::move_next(std::error_code& ec)
    {
        if (ec)
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
                    ec = std::error_code(errno, boost::system::system_category());
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
                ec = std::error_code(errno, boost::system::system_category());
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
}
#endif

#endif //_DIRECTORY_ENUMERATOR_HPP_
