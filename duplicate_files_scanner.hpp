#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include <map>
#include <forward_list>
#include <tuple>
#include <execution>
#include <memory>
#include <set>
#include <boost/thread.hpp>
#include <mutex>
#include <iterator>
#include <botan/skein_512.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include "base.hpp"
#include "directory_enumerator.hpp"

#ifndef _DUPLICATE_SCANNER_LIST_HPP_
#define _DUPLICATE_SCANNER_LIST_HPP_

namespace oasis::filesystem
{
    template<typename SorterT = sort_by_filename>
    class duplicate_files_scanner : public directory_scanner
    {
    private:
        bool _remove_single;
        std::mutex _list_lock;
        uintmax_t _file_count;
        uintmax_t _space_occupied;
        uintmax_t _sets_found;
        boost::thread_group _threads;
        std::function<void(const boost::filesystem::path&)> _scan_started_callback;
        std::function<void(const boost::filesystem::path&, uintmax_t, uintmax_t)> _scan_progress_callback;
        std::function<void(const boost::filesystem::path&, uintmax_t, uintmax_t, uintmax_t, uintmax_t)> _scan_completed_callback;
        std::function<void(const boost::filesystem::path&, const boost::filesystem::path&, const std::error_condition&)> _scan_error_callback;
        using set_t = std::set<boost::filesystem::path, SorterT>;
        using map_t = std::map<std::tuple<uintmax_t, std::string>, set_t>;
        map_t _sets;

        friend class unique_files_scanner;

        bool _get_hash_of_file(const boost::filesystem::path& p, std::tuple<std::uintmax_t, std::string>& out, boost::system::error_code& ec);
        void _process_file(const boost::filesystem::path& dirent, bool recurse);

    public:
        typedef typename map_t ::size_type size_type;
        typedef set_t value_type;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef typename map_t ::difference_type difference_type;
        typedef value_type * pointer;
        typedef const pointer const_pointer;

        template<typename IterT>
        class basic_iterator
        {
        private:
            IterT under;
        public:
            typedef std::bidirectional_iterator_tag iterator_category;
            typedef set_t value_type;
            typedef typename map_t::difference_type difference_type;
            typedef value_type& reference;
            typedef value_type * pointer;

            explicit basic_iterator(IterT x)
            {
                under = x;
            }

            reference operator*() const
            {
                return under->second;
            }

            pointer operator->()
            {
                return &under->second;
            }

            bool operator!=(const basic_iterator& o) const
            {
                return under != o.under;
            }

            basic_iterator& operator++()
            {
                ++under;
                return *this;
            }

            basic_iterator operator++(int)
            {
                basic_iterator x(*this);
                ++under;
                return x;
            }

            basic_iterator& operator--()
            {
                --under;
                return *this;
            }

            basic_iterator operator--(int)
            {
                basic_iterator x(*this);
                --under;
                return x;
            }
        };

        typedef basic_iterator<typename map_t::iterator> iterator;
        typedef basic_iterator<typename map_t::const_iterator> const_iterator;
        typedef basic_iterator<typename map_t::reverse_iterator> reverse_iterator;
        typedef basic_iterator<typename map_t::const_reverse_iterator> const_reverse_iterator;

        iterator begin() noexcept
        {
            return iterator(_sets.begin());
        }

        [[nodiscard]] const_iterator begin() const noexcept
        {
            return const_iterator(_sets.begin());
        }

        [[nodiscard]] const_iterator cbegin() const noexcept
        {
            return const_iterator(_sets.cbegin());
        }

        [[nodiscard]] iterator end() noexcept
        {
            return iterator(_sets.end());
        }

        [[nodiscard]] const_iterator end() const noexcept
        {
            return const_iterator(_sets.end());
        }

        [[nodiscard]] const_iterator cend() const noexcept
        {
            return const_iterator(_sets.cend());
        }

        reverse_iterator rbegin() noexcept
        {
            return reverse_iterator(_sets.rbegin());
        }

        [[nodiscard]] const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator(_sets.rbegin());
        }

        [[nodiscard]] const_reverse_iterator crbegin() const noexcept
        {
            return const_reverse_iterator(_sets.crbegin());
        }

        reverse_iterator rend() noexcept
        {
            return reverse_iterator(_sets.rend());
        }

