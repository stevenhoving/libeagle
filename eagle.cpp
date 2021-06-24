#include "eagle.h"
#include <vector>
#include <cstdint>
#include <format>
#include <iostream>
#include <cassert>
#include <algorithm>

void dump_hex(const std::vector<uint8_t>& data)
{
	for (const auto val : data)
		std::cout << std::format("{:02X}", val) << " ";
	std::cout << std::endl;
}

void dump_hex(const std::string& data)
{
    for (const auto val : data)
        std::cout << std::format("{:02X}", static_cast<uint8_t>(val)) << " ";
	std::cout << std::endl;
}

void dump_hex_ascii(const std::vector<uint8_t>& data)
{
	dump_hex(data);

	for (const auto val : data)
		if (val > 32 && val < 127)
			std::cout << std::format("{}", static_cast<char>(val)) << " ";
		else
			std::cout << ". ";
	std::cout << "\n";
}


void dump_dec_hex_ascii(const std::vector<uint8_t>& data)
{
	// \todo
	__debugbreak();

	for (const auto val : data)
		if (val > 32 && val < 127)
			std::cout << std::format("{:02X}", val) << " ";
		else
			std::cout << ". ";

    //print(' '.join('%03d' % (ord(byte), ) for byte in data))
    //	print(' '.join(' %02x' % (ord(byte), ) for byte in data))
    //	print(' '.join(' %s ' % (byte if 32 <= ord(byte) <= 127 else '.', ) for byte in data))
    //	print()
}

// simple string split function
std::vector<std::string> split(const std::string& str, const char character)
{
    if (std::empty(str))
        return {};

    size_t offset = 0;
    auto found = str.find(character, offset);
    if (found == std::string::npos)
        return { str };

    auto result = std::vector<std::string>{};
    for (;;)
    {
        result.push_back(str.substr(offset, found - offset));

        offset = found + 1;
        found = str.find(character, offset);
        if (found == std::string::npos)
            break;
    }

    if (offset < std::size(str))
        result.push_back(str.substr(offset, std::size(str) - offset));

    return result;
}



template<int ExpectedReadCount = 22>
class check_chunk
{
public:
	check_chunk(stream_buffer& stream)
		: stream_{stream}
		, offset_{stream_.tell()}
	{
	}

	~check_chunk()
	{
		const auto delta = stream_.tell() - offset_;
		if (delta != ExpectedReadCount)
			__debugbreak();
	}

	stream_buffer& stream_;
	size_t offset_;
};

header_section eagle::parse_header(stream_buffer& stream)
{
	const auto check = check_chunk<24>{ stream };

	const auto section_type = stream.read<uint8_t>(); // 0
	assert(section_type == 0x10);

	const auto unknown = stream.read<uint8_t>(); // 1

	const auto subsecs = stream.read<uint16_t>(); // 2
	const auto numsecs = stream.read<uint32_t>(); // 4
	const auto major = stream.read<uint8_t>(); // 8
	const auto minor = stream.read<uint8_t>(); // 9

	const auto unkn1 = stream.read<uint8_t>(14);
	// \todo unkn1 contains 2 or 3 fields that contain data. Most likely counts of some sort.

	std::cout << std::format("type: 0x{:x} unknown: {}, subsecs: {}, numsecs: {}, major: {}, minor: {}\n",
		section_type, unknown, subsecs, numsecs, major, minor);

	header_section result;
	result.type = static_cast<section_types>(section_type);
	result.unknown = unknown;
	result.subsecs = subsecs;
	result.numsecs = numsecs;
	result.major = major;
	result.minor = minor;
	result.unkn1 = unkn1;
	return result;
}

std::vector<std::string> eagle::parse_string_table(stream_buffer& stream, size_t offset)
{
	const auto current_position = stream.tell();
	stream.seek(static_cast<int>(offset), SEEK_SET);

	const auto unk = stream.read<uint32_t>(); // assert f.read(4) == '\x13\x12\x99\x19'
	// this is wierd....
	//assert(unk == 0x13129919);
	assert(unk == 0x19991213); // string table
	const auto size = stream.read<uint32_t>();

	const auto string_table = stream.read_string(size);
	stream.seek(static_cast<int>(current_position), SEEK_SET); // restore to current position

	auto string_table_data = split(string_table, '\0');

	string_table_data.pop_back();

	std::cout << "string table:\n";
	for (const auto& string : string_table_data)
		std::cout << "  " << string << '\n';

	return string_table_data;
}

void eagle::parse_unknown(stream_buffer& stream)
{
	const auto check = check_chunk{ stream };

    // this is really awesome.... unknown data... the file we are parsing contains 2x this one
    const auto unknown_data = stream.read<uint8_t>(22);
    dump_hex_ascii(unknown_data);
}

