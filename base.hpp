#include <stdexcept>
#include <algorithm>
#include <filesystem>
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
    namespace _private
    {
#if defined(_WIN32)
#define PLATFORM_NAME "windows" // Windows
#elif defined(_WIN64)
#define PLATFORM_NAME "windows" // Windows
#elif defined(__CYGWIN__) && !defined(_WIN32)
#define PLATFORM_NAME "windows" // Windows (Cygwin POSIX under Microsoft Window)
#elif defined(__ANDROID__)
#define PLATFORM_NAME "android" // Android (implies Linux, so it must come first)
#elif defined(__linux__)
#define PLATFORM_NAME "linux" // Debian, Ubuntu, Gentoo, Fedora, openSUSE, RedHat, Centos and other
#elif defined(__unix__) || !defined(__APPLE__) && defined(__MACH__)
        #include <sys/param.h>
    #if defined(BSD)
        #define PLATFORM_NAME "bsd" // FreeBSD, NetBSD, OpenBSD, DragonFly BSD
    #endif
#elif defined(__hpux)
    #define PLATFORM_NAME "hp-ux" // HP-UX
#elif defined(_AIX)
    #define PLATFORM_NAME "aix" // IBM AIX
#elif defined(__APPLE__) && defined(__MACH__) // Apple OSX and iOS (Darwin)
    #include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR == 1
        #define PLATFORM_NAME "ios" // Apple iOS
    #elif TARGET_OS_IPHONE == 1
        #define PLATFORM_NAME "ios" // Apple iOS
    #elif TARGET_OS_MAC == 1
        #define PLATFORM_NAME "osx" // Apple OSX
    #endif
#elif defined(__sun) && defined(__SVR4)
    #define PLATFORM_NAME "solaris" // Oracle Solaris, Open Indiana
#else
    #define PLATFORM_NAME NULL
#endif
    }
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
        struct last_write_sort
        {
            bool operator()(const std::filesystem::path& lhs, const std::filesystem::path& rhs) const
            {
                std::error_code ec;
                if (lhs.empty() || rhs.empty()) return false;
                if (std::filesystem::equivalent(lhs, rhs, ec)) return false;
                auto t1 = std::filesystem::last_write_time(lhs);
                auto t2 = std::filesystem::last_write_time(rhs);

                return (t1 < t2);
            }
        } sort_by_last_write_time;

        struct file_size_sort
        {
            bool operator()(const std::filesystem::path& lhs, const std::filesystem::path& rhs) const
            {
                std::error_code ec;
                if (lhs.empty() || rhs.empty()) return false;
                if (std::filesystem::equivalent(lhs, rhs, ec)) return false;
                auto s1 = std::filesystem::file_size(lhs);
                auto s2 = std::filesystem::file_size(rhs);

                return (s1 < s2);
            }
        } sort_by_file_size;

        class filename_sort
        {
        private:
            boost::wregex re;
        public:
            filename_sort()
            {
                re.assign(L"(?:[\\(\\[{_])(\\d+)(?:[\\)\\]}_])", boost::regex::icase);
            }

            bool operator()(const std::filesystem::path& lhs, const std::filesystem::path& rhs) const
            {
                std::error_code ec;
                if (lhs.empty() || rhs.empty()) return false;
                if (std::filesystem::equivalent(lhs, rhs, ec)) return false;
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

                // Check for substrings
                if (n1.size() < n2.size()) return boost::icontains(n2, n1);

                return boost::ilexicographical_compare(n1, n2);
            }
        } sort_by_filename;

        class directory_scanner
        {
        protected:
            bool _follow_links;
            std::set<std::filesystem::path> _extensions;
            size_t _min_size;
            size_t _max_size;
            bool _skip_hidden;
            uintmax_t _files_examined;
        public:
            directory_scanner()
            {
                _follow_links = false;
                _files_examined = 0;
                _min_size = 0;
                _max_size = SIZE_MAX;
                _skip_hidden = false;
            }

            virtual void clear() noexcept = 0;

            virtual void perform_scan(const std::filesystem::path& search_dir, bool recursive) = 0;

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

            [[nodiscard]] const std::set<std::filesystem::path>& filters() const noexcept
            {
                return _extensions;
            }

            [[nodiscard]] uintmax_t files_examined() const noexcept
            {
                return _files_examined;
            }
        };

        bool is_hidden(const std::filesystem::path& p)
        {
            bool ret = false;
            if (p.empty()) throw std::invalid_argument("The given path was empty.");
            auto fn = p.filename().wstring();
            if (fn[0] == '.') ret = true;

            return ret;
        }

        bool is_hidden(const std::filesystem::path& p, std::error_code& ec) noexcept
        {
            ec.clear();

            bool ret = false;
            if (p.empty())
            {
                ec = std::make_error_code(std::errc::invalid_argument);
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
