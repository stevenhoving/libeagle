#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <limits>
#include <stdexcept>
#include <type_traits>

// read and write buffer...
class stream_buffer
{
    using value_type = std::vector<uint8_t>;
    using size_type = value_type::size_type;
public:
    stream_buffer() = default;

    explicit stream_buffer(std::vector<uint8_t> data)
        : data_(std::move(data))
    {
    }

    stream_buffer(const stream_buffer& other) = default;
    stream_buffer& operator=(const stream_buffer & other) = default;

    stream_buffer(stream_buffer &&other) = default;
    stream_buffer &operator=(stream_buffer &&other) = default;

    template<typename T>
    T read() const
    {
        // limit to float/int types for now
        static_assert(std::is_arithmetic_v<T>);

        T value;
        std::memcpy(&value, &data_[index_], sizeof(value));
        index_ += sizeof(value);
        return value;
    }

    template<typename T>
    std::vector<T> read(int count) const
    {
        // limit to float/int types for now
        static_assert(std::is_arithmetic_v<T>);

        std::vector<T> value;
        value.resize(count);

        std::memcpy(std::data(value), &data_[index_], sizeof(T) * count);
        index_ += sizeof(T) * count;
        return value;
    }

    std::string read_string(int count) const
    {
        std::string value(count, '\0');

        std::memcpy(std::data(value), &data_[index_], sizeof(char) * count);
        index_ += sizeof(char) * count;
        return value;
    }

    uint8_t peek_u8() const
    {
        return data_[index_];
    }

    void write(const uint8_t value) const
    {
        prep_write_(sizeof(value));
        data_[index_++] = value;
    }

    void write(const uint16_t value) const
    {
        prep_write_(sizeof(value));
        data_[index_++] = value & 0xFF;
        data_[index_++] = (value >> 8) & 0xFF;
    }

    void write(const value_type &data) const
    {
        // \todo should we throw a exception, or is it a warning?
        if (data.empty())
        {
            printf("Warning, we are attempting to write a empty buffer to the stream\n");
            return;
        }

        prep_write_(static_cast<const int>(data.size()));
        auto dst = &data_[index_];
        memmove(dst, data.data(), data.size());
        index_ += static_cast<int32_t>(data.size());
    }

    void prep_write_(const int len) const
    {
        // \todo overflow/underflow protection.
        const auto total_length = data_.size() + len;
        if (total_length >= std::numeric_limits<size_type>::max())
            throw std::runtime_error("Unable to resize stream buffer beyond 2^32");

        if (total_length > data_.size())
        {
            // \todo when we write a string/vector or buffer we should use insert
            // instead of doing this + memmove/memcpy
            data_.resize(static_cast<size_type>(total_length));
        }
    }

    bool empty() const noexcept
    {
        return data_.empty();
    }

    // \todo fix integer math issues (int = int + int).
    void seek(const int offset, const int origin) const
    {
        int32_t new_index = 0;
        switch (origin)
        {
        case SEEK_SET:
            new_index = offset;
            break;
        case SEEK_CUR:
            new_index = index_ + offset;
            break;
        case SEEK_END:
            new_index = (static_cast<int32_t>(data_.size()) - 1) - offset;
            break;
        default:
            throw std::runtime_error("Invalid seek origin value");
            break;
        }

        if (new_index < 0)
            throw std::runtime_error("Invalid seek new index, negative");

        if (static_cast<size_type>(new_index) > data_.size())
            throw std::runtime_error("Invalid seek new index, to big");

        index_ = new_index;
    }

    uint8_t *data() noexcept
    {
        return data_.data();
    }

    const uint8_t *data() const noexcept
    {
        return std::data(data_);
    }

    size_type tell() const noexcept
    {
        return index_;
    }

    size_type size() const noexcept
    {
        return std::size(data_);
    }

    mutable value_type data_ = {};
    mutable int32_t index_ = 0;
};