void eagle::parse_grid(stream_buffer& stream)
{
	const auto check = check_chunk{ stream };

	const auto flags = stream.read<uint8_t>(); // 2
	const auto display = flags & 0x01;
	const auto style = (flags & 0x02) >> 1;

	const auto units_flags = stream.read<uint8_t>(); // 3
	const auto units = units_flags & 0x0F;
	const auto altunit = (units_flags & 0xF0) >> 4;

	//const auto multiple = static_cast<uint32_t>(stream.read<uint16_t>()) << 8 | stream.read<uint8_t>(); // 4
	const auto multiple = stream.read<uint32_t>(); // this is fuckedup... in the pyeagle this is read as a 24 bit number
	const auto size = stream.read<double>();
	const auto altsize = stream.read<double>();

    std::cout << std::format("grid - display: {}, style: {}, units: {}, altunit: {}, multiple: {}, size: {}, altsize: {}\n",
		display, style, units, altunit, multiple, size, altsize);
}

/*
layer_fills = dict(enumerate('stroke fill horiz thinslash thickslash thickback thinback square diamond dither coarse fine bottomleft bottomright topright topleft'.split()))
layer_colors = dict(enumerate('black darkblue darkgreen darkcyan darkred darkpurple darkyellow grey darkgrey blue green cyan red purple yellow lightgrey'.split()))
*/
void eagle::parse_layer(stream_buffer& stream)
{
	const auto check = check_chunk{ stream };

	const auto flags = stream.read<uint8_t>();

	// these flags are really fucked up. Visible for example exists of 2 flags (4 and 8).
	const auto side = flags & 0x10;
	const auto visible = flags & 0xc; // whether objects on this layer are currently shown
	const auto available = flags & 0x02; // not available = > not visible in layer display dialog at all

	const auto layer = stream.read<uint8_t>(); // layer id
	const auto other = stream.read<uint8_t>(); // the number of the matching layer on the other side

	const auto fill = stream.read<uint8_t>() & 0x0F; // \todo figure out what that other bits mean
	const auto color = stream.read<uint8_t>() & 0x3f; // \todo same weird shit
	const auto unknown = stream.read<uint8_t>(); // this one is unknown
	const auto zeros = stream.read<uint8_t>(7); // this one is known to be zero... why would you have a file format that does this.
	const auto name = stream.read_string(9);

	for (const auto zero : zeros)
		assert(zero == 0);

    std::cout << std::format("layer - flags: {}, layer: {}, other: {}, fill: {}, color: {}, unknown: {}, name: {}\n",
		flags, layer, other, fill, color, unknown, name);
}

void eagle::parse_schema(stream_buffer& stream)
{
	const auto check = check_chunk{ stream };

	const auto unknown = stream.read<uint8_t>();
	const auto zero = stream.read<uint8_t>();
	const auto libsubsecs = stream.read<uint32_t>();
	const auto shtsubsecs = stream.read<uint32_t>();

	// version 5 or higher \todo add version check
	const auto atrsubsecs = stream.read<uint32_t>();
	const auto zeros = stream.read<uint8_t>(3);
	const auto xref_format = stream.read_string(5);

    std::cout << std::format("schema - unknown: {}, zero: {}, libsubsecs: {}, shtsubsecs: {}, atrsubsecs: {}, xref_format: {}\n",
		unknown, zero, libsubsecs, shtsubsecs, atrsubsecs, xref_format);
}

void eagle::parse_library(stream_buffer& stream)
{
	const auto check = check_chunk{ stream };

	const auto zeros = stream.read<uint8_t>(2);

	const auto devsubsecs = stream.read<uint32_t>();
	const auto symsubsecs = stream.read<uint32_t>();
	const auto pacsubsecs = stream.read<uint32_t>();
	const auto name = stream.read_string(8);

    std::cout << std::format("library - devsubsecs: {}, symsubsecs: {}, pacsubsecs: {}, name: {}\n",
		devsubsecs, symsubsecs, pacsubsecs, name);
}

void eagle::parse_devices(stream_buffer& stream)
{
	const auto check = check_chunk{ stream };

	const auto zeros = stream.read<uint8_t>(2);
	const auto subsecs = stream.read<uint32_t>();
	const auto children = stream.read<uint32_t>(); // childeren count
	const auto unknown = stream.read<uint32_t>();
	const auto libname = stream.read_string(8);

    std::cout << std::format("devices - subsecs: {}, children: {}, unknown: {}, libname: {}\n",
		subsecs, children, unknown, libname);
}

