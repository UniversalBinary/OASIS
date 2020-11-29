#include <stdexcept>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <map>
#include <set>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/regex.h>
#include <fmt/format.h>
#include <cmath>

#ifndef _BASE_HPP_
#define _BASE_HPP_

namespace oasis
{
    enum class operation_state
    {
        imminent,
        underway,
        complete,
    };

    class invalid_operation : std::runtime_error
    {
    public:
        explicit invalid_operation(const std::string& what_arg) : std::runtime_error(what_arg)
        {

        }

        explicit invalid_operation(const char* what_arg) : std::runtime_error(what_arg)
        {

        }
    };

    template<typename StringT>
    inline StringT cleanup_spaces(StringT& str)
    {
        if (str.empty()) return str;
        boost::trim(str);
        if (str.empty()) return str;

        auto new_end = std::unique(str.begin(), str.end(), [](auto lhs, auto rhs) { return (lhs == rhs) && std::isspace(lhs); });
        str.erase(new_end, str.end());
        std::replace(str.begin(), str.end(), '_', ' ');

        return str;
    }

    /// Determines if str is a number in the Arabic (decimal) numbering system.
    /// \tparam StringT A character container type, e.g. std::string or std::wstring.
    /// \param str The string to examine.
    /// \return Returns true if str contains a number in the Arabic (decimal) numbering system; otherwise false.
    template<typename StringT>
    inline bool are_arabic_numerals(const StringT& str)
    {
        if (str.empty())
        {
            return false;
        }

        return std::all_of(str.begin(), str.end(), [](auto c) { return ((c >= '0') && (c <= '9')); });
    }

    /// Determines if str is a number in the Roman numbering system.
    /// \tparam StringT A character container type, e.g. std::string or std::wstring.
    /// \param str The string to examine.
    /// \return Returns true if str contains a number in the Roman numbering numbering system; otherwise false.
    template<typename StringT>
    inline bool are_roman_numerals(const StringT& str)
    {
        if (str.empty())
        {
            return false;
        }

        return std::all_of(str.begin(), str.end(), [](auto c) { return ((c == 'C') || (c == 'D') || (c == 'I') || (c == 'L') || (c == 'M') || (c == 'V') || (c == 'X')); });
    }

    /// Converts a string containing a number in Roman numerals to its integer representation.
    /// \tparam StringT A character container type, e.g. std::string or std::wstring.
    /// \param str The string to examine.
    /// \return
    template<typename StringT>
    inline int roman_to_int(const StringT& str)
    {
        std::map<char, int> m = {{'I', 1}, {'V', 5}, {'X', 10}, {'L', 50}, {'C', 100}, {'D', 500}, {'M', 1000}};
        int total = 0;
        StringT s = boost::to_upper_copy(str);

        if (are_arabic_numerals(s))
        {
            return std::stoi(s);
        }

        if (are_roman_numerals(s))
        {
            for (int i = 0; i < s.length(); i++)
            {
                if (m[s[i + 1]] <= m[s[i]])
                {
                    total += m[s[i]];
                }
                else
                {
                    total -= m[s[i]];
                }
            }
        }

        return total;
    }

    template<typename StringT1>
    class basic_number_formatter
    {
    private:
        StringT1 format;
        int group;
    public:
        explicit basic_number_formatter(const StringT1& fmt, int subgroup = 0)
        {
            format = fmt;
            group = subgroup;
        }

        template<typename MatchT>
        inline StringT1 operator()(const MatchT& what) const
        {
            int n = roman_to_int(what.str(group));
            return fmt::format(format, n);
        }
    };

    typedef basic_number_formatter<std::string> snumber_formatter;
    typedef basic_number_formatter<std::wstring> wsnumber_formatter;

    namespace filesystem
    {
        struct sort_by_creation_time
        {
            bool operator()(const boost::filesystem::path& lhs, const boost::filesystem::path& rhs) const
            {
                boost::system::error_code ec;
                if (lhs.empty() || rhs.empty()) return false;
                if (boost::filesystem::equivalent(lhs, rhs, ec)) return false;
                if (ec) return false;
                auto t1 = boost::filesystem::creation_time(lhs);
                auto t2 = boost::filesystem::creation_time(rhs);

                return (t1 < t2);
            }
        };

