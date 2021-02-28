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
#include <openssl/evp.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include "foundation.hpp"
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
        boost::mutex _list_lock;
        boost::mutex _counter_lock;
        uintmax_t _file_count;
        uintmax_t _space_occupied;
        uintmax_t _sets_found;
        boost::thread_group _threads;
        std::function<void(const boost::filesystem::path&)> _scan_started_callback;
        std::function<void(const boost::filesystem::path&, uintmax_t, uintmax_t)> _scan_progress_callback;
        std::function<void(const boost::filesystem::path&, uintmax_t, uintmax_t, uintmax_t, uintmax_t)> _scan_completed_callback;
        std::function<void(const boost::filesystem::path&, const boost::filesystem::path&, const std::error_condition&)> _scan_error_callback;
        using set_t = std::set<boost::filesystem::path, SorterT>;
        using map_t = std::map<std::string, set_t>;
        map_t _sets;

        friend class unique_files_scanner;

        void _process_filesystem_entry(const boost::filesystem::path& dirent, bool recurse);
        void _process_file(boost::filesystem::path p);

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
            _sets_found = other._sets_found;
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
            _sets_found = other._sets_found;
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
            _sets_found = other._sets_found;
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

        void perform_scan(bool recurse) override;

    };

    template<typename SorterT>
    void duplicate_files_scanner<SorterT>::perform_scan(bool recurse)
    {
        if (_scan_started_callback) _scan_started_callback(_search_dir);

        boost::system::error_code ec;
        directory_enumerator de(_search_dir);
        while (de.move_next(ec))
        {
            _process_filesystem_entry(de.current(), recurse);
        }
        if (ec && _scan_error_callback) _scan_error_callback(_search_dir, boost::filesystem::path(), ec.default_error_condition());

        // Wait for threads.
        //_threads.join_all();

        // Work out the statistics.
        std::set<std::string> del_list;
        for (const auto& k : _sets)
        {
            if (k.second.size() == 1)
            {
                if (_remove_single)
                {
                    del_list.insert(k.first);
                    continue;
                }
                _file_count++;
                boost::filesystem::path px = *(k.second.begin());
                _space_occupied += boost::filesystem::file_size(px, ec);
                if (ec && _scan_error_callback) _scan_error_callback(_search_dir, px, ec.default_error_condition());
            }
            else
            {
                _file_count += k.second.size() - 1;
                boost::filesystem::path px = *(k.second.begin());
                auto fsize = boost::filesystem::file_size(px, ec);
                if (ec && _scan_error_callback) _scan_error_callback(_search_dir, px, ec.default_error_condition());
                _space_occupied += (fsize * (k.second.size() - 1));
            }
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
    void duplicate_files_scanner<SorterT>::_process_filesystem_entry(const boost::filesystem::path& dirent, bool recurse)
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
        boost::system::error_code sec;
        if (directory)
        {
            directory_enumerator de(p);
            while (de.move_next(sec))
            {
                _process_filesystem_entry(de.current(), recurse);
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
            std::cout << "EXTENSIONS NOT EMPTY!!" << std::endl;
            auto e = p.extension().string();
            if (!_extensions.contains(e)) return;
        }

        /*try
        {
            // Try to run the add routine on a separate thread...
            _threads.create_thread([p, this]() { _add_to_set(p); });
        }
        catch(...)
        {
            // If we cannot, we will have to run it on the main thread.
            _process_file(p);
        } */

        _process_file(p);
    }

    template <typename SorterT>
    void duplicate_files_scanner<SorterT>::_process_file(boost::filesystem::path p)
    {
        boost::system::error_code ec;
        _counter_lock.lock();
        _files_encountered++;
        _counter_lock.unlock();

        boost::filesystem::path directory = p;
        directory.remove_filename();

        unsigned char digest[EVP_MAX_MD_SIZE];
        std::stringstream ss;
        std::string h;
        //std::unique_ptr<uint8_t, free_delete> _buffer;
        auto file_size = boost::filesystem::file_size(p, ec);
        if (ec)
        {
            if (_scan_error_callback) _scan_error_callback(directory, p, ec.default_error_condition());
            return;
        }
        if ((file_size < _min_size) || (file_size > _max_size)) return;

        // Try to get some memory.
        size_t buffer_size = (file_size < 10485760) ? file_size : 10485760;
        auto _buffer = make_buffer(buffer_size);

        EVP_MD_CTX *evp_ctx = EVP_MD_CTX_new();

        // Try to open the file.
        FILE *file = nullptr;
        for (;;)
        {
#if defined (_MSC_VER)
            file = _wfopen(p.wstring().c_str(), L"rbS");
#else
            file = fopen64(p.string().c_str(), "rb");
#endif
            if (file == nullptr)
            {
                if ((errno == ENFILE) || (errno == EMFILE) || (errno == ENOSR) || (errno == EAGAIN))
                {
                    boost::this_thread::sleep_for(boost::chrono::seconds(5));
                    sleep(5);
                    continue;
                }
                else
                {
                    ec = boost::system::error_code(errno, boost::system::system_category());
                    if (_scan_error_callback) _scan_error_callback(directory, p, ec.default_error_condition());
                    return;
                }
            }
            else
            {
                break;
            }
        }

        // Start building the hash string key.
        ss << file_size << ":";

        // Deal with zero byte files
        if (file_size == 0)
        {
            h = "0:0";
        }
        else if (file_size <= EVP_MAX_MD_SIZE) // If the contents of the file will fit inside the hash buffer then we can save a pointless hashing operation.
        {
            auto bytes_read = fread(digest, 1, file_size, file);
            if (bytes_read != file_size)
            {
                ec = boost::system::error_code(errno, boost::system::system_category());
                if (_scan_error_callback) _scan_error_callback(directory, p, ec.default_error_condition());
                fclose(file);
                return;
            }
            char *hex_out = OPENSSL_buf2hexstr(digest, file_size);
            ss << hex_out;
            OPENSSL_free(hex_out);
            h = ss.str();
        }
        else
        {
            EVP_DigestInit(evp_ctx, EVP_sha512());
#if defined (_MSC_VER)
            while (!feof(file) && (_ftelli64 < file_size))
#else
            while (!feof(file) && (ftello64(file) < file_size))
#endif
            {
                auto bytes_read = fread(_buffer.get(), 1, buffer_size, file);
                if ((bytes_read == 0) || ferror(file))
                {
                    ec = boost::system::error_code(errno, boost::system::system_category());
                    if (_scan_error_callback) _scan_error_callback(directory, p, ec.default_error_condition());
                    fclose(file);
                    EVP_MD_CTX_free(evp_ctx);
                    return;
                }
                EVP_DigestUpdate(evp_ctx, _buffer.get(), bytes_read);
            }
            EVP_DigestFinal(evp_ctx, digest, nullptr);
            char *hex_out = OPENSSL_buf2hexstr(digest, EVP_MAX_MD_SIZE);
            ss << hex_out;
            OPENSSL_free(hex_out);
            h = ss.str();
        }
        fclose(file);

        // Free the memory as soon as we are finished with it.
        _buffer.reset();

        // Query set for discovered hash.
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
        _list_lock.unlock();
        if (_scan_progress_callback) _scan_progress_callback(directory, _files_encountered, _sets_found);
    }
}

#endif
