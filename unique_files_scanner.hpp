#include "duplicate_files_scanner.hpp"

#ifndef _UNIQUE_FILE_SCANNER_HPP_
#define _UNIQUE_FILE_SCANNER_HPP_

namespace oasis::filesystem
{
    class unique_files_scanner : public directory_scanner
    {
    private:
        duplicate_files_scanner _scanner;
        std::vector<std::filesystem::path> _files;
        std::function<void(uintmax_t, uintmax_t, operation_state)> _progress_callback;
        void _callback_broker(uintmax_t examined, uintmax_t unused, operation_state state)
        {
            if (_progress_callback) _progress_callback(examined, _scanner._sets.size(), state);
        }

    public:
        using iterator = typename std::vector<std::filesystem::path>::iterator;
        using const_iterator = typename std::vector<std::filesystem::path>::const_iterator;
        using reverse_iterator = typename std::vector<std::filesystem::path>::reverse_iterator;
        using const_reverse_iterator = typename std::vector<std::filesystem::path>::const_reverse_iterator;
        //using size_type = typename std::vector<std::filesystem::path>::size_type;

        [[nodiscard]] iterator begin() noexcept
        {
            return _files.begin();
        }

        [[nodiscard]] const_iterator begin() const noexcept
        {
            return _files.begin();
        }

        [[nodiscard]] const_iterator cbegin() const noexcept
        {
            return _files.cbegin();
        }

        [[nodiscard]] iterator end() noexcept
        {
            return _files.end();
        }

        [[nodiscard]] const_iterator end() const noexcept
        {
            return _files.end();
        }

        [[nodiscard]] const_iterator cend() const noexcept
        {
            return _files.cend();
        }

        reverse_iterator rbegin() noexcept
        {
            return _files.rbegin();
        }

        [[nodiscard]] const_reverse_iterator rbegin() const noexcept
        {
            return _files.rbegin();
        }

        [[nodiscard]] const_reverse_iterator crbegin() const noexcept
        {
            return _files.crbegin();
        }

        reverse_iterator rend() noexcept
        {
            return _files.rend();
        }

        [[nodiscard]] const_reverse_iterator rend() const noexcept
        {
            return _files.rend();
        }

        [[nodiscard]] const_reverse_iterator crend() const noexcept
        {
            return _files.crend();
        }

        [[nodiscard]] bool empty() const noexcept override
        {
            return _files.empty();
        }

        [[nodiscard]] std::size_t size() const noexcept override
        {
            return _files.size();
        }

        void set_progress_callback(const std::function<void(uintmax_t, uintmax_t, operation_state)>& callback)
        {
            _progress_callback = callback;
        }

        unique_files_scanner& operator=(unique_files_scanner&& other) noexcept
        {
            _scanner = std::move(other._scanner);
            _files = std::move(other._files);

            return *this;
        }

        unique_files_scanner& operator=(const unique_files_scanner&) = delete;

        unique_files_scanner() : directory_scanner()
        {

        }

        unique_files_scanner(const unique_files_scanner& other) = delete;

        unique_files_scanner(unique_files_scanner&& other) noexcept
        {
            _scanner = std::move(other._scanner);
            _files = std::move(other._files);
            _include_zero_byte = other._include_zero_byte;
            _files_examined = other._files_examined;
            _extensions = std::move(other._extensions);
        }

        void perform_scan(const std::filesystem::path& search_dir, bool recursive) override
        {
            _scanner._remove_single = false;
            _scanner.set_follow_symlinks(this->_follow_links);
            _scanner.set_include_empty_files(this->_include_zero_byte);
            _scanner.add_filters(this->_extensions);
            _scanner.perform_scan(search_dir, recursive);

            for (const auto& ds : _scanner._sets)
            {
                _files.push_back(*(ds.second.begin()));
            }
        }

        template<typename CompT>
        void sort(CompT compare = sort_by_filename)
        {
            std::sort(_files.begin(), _files.end(), compare);
        }

        void clear() noexcept override
        {
            _scanner.clear();
            _files.clear();
        }
    };
}

#endif //_UNIQUE_SCANNER_LIST_HPP_
