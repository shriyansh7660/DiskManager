#include <iostream>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;
//this is a new comment

// Function to display disk space information
void displayDiskSpaceInfo() {
    fs::space_info spaceInfo = fs::space(fs::current_path());
    std::cout << "Total Disk Space: " << spaceInfo.capacity << " bytes\n";
    std::cout << "Free Disk Space: " << spaceInfo.free << " bytes\n";
    std::cout << "Used Disk Space: " << spaceInfo.capacity - spaceInfo.free << " bytes\n";
}

// Function to detect duplicate files using MD5 hashing
std::unordered_map<std::string, std::vector<std::string>> findDuplicateFiles(const fs::path& rootPath) {
    std::unordered_map<std::string, std::vector<std::string>> duplicateFiles;
    
    for (const auto& entry : fs::recursive_directory_iterator(rootPath)) {
        if (fs::is_regular_file(entry)) {
            std::ifstream file(entry.path().string(), std::ios::binary);
            if (!file) {
                // Handle error opening file
                continue;
            }
            
            std::string contentHash;
            // Compute MD5 hash of file content and store it in contentHash
            // (You'll need to use a library or implement the hashing algorithm)
            
            duplicateFiles[contentHash].push_back(entry.path().string());
        }
    }
    
    // Remove entries with a single file (no duplicates)
    for (auto it = duplicateFiles.begin(); it != duplicateFiles.end();) {
        if (it->second.size() < 2) {
            it = duplicateFiles.erase(it);
        } else {
            ++it;
        }
    }
    
    return duplicateFiles;
}

// Function to identify large files
std::vector<fs::path> findLargeFiles(const fs::path& rootPath, uintmax_t minSize) {
    std::vector<fs::path> largeFiles;
    
    for (const auto& entry : fs::recursive_directory_iterator(rootPath)) {
        if (fs::is_regular_file(entry) && fs::file_size(entry) >= minSize) {
            largeFiles.push_back(entry.path());
        }
    }
    
    return largeFiles;
}

int main() {
    fs::path rootPath = fs::current_path();
    displayDiskSpaceInfo();

    std::cout << "\nFinding duplicate files...\n";
    std::unordered_map<std::string, std::vector<std::string>> duplicateFiles = findDuplicateFiles(rootPath);
    for (const auto& [hash, files] : duplicateFiles) {
        std::cout << "Duplicate files with hash " << hash << ":\n";
        for (const auto& file : files) {
            std::cout << file << '\n';
        }
    }

    std::cout << "\nFinding large files...\n";
    uintmax_t minSize = 1000000; // Example minimum size for large files (1 MB)
    std::vector<fs::path> largeFiles = findLargeFiles(rootPath, minSize);
    for (const auto& file : largeFiles) {
        std::cout << "Large file: " << file << " (Size: " << fs::file_size(file) << " bytes)\n";
    }

    return 0;
}
