#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

enum class FileType {
    Unknown,
    Video,
    Image,
    Document,
    // Add more file types as needed
};

std::unordered_map<std::string, FileType> fileTypeMap = {
    {".mp4", FileType::Video},
    {".avi", FileType::Video},
    {".jpg", FileType::Image},
    {".png", FileType::Image},
    {".docx", FileType::Document},
    // Add more mappings for other file types
};

// Function to categorize files based on their extensions
FileType categorizeFile(const fs::path& filePath) {
    auto extension = filePath.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower); // Convert extension to lowercase
    if (fileTypeMap.find(extension) != fileTypeMap.end()) {
        return fileTypeMap[extension];
    }
    return FileType::Unknown;
}

// Function to get the name of the file type as a string
std::string getFileTypeName(FileType type) {
    switch (type) {
        case FileType::Video:
            return "Video";
        case FileType::Image:
            return "Image";
        case FileType::Document:
            return "Document";
        // Add more cases for other file types
        default:
            return "Unknown";
    }
}

// Custom recursive directory traversal function
void traverseDirectory(const fs::path& dirPath, const std::vector<FileType>& fileTypesToScan, std::unordered_map<FileType, uintmax_t>& fileTypeUsage, std::vector<std::string>& inaccessibleDirs) {
    try {
        for (const auto& entry : fs::directory_iterator(dirPath)) {
            if (fs::is_regular_file(entry)) {
                FileType fileType = categorizeFile(entry.path());
                if (fileType != FileType::Unknown && std::find(fileTypesToScan.begin(), fileTypesToScan.end(), fileType) != fileTypesToScan.end()) {
                    fileTypeUsage[fileType] += fs::file_size(entry.path());
                }
            } else if (fs::is_directory(entry)) {
                traverseDirectory(entry.path(), fileTypesToScan, fileTypeUsage, inaccessibleDirs);
            }
        }
    } catch (const fs::filesystem_error& e) {
        // Handle the error while iterating through directories
       // std::cerr << "Error accessing file/directory: " << " - " << e.what() << "\n";
      //  std::cerr << "Error accessing file/directory: " << dirPath.string() << " - " << e.what() << "\n";
        inaccessibleDirs.push_back(dirPath.string());
    }
}

// Function to calculate the space utilization breakdown for specific file types
void calculateSpaceUtilization(const std::string& drive, const std::vector<FileType>& fileTypesToScan) {
    fs::space_info spaceInfo;
    try {
        spaceInfo = fs::space(drive);
    } catch (const fs::filesystem_error& e) {
        // Handle the error while accessing the main directory
        std::cerr << "Error accessing the drive: " << drive << " - " << e.what() << "\n";
        return;
    }

    std::cout << "Drive: " << drive << "\n";
    std::cout << "Total Space: " << spaceInfo.capacity << " bytes\n";
    std::cout << "Free Space: " << spaceInfo.free << " bytes\n";

    // Data structure to store space utilization breakdown by file type
    std::unordered_map<FileType, uintmax_t> fileTypeUsage;

    std::vector<std::string> inaccessibleDirs;

    // Perform custom recursive directory traversal
    traverseDirectory(drive, fileTypesToScan, fileTypeUsage, inaccessibleDirs);

    // Display space utilization breakdown by file type
    for (const auto& [type, size] : fileTypeUsage) {
        std::cout << getFileTypeName(type) << ": " << size << " bytes\n";
    }

    std::cout << "\n";

    // Display inaccessible directories
    if (!inaccessibleDirs.empty()) {
        std::cout << "Inaccessible Directories:\n";
        for (const auto& dir : inaccessibleDirs) {
            std::cout << dir << "\n";
        }
        std::cout<<"\n";
    }
}

int main() {
    // Get a list of available drives (you can use platform-specific APIs or libraries)
    std::vector<std::string> drives = { "D:/", "F:/" }; // Replace with available drives on your system

    std::vector<FileType> fileTypesToScan = { FileType::Video, FileType::Image, FileType::Document }; // User-specified file types to scan

    for (const auto& drive : drives) {
        calculateSpaceUtilization(drive, fileTypesToScan);
    }

    std::cout << "\nNOTE: If some directories are inaccessible, try running the program as an administrator to access all files.\n";

    return 0;
}
