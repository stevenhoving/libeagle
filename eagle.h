#pragma once

#include "stream_buffer.h"

#include <string>
#include <vector>
#include <cstdint>

// Values are in tenths of a micrometre
constexpr auto u2mm(auto val)
{
    return val / 10.0 / 1000.0;
}

constexpr auto u2in(auto val)
{
    return val / 2.54 / 100.0 / 1000.0;
}

constexpr auto mm2u(auto val)
{
    return val * 10.0 * 1000.0;
}

constexpr auto in2u(auto val)
{
    return val * 254.0 * 1000.0;
}

// \todo rename to section_type
enum class section_types
{
    header = 0x10,
    unknown = 0x11,
    grid = 0x12,
    layer = 0x13,
    schema = 0x14,
    library = 0x15,
    // 0x16
    devices = 0x17,
    symbols = 0x18,
    packages = 0x19,
    schema_sheet = 0x1a,
    board = 0x1b,
    board_net = 0x1c,
    symbol = 0x1d,
    package = 0x1e,
    schema_net = 0x1f,
    path = 0x20,
    polygon = 0x21,
    line = 0x22,
    // 0x23
    arc = 0x24,
    circle = 0x25,
    rectangle = 0x26,
    junction = 0x27,
    hole = 0x28,
    via = 0x29,
    pad = 0x2a,
    smd = 0x2b,
    pin = 0x2c,
    gate = 0x2d,
    board_package = 0x2e,
    board_package2 = 0x2f,
    instance = 0x30,
    text = 0x31, // TextBaseSection
    // 0x32
    net_bus_label = 0x33, // TextBaseSections
    smashed_name = 0x34, // TextBaseSections
    smashed_value = 0x35, // TextBaseSections
    package_variant = 0x36,
    device = 0x37,
    part = 0x38,
    // 0x39
    schema_bus = 0x3a,
    // 0x3b
    variant_connections = 0x3c,
    schema_connection = 0x3d,
    board_connection = 0x3e,
    smashed_part = 0x3f,
    smashed_gate = 0x40,
    attribute = 0x41,
    attribute_value = 0x42,
    frame = 0x43,
    smashed_xref = 0x44
};

struct header_section
{
    section_types type;
    uint8_t unknown; // 0x80 // must be a flag or some sort...
    uint16_t subsecs;
    uint32_t numsecs;
    uint8_t major;
    uint8_t minor;

    std::vector<uint8_t> unkn1;
};

// \todo promote to namespace and rename eagle class to 'project (whatever)' or something.
class eagle
{
public:
    void parse(const std::vector<uint8_t>& data);


private:
    std::string read_string(stream_buffer& stream, int count);
    
    void parse_section(stream_buffer& stream);

    //void parse_device(stream_buffer& stream);
    //void parse_package_variant(stream_buffer& stream);

    header_section parse_header(stream_buffer& stream); //0x10,
    void parse_unknown(stream_buffer& stream); //0x11,
    void parse_grid(stream_buffer& stream); //0x12,
    void parse_layer(stream_buffer& stream); //0x13,
    void parse_schema(stream_buffer& stream); //0x14,
    void parse_library(stream_buffer& stream); //0x15,
    void parse_devices(stream_buffer& stream); //0x17,
    void parse_symbols(stream_buffer& stream); //0x18,
    void parse_packages(stream_buffer& stream); //0x19,
    void parse_schema_sheet(stream_buffer& stream); //0x1a,
    void parse_board(stream_buffer& stream); //0x1b,
    void parse_board_net(stream_buffer& stream); //0x1c,
    void parse_symbol(stream_buffer& stream); //0x1d,
    void parse_package(stream_buffer& stream); //0x1e,
    void parse_schema_net(stream_buffer& stream); //0x1f,
    void parse_path(stream_buffer& stream); //0x20,
    void parse_polygon(stream_buffer& stream); //0x21,
    void parse_line(stream_buffer& stream); //0x22,
    void parse_arc(stream_buffer& stream); //0x24,
    void parse_circle(stream_buffer& stream); //0x25,
    void parse_rectangle(stream_buffer& stream); //0x26,
    void parse_junction(stream_buffer& stream); //0x27,
    void parse_hole(stream_buffer& stream); //0x28,
    void parse_via(stream_buffer& stream); //0x29,
    void parse_pad(stream_buffer& stream); //0x2a,
    void parse_smd(stream_buffer& stream); //0x2b,
    void parse_pin(stream_buffer& stream); //0x2c,
    void parse_gate(stream_buffer& stream); //0x2d,
    void parse_board_package(stream_buffer& stream); //0x2e,
    void parse_board_package2(stream_buffer& stream); //0x2f,
    void parse_instance(stream_buffer& stream); //0x30,
    void parse_text(stream_buffer& stream); //0x31, // TextBaseSection
    void parse_net_bus_label(stream_buffer& stream); //0x33, // TextBaseSections
    void parse_smashed_name(stream_buffer& stream); //0x34, // TextBaseSections
    void parse_smashed_value(stream_buffer& stream); //0x35, // TextBaseSections
    void parse_package_variant(stream_buffer& stream); //0x36,
    void parse_device(stream_buffer& stream); //0x37,
    void parse_part(stream_buffer& stream); //0x38,
    void parse_schema_bus(stream_buffer& stream); //0x3a,
    void parse_variant_connections(stream_buffer& stream); //0x3c,
    void parse_schema_connection(stream_buffer& stream); //0x3d,
    void parse_board_connection(stream_buffer& stream); //0x3e,
    void parse_smashed_part(stream_buffer& stream); //0x3f,
    void parse_smashed_gate(stream_buffer& stream); //0x40,
    void parse_attribute(stream_buffer& stream); //0x41,
    void parse_attribute_value(stream_buffer& stream); //0x42,
    void parse_frame(stream_buffer& stream); //0x43,
    void parse_smashed_xref(stream_buffer& stream); //0x44

    

    std::vector<std::string> parse_string_table(stream_buffer& stream, size_t offset);

    std::vector<std::string> string_table_;
    size_t string_table_read_index_{ 0 };
};

