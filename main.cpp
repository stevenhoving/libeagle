#include "eagle.h"
#include <string>
#include <cstdio>

std::vector<uint8_t> read_data(const std::string& path)
{
    std::vector<uint8_t> result;
    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp)
    {
        printf("unable to open: %s\n", path.c_str());
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    auto fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    result.resize(fsize + 1);
    fread(&result[0], fsize, 1, fp);
    fclose(fp);

    return result;
}

int main(int argc, const char* argv[])
{
    const auto data = read_data("D:/dev/eagle/test/intertechnik/Stecker.lbr");
    eagle file;
    file.parse(data);
    return 0;
}