        [[nodiscard]] const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator(_sets.rend());
        }

        [[nodiscard]] const_reverse_iterator crend() const noexcept
        {
            return const_reverse_iterator(_sets.crend());
        }

        [[nodiscard]] bool empty() const noexcept override
        {
            return _sets.empty();
        }

        [[nodiscard]] std::size_t size() const noexcept override
        {
            return _sets.size();
        }

        void clear() noexcept override
        {
            _sets.clear();
        }

        void set_scan_started_callback(const std::function<void(const boost::filesystem::path&)>& callback)
        {
            _scan_started_callback = callback;
        }

        void set_scan_progress_callback(const std::function<void(const boost::filesystem::path&, uintmax_t, uintmax_t)>& callback)
        {
            _scan_progress_callback = callback;
        }

        void set_scan_completed_callback(const std::function<void(const boost::filesystem::path&, uintmax_t, uintmax_t, uintmax_t, uintmax_t)>& callback)
        {
            _scan_completed_callback = callback;
        }

        void set_scan_error_callback(const std::function<void(const boost::filesystem::path&, const boost::filesystem::path&, const std::error_condition&)>& callback)
        {
            _scan_error_callback = callback;
        }

        [[nodiscard]] uintmax_t set_count() const
        {
            return _sets.size();
        }

        [[nodiscard]] uintmax_t file_count() const
        {
            return _file_count;
        }

        [[nodiscard]] uintmax_t space_occupied() const
        {
            return _space_occupied;
        }

        duplicate_files_scanner& operator=(duplicate_files_scanner&& other) noexcept
        {
            _sets = std::move(other._sets);
            _follow_links = other._follow_links;
            _remove_single = other._remove_single;
            _files_encountered = other._files_encountered;
            _extensions = std::move(other._extensions);
            _file_count = other._file_count;
            _space_occupied = other._space_occupied;

            return *this;
        }

        duplicate_files_scanner& operator=(const duplicate_files_scanner& other)
        {
            _sets = other._sets;
            _follow_links = other._follow_links;
            _remove_single = other._remove_single;
            _files_encountered = other._files_encountered;
            _extensions = other._extensions;
            _file_count = other._file_count;
            _space_occupied = other._space_occupied;

            return *this;
        }

        explicit duplicate_files_scanner(const boost::filesystem::path& p) : directory_scanner(p)
        {
            _remove_single = true;
            _file_count = 0;
            _space_occupied = 0;
            _sets_found = 0;
        }

        duplicate_files_scanner(const duplicate_files_scanner& other) : directory_scanner(other)
        {
            _sets = other._sets;
            _follow_links = other._follow_links;
            _remove_single = other._remove_single;
            _files_encountered = other._files_encountered;
            _extensions = other._extensions;
            _file_count = other._file_count;
            _space_occupied = other._space_occupied;
            _sets_found = 0;
        }

        duplicate_files_scanner(duplicate_files_scanner&& other) noexcept
        {
            _sets = std::move(other._sets);
            _follow_links = other._follow_links;
            _remove_single = other._remove_single;
            _files_encountered = other._files_encountered;
            _extensions = std::move(other._extensions);
            _file_count = other._file_count;
            _space_occupied = other._space_occupied;
            _sets_found = 0;
        }

        void perform_scan(bool recursive) override;

    };

    template<typename SorterT>
    void duplicate_files_scanner<SorterT>::perform_scan(bool recursive)
    {
        if (_scan_started_callback) _scan_started_callback(_search_dir);

        std::error_code sec;
        directory_enumerator de(_search_dir);
        while (de.move_next(sec))
        {
            _process_file(de.current(), recursive);
        }
        if (sec && _scan_error_callback) _scan_error_callback(_search_dir, boost::filesystem::path(), sec.default_error_condition());

        // Work out the statistics.
        boost::system::error_code ec;
        std::set<std::tuple<uintmax_t, std::string>> del_list;
        for (const auto& k : _sets)
        {
            if (k.second.size() == 1 && _remove_single)
            {
                del_list.insert(k.first);
                continue;
            }
            _file_count += k.second.size();
            boost::filesystem::path px = *(k.second.begin());
            _space_occupied += boost::filesystem::file_size(px, ec);
            if (ec && _scan_error_callback) _scan_error_callback(_search_dir, px, ec.default_error_condition());
        }
        if (_remove_single)
        {
            for (const auto& kr: del_list)
            {
                _sets.erase(kr);
            }
        }

        if (_scan_completed_callback) _scan_completed_callback(_search_dir, _files_encountered, _file_count, _sets.size(), _space_occupied);
    }

    template<typename SorterT>
    void duplicate_files_scanner<SorterT>::_process_file(const boost::filesystem::path& dirent, bool recurse)
    {
        boost::system::error_code ec;
        boost::filesystem::path p;
        bool hidden = is_hidden(dirent, ec);
        if (ec)
        {
            if (_scan_error_callback) _scan_error_callback(_search_dir, dirent, ec.default_error_condition());
            return;
        }
        bool symlink = boost::filesystem::is_symlink(dirent, ec);
        if (ec)
        {
            if (_scan_error_callback) _scan_error_callback(_search_dir, dirent, ec.default_error_condition());
            return;
        }

        if (_skip_hidden && hidden) return;

        if (symlink)
        {
            if (_follow_links)
            {
                p = boost::filesystem::canonical(dirent, ec);
                if (ec)
                {
                    if (_scan_error_callback) _scan_error_callback(_search_dir, dirent, ec.default_error_condition());
                    return;
                }
            }
            else
            {
                return;
            }
        }
        else
        {
            p = boost::filesystem::canonical(dirent, ec);
            if (ec)
            {
                if (_scan_error_callback) _scan_error_callback(_search_dir, dirent, ec.default_error_condition());
                return;
            }
        }

        if (!boost::filesystem::exists(p, ec) && !ec) return;
        if (ec)
        {
            if (_scan_error_callback) _scan_error_callback(_search_dir, p, ec.default_error_condition());
            return;
        }

        // ---------------------------------------------------------------------------------------------------------
        // Directory.
        // ---------------------------------------------------------------------------------------------------------
        bool directory = boost::filesystem::is_directory(p, ec);
        if (ec)
        {
            if (_scan_error_callback) _scan_error_callback(_search_dir, p, ec.default_error_condition());
            return;
        }
        std::error_code sec;
        if (directory)
        {
            directory_enumerator de(p);
            while (de.move_next(sec))
            {
                _process_file(de.current(), recurse);
            }
            if (sec && _scan_error_callback) _scan_error_callback(_search_dir, p, sec.default_error_condition());
        }

        // ---------------------------------------------------------------------------------------------------------
        // File.
        // ---------------------------------------------------------------------------------------------------------
        if (!boost::filesystem::is_regular_file(p, ec) && !ec) return;
        if (ec)
        {
            if (_scan_error_callback) _scan_error_callback(_search_dir, p, ec.default_error_condition());
            return;
        }
        if (!_extensions.empty())
        {
            auto e = p.extension().string();
            if (!_extensions.contains(e)) return;
        }
        _files_encountered++;
        std::tuple<uintmax_t, std::string> h;
        if (!_get_hash_of_file(p, h, ec))
        {
            if (ec && _scan_error_callback) _scan_error_callback(_search_dir, p, ec.default_error_condition());
            return;
        }

        //std::cout << "Hash key of " << p << " " << std::get<0>(h) << " " << std::get<1>(h) << std::endl;

        _list_lock.lock();
        if (_sets.contains(h))
        {
            _sets.at(h).emplace(p);
            if (_sets.at(h).size() == 2) _sets_found++;
        }
        else
        {
            auto result = _sets.emplace(std::make_pair(h, set_t()));
            if (result.second) result.first->second.emplace(p);
        }
        if (_scan_progress_callback) _scan_progress_callback(_search_dir, _files_encountered, _sets_found);
        _list_lock.unlock();
    }

    template<typename SorterT>
    bool duplicate_files_scanner<SorterT>::_get_hash_of_file(const boost::filesystem::path& p, std::tuple<std::uintmax_t, std::string>& out, boost::system::error_code& ec)
    {
        ec.clear();
        std::unique_ptr<uint8_t> _buffer;
        auto s = boost::filesystem::file_size(p, ec);
        if (ec) return false;
        if ((s < _min_size) || (s > _max_size)) return false;

        size_t buffer_size = (s < 10485760) ? s : 10485760;
        _buffer = std::unique_ptr<uint8_t>(new uint8_t[buffer_size]);

        static constexpr char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
        Botan::Skein_512 hasher;
        auto digest_length = hasher.output_length();

        // Try to open the file.
        FILE *file = nullptr;
        for (;;)
        {
            file = fopen(p.string().c_str(), "rb");
            if (file == nullptr)
            {
                if ((errno == ENFILE) || (errno == EMFILE) || (errno == ENOSR) || (errno == EAGAIN))
                {
                    boost::this_thread::sleep_for(boost::chrono::seconds(5));
                    continue;
                }
                else
                {
                    ec = boost::system::error_code(errno, boost::system::system_category());
                    return false;
                }
            }
            else
            {
                break;
            }
        }

        std::string result;
        result.reserve(digest_length * 2);   // Two digits per character
        // If the contents of the file will fit inside the hash string then we can save a pointless hashing operation.
        if (s <= digest_length)
        {
            result.insert(0, (digest_length * 2), '0');
            auto bytes_read = fread(_buffer.get(), 1, s, file);
            if (bytes_read != s)
            {
                ec = boost::system::error_code(errno, boost::system::system_category());
                fclose(file);
                return false;
            }
            auto temp = _buffer.get();
            auto it = result.begin();
            for (size_t i = 0; i < s; i++)
            {
                auto c = temp[i];
                *it = hex[c / 16];
                it++;
                *it = hex[c % 16];
            }
        }
        else
        {
            while (!feof(file))
            {
                auto bytes_read = fread(_buffer.get(), 1, buffer_size, file);
                if (bytes_read == 0)
                {
                    ec = boost::system::error_code(errno, boost::system::system_category());
                    fclose(file);
                    return false;
                }
                hasher.update(_buffer.get(), bytes_read);
            }
            auto digest = hasher.final_stdvec();

            for (uint8_t c : digest)
            {
                result.push_back(hex[c / 16]);
                result.push_back(hex[c % 16]);
            }
        }
        fclose(file);
        out = std::make_tuple(s, result);

        return true;
    }
}

#endif
