#ifndef REPLY_FS_STREAM_H_
#define REPLY_FS_STREAM_H_

#include <ios>
#include <string>
#include <vector>
#include "flash_mem_manager.h"

namespace fcpp {

namespace common {

class reply_fs_stream {

public:
    reply_fs_stream(std::string &file_name, std::ios_base::openmode mode) : f_name(file_name), m_mode(mode), read_pointer(0) {
        if (!initialized) {
            init_flash_mem_manager();
            initialized = true;
        }
        if (mode == std::ios_base::in) {
            int fd = open_file(file_name.c_str(), file_name.size(), READ_MODE);
                if (fd != -1) {
                char b[256];
                int len = read_round(b, 256);
                if (len != -1) {
                    buffer.clear();
                    buffer.insert(buffer.end(), b, b + len);
                    if (check_crc16(buffer)) {
                        buffer.pop_back();
                        buffer.pop_back();
                    } else {
                        printf("WRONG CRC while reading storage!\n");
                        buffer.clear();
                    }
                }
                close_file(fd);
            }
        }
    }

    ~reply_fs_stream() {
        if (m_mode == std::ios_base::out) {
            int fd = open_file(f_name.c_str(), f_name.size(), WRITE_MODE);
            if (fd != -1) {
                uint16_t crc = crc16(buffer);
                buffer.push_back((crc >> 8) & 0xFF);
                buffer.push_back(crc & 0xFF);
                write_round(buffer.data(), buffer.size());
                close_file(fd);
            }
        }
    }

    reply_fs_stream& operator<<(char c) {
        buffer.push_back(c);
        return *this;
    }

    reply_fs_stream& operator>>(char &c) {
        if (read_pointer < buffer.size()) {
            c = buffer[read_pointer];
        }
        read_pointer++;
        return *this;
    }

    operator bool() {
        return read_pointer <= buffer.size();
    }

    static uint16_t crc16(const std::vector<char> &buffer) {
        uint16_t wCrc = 0xffff;
        uint16_t i;
        uint8_t j;
        for (i = 0; i < buffer.size(); ++i)
        {
            wCrc ^= buffer[i] << 8;
            for (j = 0; j < 8; ++j)
            {
            wCrc = (wCrc & 0x8000) > 0 ? (wCrc << 1) ^ 0x1021 : wCrc << 1;
            }
        }
        return wCrc & 0xffff;
    }

    static bool check_crc16(const std::vector<char> &buffer) {
        std::vector<char> data = buffer;
        data.pop_back();
        data.pop_back();
        uint16_t crc16_calculated;
        uint16_t crc16_received;
        bool result_b = false;
        crc16_received = ((uint16_t)buffer[data.size()] << 8) + (uint16_t)buffer[data.size() + 1];
        crc16_calculated = crc16(data);
        if (crc16_calculated == crc16_received) {
            result_b = true;
        } else {
            result_b = false;
        }
        return result_b;
    }



private:
    std::vector<char> buffer;
    std::string f_name;
    std::ios_base::openmode m_mode;
    uint read_pointer;

    static bool initialized;
};

}


}

#endif // REPLY_FS_STREAM_H_