        struct sort_by_last_write_time
        {
            bool operator()(const boost::filesystem::path& lhs, const boost::filesystem::path& rhs) const
            {
                boost::system::error_code ec;
                if (lhs.empty() || rhs.empty()) return false;
                if (boost::filesystem::equivalent(lhs, rhs, ec)) return false;
                if (ec) return false;
                auto t1 = boost::filesystem::last_write_time(lhs);
                auto t2 = boost::filesystem::last_write_time(rhs);

                return (t1 < t2);
            }
        };

        struct sort_by_file_size
        {
            bool operator()(const boost::filesystem::path& lhs, const boost::filesystem::path& rhs) const
            {
                boost::system::error_code ec;
                if (lhs.empty() || rhs.empty()) return false;
                if (boost::filesystem::equivalent(lhs, rhs, ec)) return false;
                if (ec) return false;
                auto s1 = boost::filesystem::file_size(lhs);
                auto s2 = boost::filesystem::file_size(rhs);

                return (s1 < s2);
            }
        };

        class sort_by_filename
        {
        private:
            boost::wregex re;
        public:
            sort_by_filename()
            {
                re.assign(L"(?:[\\(\\[{_])(\\d+)(?:[\\)\\]}_])", boost::regex::icase);
            }

            bool operator()(const boost::filesystem::path& lhs, const boost::filesystem::path& rhs) const
            {
                boost::system::error_code ec;
                if (lhs.empty() || rhs.empty()) return false;
                if (boost::filesystem::equivalent(lhs, rhs, ec)) return false;
                if (ec) return false;
                auto n1 = lhs.filename().wstring();
                auto n2 = rhs.filename().wstring();

                // Check for numbered filenames.
                boost::wsmatch fn1;
                boost::wsmatch fn2;
                bool n1_has_num = boost::regex_search(n1, fn1, re);
                bool n2_has_num = boost::regex_search(n2, fn2, re);

                if (n1_has_num && n2_has_num)
                {
                    unsigned long num1 = std::stoull(fn1[1]);
                    unsigned long num2 = std::stoull(fn2[1]);

                    return (num1 < num2);
                }

                if (n1_has_num && !n2_has_num) return false;
                if (!n1_has_num && n2_has_num) return true;

                return boost::ilexicographical_compare(n1, n2);
            }
        };

        class directory_scanner
        {
        protected:
            bool _follow_links;
            std::set<boost::filesystem::path> _extensions;
            size_t _min_size;
            size_t _max_size;
            bool _skip_hidden;
            uintmax_t _files_encountered;
            boost::filesystem::path _search_dir;
        public:
            directory_scanner(const boost::filesystem::path& p)
            {
                if (p.empty()) throw std::invalid_argument("Invalid search path");
                _search_dir = boost::filesystem::canonical(p);
                if (!boost::filesystem::exists(_search_dir)) throw std::system_error(EEXIST, std::generic_category());
                if (!boost::filesystem::is_directory(_search_dir)) throw std::invalid_argument("Search path is not a directory");
                _follow_links = false;
                _files_encountered = 0;
                _min_size = 0;
                _max_size = SIZE_MAX;
                _skip_hidden = false;
            }

            virtual void clear() noexcept = 0;

            virtual void perform_scan(bool recursive) = 0;

            virtual std::size_t size() const noexcept = 0;

            virtual bool empty() const noexcept = 0;

            [[nodiscard]] bool skip_hidden_files() const
            {
                return _skip_hidden;
            }

            void set_skip_hidden_files(bool flag)
            {
                _skip_hidden = flag;
            }

            [[nodiscard]] size_t minimum_size() const
            {
                return _min_size;
            }

            [[nodiscard]] size_t maximum_size() const
            {
                return _max_size;
            }

            void set_minimum_size(size_t value)
            {
                _min_size = value;
            }

