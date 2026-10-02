import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / 'src/tools/vmap4_extractor'

MPQ_TRANSPORT = r'''
#ifndef MPQ_H
#define MPQ_H
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

class MPQFile
{
public:
    explicit MPQFile(char const* filename)
    {
        std::ifstream input(filename, std::ios::binary);
        bytes.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    }

    bool isEof() const { return position >= bytes.size(); }

    std::size_t read(void* destination, std::size_t requested)
    {
        std::size_t available = std::min(requested, bytes.size() - position);
        if (available)
            std::memcpy(destination, bytes.data() + position, available);
        position += available;
        return available;
    }

    void close() {}

private:
    std::vector<char> bytes;
    std::size_t position = 0;
};
#endif
'''

CASES = r'''
#include "dbcfile.h"
#include <cstring>
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 3)
        return 2;

    DBCFile file(argv[1]);
    bool packed = std::strcmp(argv[2], "packed") == 0;
    bool expected = packed || std::strcmp(argv[2], "valid") == 0;
    if (file.open() != expected)
    {
        std::cerr << "DBCFile::open returned the wrong acceptance result\n";
        return 1;
    }

    if (!expected)
        return 0;

    if (packed)
    {
        if (file.getRecordCount() != 1 || file.getFieldCount() != 2)
            return 1;
        auto record = file.getRecord(0);
        return record.getByte(0) == 1 && record.getByte(1) == 2 ? 0 : 1;
    }

    if (file.getRecordCount() != 2 || file.getFieldCount() != 2)
        return 1;

    auto first = file.getRecord(0);
    auto second = file.getRecord(1);
    if (first.getUInt(0) != 41 || std::strcmp(first.getString(1), "a.m2") != 0)
        return 1;
    if (second.getUInt(0) != 42 || std::strcmp(second.getString(1), "b.wmo") != 0)
        return 1;
    return 0;
}
'''


def main():
    compiler = shutil.which(os.environ.get('CXX', 'c++'))
    if not compiler:
        raise RuntimeError('A C++17 compiler is required')
    strings = b'\0a.m2\0b.wmo\0'
    valid = b'WDBC' + struct.pack('<IIII', 2, 2, 8, len(strings))
    valid += struct.pack('<IIII', 41, 1, 42, 6) + strings
    packed = b'WDBC' + struct.pack('<IIII', 1, 2, 2, 1) + b'\x01\x02\0'
    cases = [('complete', valid, 'valid'), ('byte-fields', packed, 'packed'),
             ('missing-string-terminator', valid[:-1], 'invalid')]
    cases += [(f'truncated-payload-{length}', valid[:length], 'invalid') for length in range(20, len(valid) - 1)]
    cases += [(f'truncated-header-{length}', valid[:length], 'invalid') for length in range(20)]
    cases += [('invalid-magic', b'BAD!' + valid[4:], 'invalid')]
    with tempfile.TemporaryDirectory(prefix='coa-vmap-dbc-') as directory:
        out = Path(directory)
        for name in ('dbcfile.cpp', 'dbcfile.h'):
            shutil.copyfile(SOURCE / name, out / name)
        (out / 'mpq_libmpq04.h').write_text(MPQ_TRANSPORT)
        (out / 'main.cpp').write_text(CASES)
        executable = out / 'test'
        subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', str(out / 'dbcfile.cpp'),
                        str(out / 'main.cpp'), '-o', str(executable)], check=True)
        for name, data, expected in cases:
            fixture = out / f'{name}.dbc'
            fixture.write_bytes(data)
            subprocess.run([str(executable), str(fixture), expected], check=True)
    print(f'vmap_dbc: {len(cases)} cases passed using the production DBCFile implementation')


if __name__ == '__main__':
    main()
