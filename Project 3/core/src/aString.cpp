#include "strset/aString.h"

#include <climits>
#include <cstring>
#include <ostream>
#include <stdexcept>
#include <utility>

namespace strset {

namespace {

/// A new buffer with a copy of `text` and a terminating '\0'. `size` is
/// text.size(), already checked.
char* duplicate(std::string_view text, int size) {
    if (size == 0) {
        return nullptr;
    }
    char* chars = new char[static_cast<std::size_t>(size) + 1];
    std::memcpy(chars, text.data(), text.size());
    chars[size] = '\0';
    return chars;
}

int checked_size(std::string_view text) {
    if (text.size() > static_cast<std::size_t>(INT_MAX)) {
        throw std::length_error("strset::String: the text is longer than INT_MAX characters");
    }
    return static_cast<int>(text.size());
}

}  // namespace

String::String(const char* text) : String(text != nullptr ? std::string_view{text} : std::string_view{}) {}

String::String(std::string_view text) : size_(checked_size(text)), chars_(duplicate(text, size_)) {}

String::String(const String& other) : size_(other.size_), chars_(duplicate(other.view(), other.size_)) {}

String::String(String&& other) noexcept
    : size_(std::exchange(other.size_, 0)), chars_(std::exchange(other.chars_, nullptr)) {}

String& String::operator=(const String& other) {
    // Copy and swap: if the allocation throws, *this does not change. It is
    // also correct for self-assignment.
    String copy{other};
    swap(*this, copy);
    return *this;
}

String& String::operator=(String&& other) noexcept {
    if (this != &other) {
        delete[] chars_;
        size_ = std::exchange(other.size_, 0);
        chars_ = std::exchange(other.chars_, nullptr);
    }
    return *this;
}

String::~String() { delete[] chars_; }

std::string_view String::view() const noexcept {
    return chars_ != nullptr ? std::string_view{chars_, static_cast<std::size_t>(size_)} : std::string_view{};
}

char String::at(int index) const {
    if (index < 0 || index >= size_) {
        throw std::out_of_range("strset::String::at: the index is not in the string");
    }
    return chars_[index];
}

int String::compare(const String& other) const noexcept {
    const int common = size_ < other.size_ ? size_ : other.size_;
    if (common > 0) {
        // memcmp compares the bytes as unsigned char.
        const int order = std::memcmp(chars_, other.chars_, static_cast<std::size_t>(common));
        if (order != 0) {
            return order;
        }
    }
    return (size_ > other.size_) - (size_ < other.size_);
}

void swap(String& a, String& b) noexcept {
    std::swap(a.size_, b.size_);
    std::swap(a.chars_, b.chars_);
}

int first_difference(const String& a, const String& b) noexcept {
    const std::string_view x = a.view();
    const std::string_view y = b.view();
    std::size_t i = 0;
    while (i < x.size() && i < y.size() && x[i] == y[i]) {
        ++i;
    }
    return static_cast<int>(i);
}

std::ostream& operator<<(std::ostream& out, const String& value) { return out << value.view(); }

}  // namespace strset
