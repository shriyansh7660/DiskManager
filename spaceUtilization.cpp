#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

// Data structure to hold file type information
struct FileType {
    std::string extension;
    unsigned long long size;
};

// Function to traverse directories and accumulate space utilization by file types
void traverse_directories(const std::string& directory, std::vector<FileType>& file_types) {
    try {
        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            try {
                if (fs::is_regular_file(entry)) {
                    std::string extension = entry.path().extension().string();
                    // Convert extension to lowercase for case-insensitivity
                    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                    // Add other file type checks based on your specific use case
                    // For example, checking for image file extensions like .jpg, .png, etc.

                    // Accumulate space utilization by file type
                    auto it = std::find_if(file_types.begin(), file_types.end(),
                        [&extension](const FileType& ft) { return ft.extension == extension; });

                    if (it != file_types.end()) {
                        it->size += fs::file_size(entry);
                    } else {
                        FileType ft;
                        ft.extension = extension;
                        ft.size = fs::file_size(entry);
                        file_types.push_back(ft);
                    }
                }
            } catch (const std::exception& e) {
                // Handle exceptions while accessing individual files, if needed
                std::cerr << "Error while accessing file: " << entry.path() << ": " << e.what() << '\n';
            }
        }
    } catch (const std::exception& e) {
        // Handle the exception while traversing directories, e.g., print an error message
        std::cerr << "Error while traversing directories: " << directory<< " "<< e.what() << '\n';
    }
}
bool sortBySize(const FileType& a, const FileType& b) {
    return a.size > b.size;
}
int main() {
    std::vector<FileType> file_types;
    std::string base_directory = "C:\\"; // or any other drive or directory

    traverse_directories(base_directory, file_types);

       std::sort(file_types.begin(), file_types.end(), sortBySize);

    // Display the breakdown of space utilization
    std::cout << "Space Utilization Breakdown:\n";
    for (const auto& ft : file_types) {
        std::cout << "File Type: " << ft.extension << ", Size: " << ft.size << " bytes\n";
    }

    return 0;
}