            void set_maximum_size(size_t value)
            {
                _max_size = value;
            }

            [[nodiscard]] bool follow_symlinks() const noexcept
            {
                return _follow_links;
            }

            void set_follow_symlinks(bool flag)
            {
                _follow_links = flag;
            }

            template<typename ContainerT>
            void add_filters(const ContainerT& list)
            {
                for (const auto& ext : list)
                {
                    _extensions.insert(ext);
                }
            }

            template<typename StringT>
            void add_filter(const StringT& filter)
            {
                auto ext = boost::to_lower_copy(filter);
                if (!ext.starts_with('.')) ext.insert(ext.begin(), '.');

                if ((ext == ".jpg") || (ext == ".jpeg"))
                {
                    _extensions.insert(".jpeg");
                    _extensions.insert(".jpg");
                }
                else if ((ext == ".tif") || (ext == ".tiff"))
                {
                    _extensions.insert(".tiff");
                    _extensions.insert(".tif");
                }
                else if ((ext == ".htm") || (ext == ".html"))
                {
                    _extensions.insert(".htm");
                    _extensions.insert(".html");
                }
                else
                {
                    _extensions.insert(ext);
                }
            }

            void add_filter(const char *filter)
            {
                std::string f(filter);
                add_filter(f);
            }

            [[nodiscard]] const std::set<boost::filesystem::path>& filters() const noexcept
            {
                return _extensions;
            }

            [[nodiscard]] uintmax_t files_examined() const noexcept
            {
                return _files_encountered;
            }
        };

        bool is_hidden(const boost::filesystem::path& p)
        {
            bool ret = false;
            if (p.empty()) throw std::invalid_argument("The given path was empty.");
            auto fn = p.filename().wstring();
            if (fn[0] == '.') ret = true;

            return ret;
        }

        bool is_hidden(const boost::filesystem::path& p, boost::system::error_code& ec) noexcept
        {
            ec.clear();

            bool ret = false;
            if (p.empty())
            {
                ec = boost::system::errc::make_error_code(boost::system::errc::invalid_argument);
                return false;
            }
            auto fn = p.filename().wstring();
            if (fn[0] == '.') ret = true;

            return ret;
        }
    }

    struct progress_data
    {
        uintmax_t total_files;
        uintmax_t files_processed;
        uintmax_t total_data;
        uintmax_t data_processed;
        int percent;

        progress_data()
        {
            total_files = 0;
            files_processed = 0;
            total_data = 0;
            data_processed = 0;
            percent = 0;
        }

        /// Creates new progress_data structure with the given data.
        /// \param tf The total number of files being processed in this operation.
        /// \param fp The total number of files that have been processed so far.
        /// \param td The total amount of data that will processed in this operation.
        /// \param dp The total amount os data that has been processed so far.
        progress_data(uintmax_t tf, uintmax_t fp, uintmax_t td, uintmax_t dp)
        {
            (*this)(tf, fp, td, dp);
        }

        /// Allows the object to be used as a functor. Updates the instance with the given values and then returns a reference to the updated object.
        /// \param tf The total number of files being processed in this operation.
        /// \param fp The total number of files that have been processed so far.
        /// \param td The total amount of data that will processed in this operation.
        /// \param dp The total amount os data that has been processed so far.
        /// \return A reference to the updated object.
        progress_data& operator()(uintmax_t tf, uintmax_t fp, uintmax_t td, uintmax_t dp)
        {
            total_files = tf;
            files_processed = fp;
            total_data = td;
            data_processed = dp;
            percent = 0;

            double fpercent = 0.0;
            if (total_data != 0)
            {
                fpercent = static_cast<double>(data_processed) / static_cast<double>(total_data);
            }
            else
            {
                fpercent = static_cast<double>(files_processed) / static_cast<double>(total_files);
            }

            fpercent *= 100.00;
            percent = std::nearbyint(fpercent);
            if (percent < 0) percent = 0;
            if (percent > 100) percent = 100;

            return *this;
        }
    };
}

#endif //_BASE_HPP_
