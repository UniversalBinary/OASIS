#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
#include <forward_list>
#include <tuple>
#include <execution>
#include <memory>
#include <set>
#include <thread>
#include <mutex>
#include <iterator>
//#include <execution>
#include <botan/skein_512.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include "base.hpp"

#ifndef _DUPLICATE_SCANNER_LIST_HPP_
#define _DUPLICATE_SCANNER_LIST_HPP_

namespace oasis::filesystem
{
    class duplicate_files_scanner : public directory_scanner
    {
    private:
        bool _remove_single;
        std::map<std::tuple<uintmax_t, std::string>, std::set<std::filesystem::path, filename_sort>> _sets;
        std::mutex _list_lock;
        uintmax_t _sets_found;
        std::forward_list<std::thread> _threads;
        std::function<void(const std::filesystem::path&)> _scan_started_callback;
        std::function<void(const std::filesystem::path&, uintmax_t, uintmax_t)> _scan_progress_callback;
        std::function<void(const std::filesystem::path&, uintmax_t, uintmax_t)> _scan_completed_callback;
        std::function<void(const std::filesystem::path&, const std::filesystem::path&, const std::error_condition&)> _scan_error_callback;

        friend class unique_files_scanner;

        bool _get_hash_of_file(const std::filesystem::path& p, std::tuple<std::uintmax_t, std::string>& out, std::error_code& ec)
        {
            ec.clear();
            std::unique_ptr<uint8_t> _buffer;
            auto s = std::filesystem::file_size(p, ec);
            if (ec) return false;
            if ((s < _min_size) || (s > _max_size)) return false;

            size_t buffer_size = (s < 10485760) ? s : 10485760;
            _buffer = std::unique_ptr<uint8_t>(new uint8_t[buffer_size]);

            static constexpr char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
            Botan::Skein_512 hasher;
            auto digest_length = hasher.output_length();
            std::ifstream file(p, std::ios::binary);
            if (!file) return false;

            std::string result;
            result.reserve(digest_length * 2);   // Two digits per character
            // If the contents of the file will fit inside the hash string then we can save a pointless hashing operation.
            if (s <= digest_length)
            {
                result.insert(0, (digest_length * 2), '0');
                file.read(reinterpret_cast<char *>(_buffer.get()), buffer_size);
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
                while (!file.eof())
                {
                    file.read(reinterpret_cast<char *>(_buffer.get()), buffer_size);
                    //if (file.fail()) return false;
                    auto bytes_read = file.gcount();
                    if (bytes_read == 0) return false;
                    hasher.update(_buffer.get(), bytes_read);
                }
                auto digest = hasher.final_stdvec();

                for (uint8_t c : digest)
                {
                    result.push_back(hex[c / 16]);
                    result.push_back(hex[c % 16]);
                }
            }

            out = std::make_tuple(s, result);
            return true;
        }

        void _add_file(const std::filesystem::path dirent, const std::filesystem::path search_dir)
        {
            std::error_code ec;
            std::filesystem::path p;

            if (_skip_hidden && is_hidden(dirent, ec))
            {
                if (ec && _scan_error_callback) _scan_error_callback(search_dir, dirent, ec.default_error_condition());
                return;
            }

            if (std::filesystem::is_symlink(dirent, ec))
            {
                if (_follow_links)
                {
                    p = std::filesystem::canonical(dirent, ec);
                    if (ec)
                    {
                        if (_scan_error_callback) _scan_error_callback(search_dir, dirent, ec.default_error_condition());
                        return;
                    }
                }
                else
                {
                    p = (dirent.is_absolute()) ? dirent : std::filesystem::absolute(dirent, ec);
                }
            }
            else
            {
                p = std::filesystem::canonical(dirent);
            }

            if (!std::filesystem::exists(p, ec) && !ec) return;
            if (ec)
            {
                if (_scan_error_callback) _scan_error_callback(search_dir, dirent, ec.default_error_condition());
                return;
            }
            if (!std::filesystem::is_regular_file(p, ec)) return;
            if (ec)
            {
                if (_scan_error_callback) _scan_error_callback(search_dir, dirent, ec.default_error_condition());
                return;
            }
            if (!_extensions.empty())
            {
                auto e = p.extension().string();
                if (!_extensions.contains(e)) return;
            }
            std::tuple<uintmax_t, std::string> h;
            if (!_get_hash_of_file(p, h, ec))
            {
                if (ec && _scan_error_callback) _scan_error_callback(search_dir, dirent, ec.default_error_condition());
                return;
            }

            //std::cout << "Hash key of " << p << " " << std::get<0>(h) << " " << std::get<1>(h) << std::endl;

            _list_lock.lock();
            _files_examined++;
            if (_sets.contains(h))
            {
                _sets.at(h).emplace(p);// .insert(p);
                // Raise the callback.
                if (_sets.at(h).size() == 2)
                {
                    _sets_found++;
                    if (_scan_progress_callback) _scan_progress_callback(search_dir, _files_examined, _sets_found);
                }
            }
            else
            {
                auto result = _sets.emplace(std::make_pair(h, std::set<std::filesystem::path, filename_sort>()));
                if (result.second) result.first->second.emplace(p);
            }
            _list_lock.unlock();
        }