void eagle::parse_symbols(stream_buffer& stream)
{
	const auto check = check_chunk{ stream };

	const auto zero = stream.read<uint16_t>();
	assert(zero == 0);
	const auto subsecs = stream.read<uint32_t>();
	const auto children = stream.read<uint32_t>();
	const auto zero2 = stream.read<uint32_t>();
	assert(zero2 == 0);
	const auto libname = stream.read_string(8);

    std::cout << std::format("symbols - subsecs: {}, children: {}, libname: {}\n",
        subsecs, children, libname);

	// parse subsections
	for (uint32_t i = 0; i < subsecs; ++i)
	{
        const auto type = static_cast<section_types>(stream.read<uint8_t>()); // 0
        const auto unknown = stream.read<uint8_t>(); // 1

        std::cout << std::format("type: 0x{:x}\n", static_cast<uint8_t>(type));
		assert(type == section_types::symbol);

		parse_symbol(stream);
	}
}

void eagle::parse_packages(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_schema_sheet(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_board(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_board_net(stream_buffer& stream) { const auto check = check_chunk{ stream }; }

void eagle::parse_symbol(stream_buffer& stream)
{
	const auto check = check_chunk{ stream };

	const auto subsecs = stream.read<uint16_t>();
	const auto minx = stream.read<int16_t>();
	const auto miny = stream.read<int16_t>();
	const auto maxx = stream.read<int16_t>();
	const auto maxy = stream.read<int16_t>();
    const auto zero = stream.read<int32_t>();
	assert(zero == zero);
	const auto name = stream.read_string(8);
    //const auto subsec_counts = [self.subsecs]

    std::cout << std::format("symbol - subsecs: {}, minx: {}, miny: {}, maxx: {}, maxy: {}, name: {}\n",
        subsecs, minx, miny, maxx, maxy, name);

	for (uint16_t i = 0; i < subsecs; ++i)
	{
		// this should be parse_drawables... or whatever...
		parse_section(stream);
	}
}

void eagle::parse_package(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_schema_net(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_path(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_polygon(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_line(stream_buffer& stream) { const auto check = check_chunk{ stream }; }

void eagle::parse_arc(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_circle(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_rectangle(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_junction(stream_buffer& stream) { const auto check = check_chunk{ stream }; }



void eagle::parse_hole(stream_buffer& stream)
{
	const auto check = check_chunk{ stream };

	const auto zero = stream.read<uint16_t>();
    const auto x = stream.read<int32_t>();
    const auto y = stream.read<int32_t>();
	const auto width_2 = stream.read<uint32_t>(); // not width but radius
	const auto unknown = stream.read<uint16_t>();
	assert(unknown == 0);

	const auto zeros2 = stream.read<uint8_t>(6);
	assert(std::all_of(begin(zeros2), end(zeros2), [](auto v) {return v == 0; }));

    std::cout << std::format("hole - zero: {}, x: {}, y: {}, radius: {}, unknown: {}\n",
		zero, x, y, width_2, unknown);
}

void eagle::parse_via(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_pad(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_smd(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_pin(stream_buffer& stream) 
{ 
	const auto check = check_chunk{ stream };

	const auto flags = stream.read<uint8_t>();
	const auto function = flags & 0x03;
	const auto visible = (flags & 0xC0) >> 6;
	const auto zero_flags = (flags & 0x3c);
	assert(zero_flags == 0);
    
	const auto zero = stream.read<uint8_t>();
	assert(zero == 0);

	const auto x = stream.read<int32_t>();
	const auto y = stream.read<int32_t>();
	const auto orientation_flags = stream.read<uint8_t>();
	const auto direction = orientation_flags & 0x0f;
	const auto length = (orientation_flags & 0x30) >> 4;
	const auto angle = (orientation_flags & 0xc0) << 4;
	const auto swaplevel = stream.read<uint8_t>();
	const auto name = stream.read_string(10);

    std::cout << std::format("pin - flags: {}, x: {}, y: {}, orientation_flagsteams: {}, swaplevel: {}, name: {}\n",
		flags, x, y, orientation_flags, swaplevel, name);
}

void eagle::parse_gate(stream_buffer& stream)
{
	const auto check = check_chunk{ stream };

	const auto zero = stream.read<uint16_t>();
	const auto x = stream.read<int32_t>();
	const auto y = stream.read<int32_t>();
	const auto addlevel = stream.read<uint8_t>();
	const auto swap = stream.read<uint8_t>();
	const auto symno = stream.read<uint16_t>();
	const auto name = stream.read_string(8);

    std::cout << std::format("gage - zero: {}, x: {}, y: {}, addlevel: {}, swap: {}, symno: {}, name: {}\n",
        zero, x, y, addlevel, swap, symno, name);
}
void eagle::parse_board_package(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_board_package2(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_instance(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_text(stream_buffer& stream)
{
	const auto check = check_chunk{ stream };

	const auto font_flags = stream.read<uint8_t>();
	const auto font = font_flags & 0x03;
	const auto zero_flags = font_flags & 0xFC;
	assert(zero_flags == 0);

	const auto layer = stream.read<uint8_t>();
	const auto x = stream.read<int32_t>();
	const auto y = stream.read<int32_t>();
	const auto size_2 = stream.read<uint16_t>();
	const auto ratio_flags = stream.read<uint8_t>();
	const auto ratio = (ratio_flags & 0x7c) >> 2;
	assert((ratio_flags & 0x03) == 0);
	const auto ratio_unknown = ratio_flags & 0x80;

	const auto unknown = stream.read<uint8_t>();
	const auto orientation_field = stream.read<uint16_t>();
	const auto angle = orientation_field & 0x0fff;
	const auto mirrored = (orientation_field & 0x1000) != 0;
	const auto spin = (orientation_field & 0x4000) != 0;
	assert((orientation_field & 0xa000) == 0);

	//if self.sectype in (0x31, 0x41):
	const auto text = read_string(stream, 6);
	//else:
		//stream.read<bytes(18, 6)
		//self.text = ''

    std::cout << std::format("text - font_flags: {}, layer: {}, x: {}, y: {}, size_2: {}, ratio_flags: {}, unknown: {}, orientation_field: {}, text: {}\n",
		font_flags, layer, x, y, size_2, ratio_flags, unknown, orientation_field, text);
}

void eagle::parse_net_bus_label(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_smashed_name(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_smashed_value(stream_buffer& stream) { const auto check = check_chunk{ stream }; }

void eagle::parse_package_variant(stream_buffer& stream)
{
	const auto check = check_chunk{ stream };

	const auto subsecs = stream.read<uint16_t>();
	const auto pacno = stream.read<uint16_t>();
	const auto table = read_string(stream, 13);
    const auto name = read_string(stream, 5);

    std::cout << std::format("package variant - subsecs: {}, pacno: {}, name: {}, table: {}\n",
        subsecs, pacno, name, table);
}

std::string eagle::read_string(stream_buffer& stream, int count)
{
	const auto string = stream.read_string(count);
	if (std::empty(string))
		return string;

	if (string[0] == 0x7f)
		return string_table_.at(string_table_read_index_++);

	return string;
}

void eagle::parse_device(stream_buffer& stream)
{
	const auto check = check_chunk{ stream };

	const auto gatsubsecs = stream.read<uint16_t>();
	const auto varsubsecs = stream.read<uint16_t>();

	const auto flags = stream.read<uint8_t>();
	const auto value_on = (flags & 0x01) != 0;
	const auto unknown = (flags & 0x02) != 0;
	const auto zero_flags = (flags & 0xfc) != 0; // should be zero
	assert(zero_flags == 0);

	const auto connection_pins = stream.read<uint8_t>();

	// \todo is this correct? it seems to me that this should have been 0xf0
	const auto con_byte = (connection_pins & 0x80) >> 7;
	assert(con_byte == 1);

	const auto pin_bits = (connection_pins & 0x0f);
    //const auto _get_zero_mask(7, 0x70)
	const auto zero_flags2 = connection_pins & 0x70;
	assert(zero_flags2 == 0);

	const auto prefix = read_string(stream, 5);
    const auto desc = read_string(stream, 5);
	const auto name = read_string(stream, 6);

	//dump_hex(name);
	//dump_hex(desc);
	//dump_hex(prefix);

    std::cout << std::format("device - gatsubsecs: {}, varsubsecs: {}, flags: {}, connection_pins: {}, name: {}, desc: {}, prefix: {}\n",
		gatsubsecs, varsubsecs, flags, connection_pins, name, desc, prefix);
}

void eagle::parse_part(stream_buffer& stream) { const auto check = check_chunk{ stream }; }

void eagle::parse_schema_bus(stream_buffer& stream) { const auto check = check_chunk{ stream }; }

void eagle::parse_variant_connections(stream_buffer& stream)
{
	const auto check = check_chunk{ stream };

	// \todo if parent con_byte is 0 then we only have 11 bytes to parse
	const auto connections_data = stream.read<uint8_t>(22);
	// \todo parent has knowledge how many bits are used for pin nr encoding 'pin_bits'

	std::cout << std::format("variant connections\n");

	dump_hex(connections_data);
}

void eagle::parse_schema_connection(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_board_connection(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_smashed_part(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_smashed_gate(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_attribute(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_attribute_value(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_frame(stream_buffer& stream) { const auto check = check_chunk{ stream }; }
void eagle::parse_smashed_xref(stream_buffer& stream) { const auto check = check_chunk{ stream }; }

void eagle::parse_section(stream_buffer& stream)
{
    const auto type = static_cast<section_types>(stream.read<uint8_t>()); // 0
    const auto unknown = stream.read<uint8_t>(); // 1

	std::cout << std::format("type: 0x{:x}\n", static_cast<uint8_t>(type));
	switch (type)
	{
	case section_types::header:
		__debugbreak(); // at this point we don't expect a header section type.
		break;
	case section_types::unknown: eagle::parse_unknown(stream); break;
	case section_types::grid: eagle::parse_grid(stream); break;
	case section_types::layer: eagle::parse_layer(stream); break;
	case section_types::schema: eagle::parse_schema(stream); break;
	case section_types::library: eagle::parse_library(stream); break;

	case section_types::devices: eagle::parse_devices(stream);  break;
	case section_types::symbols: eagle::parse_symbols(stream); break;
	case section_types::packages: eagle::parse_packages(stream); break;
	case section_types::schema_sheet: eagle::parse_schema_sheet(stream); break;
	case section_types::board: eagle::parse_board(stream); break;
	case section_types::board_net: eagle::parse_board_net(stream); break;
	case section_types::symbol: eagle::parse_symbol(stream); break;
	case section_types::package: eagle::parse_package(stream); break;
	case section_types::schema_net: eagle::parse_schema_net(stream); break;
	case section_types::path: eagle::parse_path(stream); break;
	case section_types::polygon: eagle::parse_polygon(stream); break;
	case section_types::line: eagle::parse_line(stream); break;

	case section_types::arc: eagle::parse_arc(stream); break;
	case section_types::circle: eagle::parse_circle(stream); break;
	case section_types::rectangle: eagle::parse_rectangle(stream); break;
	case section_types::junction: eagle::parse_junction(stream); break;
	case section_types::hole: eagle::parse_hole(stream); break;
	case section_types::via: eagle::parse_via(stream); break;
    case section_types::pad: eagle::parse_pad(stream); break;
    case section_types::smd: eagle::parse_smd(stream); break;
    case section_types::pin: eagle::parse_pin(stream); break;
	case section_types::gate: eagle::parse_gate(stream); break;
    case section_types::board_package: eagle::parse_board_package(stream); break;
	case section_types::board_package2: eagle::parse_board_package2(stream); break;
	case section_types::instance: eagle::parse_instance(stream); break;
	case section_types::text: eagle::parse_text(stream); break;

	case section_types::net_bus_label: eagle::parse_net_bus_label(stream); break;
	case section_types::smashed_name: eagle::parse_smashed_name(stream); break;
	case section_types::smashed_value: eagle::parse_smashed_value(stream); break;
	case section_types::package_variant: eagle::parse_package_variant(stream); break;
	case section_types::device: eagle::parse_device(stream); break;
	case section_types::part: eagle::parse_part(stream); break;

	case section_types::schema_bus: eagle::parse_schema_bus(stream); break;

	case section_types::variant_connections: eagle::parse_variant_connections(stream); break;
	case section_types::schema_connection: eagle::parse_schema_connection(stream); break;
	case section_types::board_connection: eagle::parse_board_connection(stream); break;
	case section_types::smashed_part: eagle::parse_smashed_part(stream); break;
	case section_types::smashed_gate: eagle::parse_smashed_gate(stream); break;
	case section_types::attribute: eagle::parse_attribute(stream); break;
	case section_types::attribute_value: eagle::parse_attribute_value(stream); break;
	case section_types::frame: eagle::parse_frame(stream); break;
	case section_types::smashed_xref: eagle::parse_smashed_xref(stream); break;

	default:
		__debugbreak();
		break;
	}
}

void eagle::parse(const std::vector<uint8_t>& data)
{
	auto stream = stream_buffer(data);
	const auto offset_start = stream.tell();
	const auto header = parse_header(stream);
	// make sure we read 24 bytes
	assert(stream.tell() - offset_start == 24);

	// calculate the string table offset
	const auto end_offset = header.numsecs * 24;

	// \todo figure out if we can move this to after parsing all the sections
	string_table_ = eagle::parse_string_table(stream, end_offset);

	for (uint32_t section = 0; section < header.numsecs; ++section)
	{
		parse_section(stream);
	}
}

