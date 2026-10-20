// aString.h - String, the string class of the course project.
//
// The course asks for a string class with two data members: the size (int)
// and the characters (char*). String owns its character buffer manually with
// new[] and delete[], so it implements the complete rule of five. It does not
// use std::string.
//
// Invariant: if size_ is 0, chars_ is nullptr. Otherwise chars_ points to a
// buffer of size_ + 1 chars that String owns, and chars_[size_] is '\0'.
// Thus a moved-from String is an empty String, and the move operations never
// allocate and never throw.
#pragma once

#include <iosfwd>
#include <string_view>

namespace strset {

class String {
public:
    /// The empty string. It allocates nothing.
    String() noexcept = default;
    /// A copy of a NUL-terminated text. A nullptr gives the empty string.
    String(const char* text);  // NOLINT(google-explicit-constructor): String("pear") reads well.
    /// A copy of `text`. The text can contain '\0' characters.
    /// Throws std::length_error if the text has more than INT_MAX characters.
    explicit String(std::string_view text);

    String(const String& other);
    String(String&& other) noexcept;
    String& operator=(const String& other);
    String& operator=(String&& other) noexcept;
    ~String();

    /// The number of characters, without the terminating '\0'.
    [[nodiscard]] int size() const noexcept { return size_; }
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    /// A NUL-terminated text. It is "" for the empty string, never nullptr.
    [[nodiscard]] const char* c_str() const noexcept { return chars_ != nullptr ? chars_ : ""; }
    /// The characters, without a copy.
    [[nodiscard]] std::string_view view() const noexcept;
    /// The buffer that this String owns, or nullptr for the empty string.
    /// The tests use it to show that a copy has its own buffer.
    [[nodiscard]] const char* buffer() const noexcept { return chars_; }

    /// The character at `index`. Throws std::out_of_range if index is not in [0, size()).
    [[nodiscard]] char at(int index) const;

    /// Compares the characters as unsigned bytes, as strcmp() does, but it
    /// also works with '\0' inside the text. A string that is a prefix of a
    /// longer string is less. Returns a negative number, 0 or a positive number.
    [[nodiscard]] int compare(const String& other) const noexcept;

    friend void swap(String& a, String& b) noexcept;

    friend bool operator==(const String& a, const String& b) noexcept { return a.compare(b) == 0; }
    friend bool operator!=(const String& a, const String& b) noexcept { return a.compare(b) != 0; }
    friend bool operator<(const String& a, const String& b) noexcept { return a.compare(b) < 0; }
    friend bool operator<=(const String& a, const String& b) noexcept { return a.compare(b) <= 0; }
    friend bool operator>(const String& a, const String& b) noexcept { return a.compare(b) > 0; }
    friend bool operator>=(const String& a, const String& b) noexcept { return a.compare(b) >= 0; }

private:
    int size_ = 0;
    char* chars_ = nullptr;
};

/// The index of the first character that is different in `a` and `b`. If one
/// string is a prefix of the other one, the result is the size of the
/// shorter string. If the strings are equal, the result is their size.
[[nodiscard]] int first_difference(const String& a, const String& b) noexcept;

std::ostream& operator<<(std::ostream& out, const String& value);

}  // namespace strset