        template<typename DirIterT> //requires std::is_same<std::filesystem::directory_iterator, DirIterT>
        void _build_list(const DirIterT& iter, const std::filesystem::path& search_dir)
        {
            for (const auto& dirent : iter)
            {
                std::thread t([dirent, search_dir, this] { _add_file(dirent.path(), search_dir); });
                _threads.push_front(std::move(t));
            }
        }

    public:
        typedef std::map<std::tuple<uintmax_t, std::string>, std::set<std::filesystem::path, filename_sort>>::size_type size_type;
        typedef std::set<std::filesystem::path, filename_sort> value_type;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef std::map<std::tuple<uintmax_t, std::string>, std::set<std::filesystem::path, filename_sort>>::difference_type difference_type;
        typedef value_type * pointer;
        typedef const pointer const_pointer;

        using map_t = std::map<std::tuple<uintmax_t, std::string>, std::set<std::filesystem::path, filename_sort>>;

        template<typename IterT>
        class basic_iterator
        {
        private:
            using value_t = typename map_t::mapped_type;
            IterT under;
        public:
            typedef std::bidirectional_iterator_tag iterator_category;
            typedef std::set<std::filesystem::path, filename_sort> value_type;
            typedef std::map<std::tuple<uintmax_t, std::string>, std::set<std::filesystem::path, filename_sort>>::difference_type difference_type;
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

        typedef basic_iterator<map_t::iterator> iterator;
        typedef basic_iterator<map_t::const_iterator> const_iterator;
        typedef basic_iterator<map_t::reverse_iterator> reverse_iterator;
        typedef basic_iterator<map_t::const_reverse_iterator> const_reverse_iterator;

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

        void set_scan_started_callback(const std::function<void(const std::filesystem::path&)>& callback)
        {
            _scan_started_callback = callback;
        }

        void set_scan_progress_callback(const std::function<void(const std::filesystem::path&, uintmax_t, uintmax_t)>& callback)
        {
            _scan_progress_callback = callback;
        }

        void set_scan_completed_callback(const std::function<void(const std::filesystem::path&, uintmax_t, uintmax_t)>& callback)
        {
            _scan_completed_callback = callback;
        }

        void set_scan_error_callback(const std::function<void(const std::filesystem::path&, const std::filesystem::path&, const std::error_condition&)>& callback)
        {
            _scan_error_callback = callback;
        }

        [[nodiscard]] uintmax_t sets_found() const
        {
            return _sets_found;
        }

        duplicate_files_scanner& operator=(duplicate_files_scanner&& other) noexcept
        {
            _sets = std::move(other._sets);
            _follow_links = other._follow_links;
            _remove_single = other._remove_single;
            _files_examined = other._files_examined;
            _sets_found = other._sets_found;
            _extensions = std::move(other._extensions);

            return *this;
        }

        duplicate_files_scanner& operator=(const duplicate_files_scanner&) = delete;

        duplicate_files_scanner() : directory_scanner()
        {
            _remove_single = true;
            _sets_found = 0;
        }

        duplicate_files_scanner(const duplicate_files_scanner& other) = delete;

        duplicate_files_scanner(duplicate_files_scanner&& other) noexcept
        {
            _sets = std::move(other._sets);
            _follow_links = other._follow_links;
            _remove_single = other._remove_single;
            _files_examined = other._files_examined;
            _sets_found = other._sets_found;
            _extensions = std::move(other._extensions);
        }

        void perform_scan(const std::filesystem::path& search_dir, bool recursive) override
        {
            if (search_dir.empty()) throw std::invalid_argument("Invalid search path");
            std::filesystem::path abs_search_dir = std::filesystem::canonical(search_dir);
            if (!std::filesystem::exists(abs_search_dir)) throw std::invalid_argument("Search path does not exist.");
            if (!std::filesystem::is_directory(abs_search_dir)) throw std::invalid_argument("Search path is not a directory");

            std::filesystem::directory_options opts = std::filesystem::directory_options::skip_permission_denied;
            if (_follow_links) opts |= std::filesystem::directory_options::follow_directory_symlink;

            if (_scan_started_callback) _scan_started_callback(abs_search_dir);

            if (recursive)
            {
                std::filesystem::recursive_directory_iterator iter(abs_search_dir, opts);
                _build_list(iter, abs_search_dir);
            }
            else
            {
                std::filesystem::directory_iterator iter(abs_search_dir, opts);
                _build_list(iter, abs_search_dir);
            }

            for (std::thread& t : _threads)
            {
                if (t.joinable()) t.join();
            }

            if (_remove_single)
            {
                std::set<std::tuple<uintmax_t, std::string>> del_list;
                for (const auto& k : _sets)
                {
                    if (k.second.size() == 1) del_list.insert(k.first);
                }
                for (const auto& kr: del_list)
                {
                    _sets.erase(kr);
                }
            }

            if (_scan_completed_callback) _scan_completed_callback(abs_search_dir, _files_examined, _sets_found);
        }
    };
}

#endif
