/*
 * Copyright (c) 2020 Chris Morrison
 *
 * Filename: duplicate_file_set.hpp
 * Created on Thu Dec 24 2020
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <set>
#include <stdexcept>
#include <boost/filesystem.hpp>
#include "foundation.hpp"

#ifndef B1C51771_2BAB_4A66_8E74_43BAB9E3532A

namespace oasis::filesystem
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Provides a container for duplicate files, that is files that are identical in size and
    ///        content, but do not refer to the same disk area.
    ///
    /// @tparam SorterT A type that will be used to sort the files in the set.
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename SorterT = std::less<boost::filesystem::path>>
    class duplicate_file_set
    {
    private:
        boost::filesystem::path _principal;
        std::set<boost::filesystem::path, SorterT> _set;
        std::string _hash;
    public:
        using reference = typename std::set<boost::filesystem::path, SorterT>::reference;
        using const_reference = typename std::set<boost::filesystem::path, SorterT>::const_reference;
        using iterator = typename std::set<boost::filesystem::path, SorterT>::iterator;
        using const_iterator = typename std::set<boost::filesystem::path, SorterT>::const_iterator;
        using reverse_iterator = typename std::set<boost::filesystem::path, SorterT>::reverse_iterator;
        using const_reverse_iterator = typename std::set<boost::filesystem::path, SorterT>::const_reverse_iterator;
        using difference_type = typename std::set<boost::filesystem::path, SorterT>::difference_type;
        using size_type = typename std::set<boost::filesystem::path, SorterT>::size_type;

        template<typename T>
        friend class duplicate_files_scanner;

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @brief Copy construct a new duplicate_file_set object from the given instance.
        ///
        /// @param other Another duplicate_file_set object, of which this instance will be a copy.
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        duplicate_file_set(const duplicate_file_set& other) noexcept
        {
            _hash = other._hash;
            _set = other._set;
            _principal = other._principal;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @brief Move construct a new duplicate file set object from the given instance.
        ///
        /// @param other The duplicate_file_set object that is to be moved into this instance.
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        duplicate_file_set(duplicate_file_set&& other) noexcept
        {
            _hash = std::move(other._hash);
            _set = std::move(other._set);
            _principal = std::move(other._principal);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @brief Copy assigns the given duplicate_file_set object to this instance.
        ///
        /// @param other The duplicate_file_set object to assign to this instance.
        /// @return A reference to this instance.
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        duplicate_file_set& operator=(const duplicate_file_set& other) noexcept
        {
            _hash = other._hash;
            _set = other._set;
            _principal = other._principal;

            return *this;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @brief Move assigns the given duplicate_file_set object to this instance.
        ///
        /// @param other The duplicate_file_set object to move into this instance.
        /// @return A reference to this instance.
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        duplicate_file_set& operator=(duplicate_file_set&& other) noexcept
        {
            _hash = std::move(other._hash);
            _set = std::move(other._set);
            _principal = std::move(other._principal);

            return *this;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @brief Compares this instance to <tt>other</tt>.
        ///
        /// @param other The duplicate_file_set object to compare with this instance.
        /// @return Returns a negative value if the hash string of this instance appears before the hash
        ///         string of <tt>other</tt>, in lexicographical order.
        ///
        ///         Zero if both hash strings compare equivalent.
        ///
        ///         A positive value if the hash string of this instance appears after the hash string of
        ///         <tt>other</tt>, in lexicographical order.
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        [[nodiscard]] int compare(const duplicate_file_set& other) const noexcept
        {
            return _hash.compare(other._hash);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @brief Compares this instance to <tt>other</tt>
        ///
        /// @tparam StringT An array of ASCII characters, of a type convertible to <tt>char</tt> or a string
        ///         type that satisfies the C++ named requirement: <em>Container</em>.
        /// @param hash  A hash string to compare with the hash string of this instance.
        /// @return @parblock Returns a negative value if the hash string of this instance appears before
        ///         the hash string of <tt>other</tt>, in lexicographical order.
        ///
        ///         Zero if both hash strings compare equivalent.
        ///
        ///         A positive value if the hash string of this instance appears after the hash string of
        ///         <tt>other</tt>, in lexicographical order. @endparblock
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        template<typename StringT>
        [[nodiscard]] int compare(const StringT& hash) const noexcept
        {
            duplicate_file_set<SorterT> other(hash);
            return _hash.compare(other._hash);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @brief Gets the first file path that was added to this set.
        ///
        /// @return Returns a <tt>boost::filesystem::path</tt> object that represents the principal of this
        ///         set.
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        [[nodiscard]] boost::filesystem::path principal() const noexcept
        {
            return _principal;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @brief Gets the number of files in this set <b>excluding</b> the principal.
        ///
        /// @return Returns the number of file path entries in this set that are a duplicate of the
        ///         principal.
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        size_type size() const noexcept
        {
            return _set.size();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @brief Gets the maximum size of the set <b>excluding</b> the principal.
        ///
        /// @return Returns the maximum number of file path entries a set can hold that are a duplicate
        ///         of the principal.
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        size_type max_size() const noexcept
        {
            return _set.max_size();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @brief Determines if the set is empty, that is, it does not contain any file path entries other
        ///        than the principal.
        ///
        /// @return Returns <tt>true</tt> if the set contains no files that are a duplicate of the
        ///         principal; otherwise <tt>false</tt>.
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        bool empty() const noexcept
        {
            return _set.empty();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @brief Gets an iterator referring to the first entry in the set.
        ///
        /// @return Returns a read-only (constant) iterator that points to the first entry in the set.
        ///         Iteration is done in ascending order.
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        iterator begin() const noexcept
        {
            return _set.begin();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @brief \parblock Gets an iterator referring to the past-the-end entry in the set.
        ///
        ///        The past-the-end entry is the theoretical entry that would follow the last entry in the
        ///        set. It does not point to any entry, and thus shall not be dereferenced. \endparblock
        ///
        /// @return Returns a read-only (constant) iterator that points to the entry past the last entry in
        ///         the set.
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        iterator end() const noexcept
        {
            return _set.end();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @brief Gets a constant iterator referring to the first entry in the set.
        ///
        /// @return Returns a read-only (constant) iterator that points to the first entry in the set.
        ///         Iteration is done in ascending order.
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        const_iterator cbegin() const noexcept
        {
            return _set.cbegin();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @brief \parblock Gets a constant iterator referring to the past-the-end entry in the set.
        ///
        ///        The past-the-end entry is the theoretical entry that would follow the last entry in the
        ///        set. It does not point to any entry, and thus shall not be dereferenced. \endparblock
        ///
        /// @return Returns a read-only (constant) iterator that points to the entry past the last entry in
        ///         the set.
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        const_iterator cend() const noexcept
        {
            return _set.cend();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @brief Gets an reverse iterator referring to the last entry in the set.
        ///
        /// @return Returns a read-only (constant) reverse iterator that points to the last entry in the
        ///         set. Iteration is done in descending order.
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        reverse_iterator rbegin() const noexcept
        {
            return _set.rbegin();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @brief \parblock Gets an iterator referring to the before-the-beginning (reverse end) entry in
        ///        the set.
        ///
        ///        The reverse end entry is the theoretical entry that would precede the first entry in the
        ///        set. It does not point to any entry, and thus shall not be dereferenced. \endparblock
        ///
        /// @return Returns a read-only (constant) iterator that points to the entry before the first entry
        ///         in the set.
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        reverse_iterator rend() const noexcept
        {
            return _set.rend();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @brief Gets a constant reverse iterator referring to the last entry in the set.
        ///
        /// @return Returns a read-only (constant) reverse iterator that points to the last entry in the
        ///         set. Iteration is done in descending order.
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        const_reverse_iterator crbegin() const noexcept
        {
            return _set.crbegin();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @brief \parblock Gets a constant iterator referring to the before-the-beginning (reverse end)
        ///        entry in the set.
        ///
        ///        The reverse end entry is the theoretical entry that would precede the first entry in the
        ///        set. It does not point to any entry, and thus shall not be dereferenced. \endparblock
        ///
        /// @return Returns a read-only (constant) iterator that points to the entry before the first entry
        ///         in the set.
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        const_reverse_iterator crend() const noexcept
        {
            return _set.crend();
        }

    private:

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @brief Construct a new duplicate file set object populated with the contents of the given
        ///        file list.
        ///
        /// @remark The path contained in the first entry in <tt>file_list</tt> will become the principal.
        ///
        /// @tparam StringT An array of ASCII characters, of a type convertible to <tt>char</tt> or a string
        ///         type that satisfies the C++ named requirement: <em>Container</em>.
        /// @tparam FileListT A list of <tt>boost::filesystem::path</tt> objects that satisfies the C++ named
        ///         requirement: <em>Container</em>.
        /// @param hash The hash string of the files in <tt>file_list</tt>.
        /// @param file_list A list of unique files with identical content.
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        template<typename StringT, typename FileListT>
        duplicate_file_set(const StringT& hash, const FileListT& file_list)
        {
            if (std::any_of(hash.begin(), hash.end(), oasis::not_alphanumeric)) throw std::invalid_argument("Invalid hash string");
            std::transform(hash.begin(), hash.end(), std::back_inserter(_hash), [](auto ch) { return static_cast<std::string::value_type>(ch & 0xFF); });

            for (const boost::filesystem::path& p : file_list)
            {
                if (_principal.empty())
                {
                    _principal = p;
                }
                else
                {
                    _set.insert(p);
                }
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @brief Construct a new duplicate file set object with the given hash string.
        ///
        /// @tparam StringT An array of ASCII characters, of a type convertible to <tt>char</tt> or a string
        ///         type that satisfies the C++ named requirement: <em>Container</em>.
        /// @param hash The hash string of files that will be added to this set.
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        template<typename StringT>
        explicit duplicate_file_set(const StringT& hash)
        {
            if constexpr (std::is_pointer<StringT>::value || std::is_array<StringT>::value)
            {
                size_t idx = 0;
                while (hash[idx] != 0)
                {
                    auto ch = hash[idx];
                    if (!oasis::is_alphanumeric(ch)) throw std::invalid_argument("Invalid hash string");
                    _hash.push_back(static_cast<std::string::value_type>(ch & 0xFF));
                    idx++;
                }
            }
            else
            {
                if (std::any_of(hash.begin(), hash.end(), oasis::not_alphanumeric)) throw std::invalid_argument("Invalid hash string");
                std::transform(hash.begin(), hash.end(), std::back_inserter(_hash), [](auto ch) { return static_cast<std::string::value_type>(ch & 0xFF); });
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @brief Attempts to construct and insert an entry into the set.
        ///
        /// @remark If this is the first insertion into the set, the path contained in <tt>value</tt> will
        ///         become the principal.
        ///
        /// @param value The value to use to generate the entry to be emplaced in the set.
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        void emplace(const boost::filesystem::path& value) noexcept
        {
            if (_principal.empty())
            {
                _principal = value;
            }
            else
            {
                _set.emplace(value);
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        /// @brief Attempts to insert an entry into the set.
        ///
        /// @remark If this is the first insertion into the set, the path contained in <tt>value</tt> will
        ///         become the principal.
        ///
        /// @param value The value to insert into the set.
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        void insert(const boost::filesystem::path& value) noexcept
        {
            if (_principal.empty())
            {
                _principal = value;
            }
            else
            {
                _set.insert(value);
            }
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Compares the hash strings of two duplicate_file_set objects for equality.
    ///
    ///        All comparisons are carried out using the compare() member function.
    ///
    /// @tparam SorterT A type that will be used to sort the files in the set.
    /// @param lhs The first duplicate_file_set object to compare.
    /// @param rhs The second duplicate_file_set object to compare.
    /// @return Returns <tt>true</tt> if the comparison holds; otherwise <tt>false</tt>.
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename SorterT>
    inline bool operator==(const duplicate_file_set<SorterT>& lhs, const duplicate_file_set<SorterT>& rhs) noexcept
    {
        return (lhs.compare(rhs) == 0);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Compares the hash strings of two duplicate_file_set objects for inequality.
    ///
    ///        All comparisons are carried out using the compare() member function.
    ///
    /// @tparam SorterT A type that will be used to sort the files in the set.
    /// @param lhs The first duplicate_file_set object to compare.
    /// @param rhs The second duplicate_file_set object to compare.
    /// @return Returns <tt>true</tt> if the comparison holds; otherwise <tt>false</tt>.
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename SorterT>
    inline bool operator!=(const duplicate_file_set<SorterT>& lhs, const duplicate_file_set<SorterT>& rhs)
    {
        return (lhs.compare(rhs) != 0);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Compares the hash strings of two duplicate_file_set objects to determine if <tt>lhs</tt>
    ///        appears before <tt>rhs</tt>, in lexicographical order.
    ///
    ///        All comparisons are carried out using the compare() member function.
    ///
    /// @tparam SorterT A type that will be used to sort the files in the set.
    /// @param lhs The first duplicate_file_set object to compare.
    /// @param rhs The second duplicate_file_set object to compare.
    /// @return Returns <tt>true</tt> if the comparison holds; otherwise <tt>false</tt>.
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename SorterT>
    inline bool operator<(const duplicate_file_set<SorterT>& lhs, const duplicate_file_set<SorterT>& rhs)
    {
        return (lhs.compare(rhs) < 0);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Compares the hash strings of two duplicate_file_set objects to determine if <tt>lhs</tt>
    ///        appears after <tt>rhs</tt>, in lexicographical order.
    ///
    ///        All comparisons are carried out using the compare() member function.
    ///
    /// @tparam SorterT A type that will be used to sort the files in the set.
    /// @param lhs The first duplicate_file_set object to compare.
    /// @param rhs The second duplicate_file_set object to compare.
    /// @return Returns <tt>true</tt> if the comparison holds; otherwise <tt>false</tt>.
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename SorterT>
    inline bool operator>(const duplicate_file_set<SorterT>& lhs, const duplicate_file_set<SorterT>& rhs)
    {
        return (lhs.compare(rhs) > 0);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Compares the hash strings of two duplicate_file_set objects to determine if <tt>lhs</tt>
    ///        appears before or is equal to <tt>rhs</tt>, in lexicographical order.
    ///
    ///        All comparisons are carried out using the compare() member function.
    ///
    /// @tparam SorterT A type that will be used to sort the files in the set.
    /// @param lhs The first duplicate_file_set object to compare.
    /// @param rhs The second duplicate_file_set object to compare.
    /// @return Returns <tt>true</tt> if the comparison holds; otherwise <tt>false</tt>.
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename SorterT>
    inline bool operator<=(const duplicate_file_set<SorterT>& lhs, const duplicate_file_set<SorterT>& rhs)
    {
        return (lhs.compare(rhs) <= 0);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Compares the hash strings of two duplicate_file_set objects to determine if <tt>lhs</tt>
    ///        appears after or is equal to <tt>rhs</tt>, in lexicographical order.
    ///
    ///        All comparisons are carried out using the compare() member function.
    ///
    /// @tparam SorterT A type that will be used to sort the files in the set.
    /// @param lhs The first duplicate_file_set object to compare.
    /// @param rhs The second duplicate_file_set object to compare.
    /// @return Returns <tt>true</tt> if the comparison holds; otherwise <tt>false</tt>.
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename SorterT>
    inline bool operator>=(const duplicate_file_set<SorterT>& lhs, const duplicate_file_set<SorterT>& rhs)
    {
        return (lhs.compare(rhs) >= 0);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Compares the hash string of <tt>lhs</tt> with <tt>rhs</tt> for equality.
    ///
    ///        All comparisons are carried out using the compare() member function.
    ///
    /// @tparam SorterT A type that will be used to sort the files in the set.
    /// @tparam StringT An array of ASCII characters, of a type convertible to <tt>char</tt> or a string
    ///         type that satisfies the C++ named requirement: <em>Container</em>.
    /// @param lhs A duplicate_file_set object, the hash string of which will be compared to
    ///            <tt>rhs</tt>
    /// @param rhs A string to be compared to the hash string of <tt>lhs</tt>.
    /// @return Returns <tt>true</tt> if the comparison holds; otherwise <tt>false</tt>.
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename SorterT, typename StringT>
    inline bool operator==(const duplicate_file_set<SorterT>& lhs, const StringT& rhs)
    {
        return (lhs.compare(rhs) == 0);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Compares the hash string of <tt>lhs</tt> with <tt>rhs</tt> for inequality.
    ///
    ///        All comparisons are carried out using the compare() member function.
    ///
    /// @tparam SorterT A type that will be used to sort the files in the set.
    /// @tparam StringT An array of ASCII characters, of a type convertible to <tt>char</tt> or a string
    ///         type that satisfies the C++ named requirement: <em>Container</em>.
    /// @param lhs A duplicate_file_set object, the hash string of which will be compared to
    ///            <tt>rhs</tt>
    /// @param rhs A string to be compared to the hash string of <tt>lhs</tt>.
    /// @return Returns <tt>true</tt> if the comparison holds; otherwise <tt>false</tt>.
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename SorterT, typename StringT>
    inline bool operator!=(const duplicate_file_set<SorterT>& lhs, const StringT& rhs)
    {
        return (lhs.compare(rhs) != 0);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Compares the hash string of <tt>lhs</tt> with <tt>rhs</tt> to determine if <tt>lhs</tt>
    ///        appears before <tt>rhs</tt>, in lexicographical order.
    ///
    ///        All comparisons are carried out using the compare() member function.
    ///
    /// @tparam SorterT A type that will be used to sort the files in the set.
    /// @tparam StringT An array of ASCII characters, of a type convertible to <tt>char</tt> or a string
    ///         type that satisfies the C++ named requirement: <em>Container</em>.
    /// @param lhs A duplicate_file_set object, the hash string of which will be compared to
    ///            <tt>rhs</tt>
    /// @param rhs A string to be compared to the hash string of <tt>lhs</tt>.
    /// @return Returns <tt>true</tt> if the comparison holds; otherwise <tt>false</tt>.
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename SorterT, typename StringT>
    inline bool operator<(const duplicate_file_set<SorterT>& lhs, const StringT& rhs)
    {
        return (lhs.compare(rhs) < 0);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Compares the hash string of <tt>lhs</tt> with <tt>rhs</tt> to determine if <tt>lhs</tt>
    ///        appears after <tt>rhs</tt>, in lexicographical order.
    ///
    ///        All comparisons are carried out using the compare() member function.
    ///
    /// @tparam SorterT A type that will be used to sort the files in the set.
    /// @tparam StringT An array of ASCII characters, of a type convertible to <tt>char</tt> or a string
    ///         type that satisfies the C++ named requirement: <em>Container</em>.
    /// @param lhs A duplicate_file_set object, the hash string of which will be compared to
    ///            <tt>rhs</tt>
    /// @param rhs A string to be compared to the hash string of <tt>lhs</tt>.
    /// @return Returns <tt>true</tt> if the comparison holds; otherwise <tt>false</tt>.
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename SorterT, typename StringT>
    inline bool operator>(const duplicate_file_set<SorterT>& lhs, const StringT& rhs)
    {
        return (lhs.compare(rhs) > 0);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Compares the hash string of <tt>lhs</tt> with <tt>rhs</tt> to determine if <tt>lhs</tt>
    ///        appears before or is equal to <tt>rhs</tt>, in lexicographical order.
    ///
    ///        All comparisons are carried out using the compare() member function.
    ///
    /// @tparam SorterT A type that will be used to sort the files in the set.
    /// @tparam StringT An array of ASCII characters, of a type convertible to <tt>char</tt> or a string
    ///         type that satisfies the C++ named requirement: <em>Container</em>.
    /// @param lhs A duplicate_file_set object, the hash string of which will be compared to
    ///            <tt>rhs</tt>
    /// @param rhs A string to be compared to the hash string of <tt>lhs</tt>.
    /// @return Returns <tt>true</tt> if the comparison holds; otherwise <tt>false</tt>.
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename SorterT, typename StringT>
    inline bool operator<=(const duplicate_file_set<SorterT>& lhs, const StringT& rhs)
    {
        return (lhs.compare(rhs) <= 0);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Compares the hash string of <tt>lhs</tt> with <tt>rhs</tt> to determine if <tt>lhs</tt>
    ///        appears after or is equal to <tt>rhs</tt>, in lexicographical order.
    ///
    ///        All comparisons are carried out using the compare() member function.
    ///
    /// @tparam SorterT A type that will be used to sort the files in the set.
    /// @tparam StringT An array of ASCII characters, of a type convertible to <tt>char</tt> or a string
    ///         type that satisfies the C++ named requirement: <em>Container</em>.
    /// @param lhs A duplicate_file_set object, the hash string of which will be compared to
    ///            <tt>rhs</tt>
    /// @param rhs A string to be compared to the hash string of <tt>lhs</tt>.
    /// @return Returns <tt>true</tt> if the comparison holds; otherwise <tt>false</tt>.
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename SorterT, typename StringT>
    inline bool operator>=(const duplicate_file_set<SorterT>& lhs, const StringT& rhs)
    {
        return (lhs.compare(rhs) >= 0);
    }
}

#define B1C51771_2BAB_4A66_8E74_43BAB9E3532A
#endif /* B1C51771_2BAB_4A66_8E74_43BAB9E3532A */
