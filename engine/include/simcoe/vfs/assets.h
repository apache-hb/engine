#pragma once

#include <filesystem>
#include <fstream>

namespace simcoe::assets {
    struct Manager {
        Manager(const std::filesystem::path& root) : root(root) {}

        template<typename T>
        std::vector<T> loadBlob(const std::filesystem::path& path) {
            std::ifstream file(root / path, std::ios::binary);
            if (!file.is_open()) {
                return {};
            }

            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            file.seekg(0, std::ios::beg);

            std::vector<T> data(size / sizeof(T));
            file.read(reinterpret_cast<char*>(data.data()), size);
            file.close();

            return data;
        }

        std::string loadText(const std::filesystem::path& path) {
            std::ifstream file(root / path);
            if (!file.is_open()) {
                return {};
            }

            std::string data;
            file.seekg(0, std::ios::end);
            data.reserve(file.tellg());
            file.seekg(0, std::ios::beg);

            data.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            return data;
        }

    private:
        std::filesystem::path root;
    };
}
