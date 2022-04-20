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
                char b[350];
                int len = read_round(b, 350);
                if (len != -1) {
                    buffer.clear();
                    buffer.insert(buffer.end(), b, b + len);
                }
                close_file(fd);
            }
        }
    }

    ~reply_fs_stream() {
        if (m_mode == std::ios_base::out) {
            int fd = open_file(f_name.c_str(), f_name.size(), WRITE_MODE);
            if (fd != -1) {
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