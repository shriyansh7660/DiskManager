#include <iostream>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include<chrono>

namespace fs = std::filesystem;
// Global variable to store the temporary directory name
const std::string TRASH_DIR_NAME = "Trash";

// Function to convert std::filesystem::file_time_type to time_t
std::time_t to_time_t(const std::filesystem::file_time_type& ftime) {
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    return std::chrono::system_clock::to_time_t(sctp);
}

// Function to perform manual cleanup of the Trash directory
void manualCleanupTrashDirectory() {
    fs::path trashPath = fs::current_path() / TRASH_DIR_NAME;
    if (fs::exists(trashPath) && fs::is_directory(trashPath)) {
        std::time_t now = std::time(nullptr);
        bool deletedFiles = false;

        for (const auto& entry : fs::directory_iterator(trashPath)) {
            if (fs::is_regular_file(entry)) {
                std::time_t fileTime = to_time_t(fs::last_write_time(entry));
                double timeDiffDays = std::difftime(now, fileTime) / (60 * 60 * 24);
                if (timeDiffDays >= 7) {
                    fs::remove(entry.path());
                    std::cout << "Deleted: " << entry.path().filename().string() << " from Trash directory (File exceeded 7 days).\n";
                    deletedFiles = true;
                }
            }
        }

        if (!deletedFiles) {
            std::cout << "No files found in the Trash directory that have not been recovered before 7 days.\n";
        }
    } else {
        std::cout << "Trash directory does not exist or is empty. Nothing to clean up.\n";
    }
}
// Function to move a file to the Trash directory
void moveToTrash(const fs::path& filePath) {
    fs::create_directory(TRASH_DIR_NAME);
    fs::path trashPath = fs::current_path() / TRASH_DIR_NAME;
    fs::rename(filePath, trashPath / filePath.filename());
}
// Function to recover a deleted file from the Trash directory
void recoverDeletedFile() {
    fs::path trashPath = fs::current_path() / TRASH_DIR_NAME;

    if (!fs::exists(trashPath) || !fs::is_directory(trashPath)) {
        std::cout << "Trash directory is empty. No files to recover.\n";
        return;
    }

    std::cout << "Files available in Trash directory:\n";
    int fileNumber = 1;
    std::vector<fs::path> filesToRecover;

    for (const auto& entry : fs::directory_iterator(trashPath)) {
        if (fs::is_regular_file(entry)) {
            std::cout << fileNumber << ". " << entry.path().filename().string() << '\n';
            ++fileNumber;
            filesToRecover.push_back(entry.path());
        }
    }

    std::cout << "Enter the numbers of the files you want to recover (space-separated), or 0 to cancel: ";
    int fileToRecover;
    while (std::cin >> fileToRecover && fileToRecover != 0) {
        if (fileToRecover >= 1 && fileToRecover <= fileNumber - 1) {
            fs::path recoveredFilePath = filesToRecover[fileToRecover - 1];
            fs::rename(recoveredFilePath, fs::current_path() / recoveredFilePath.filename());
            std::cout << "File " << recoveredFilePath.filename().string() << " successfully recovered.\n";
        } else {
            std::cout << "Invalid file number. Please enter a valid number or 0 to cancel.\n";
        }
        std::cout << "Enter more file numbers to recover, or 0 to cancel: ";
    }
    std::cout << "File recovery process completed.\n";
}

void displayDuplicateFiles(const std::unordered_map<uintmax_t, std::vector<fs::path>>& duplicateFiles) {
    std::cout << "\nDuplicate Files:\n";
    int groupNumber = 1;
    for (const auto& [size, files] : duplicateFiles) {
        std::cout << "Group " << groupNumber << " (Size: " << size << " bytes):\n";
        for (size_t i = 0; i < files.size(); ++i) {
            std::cout << i + 1 << ". " << files[i].filename().string() << '\n';
        }
        ++groupNumber;
    }
}

void deleteDuplicateFiles(const std::unordered_map<uintmax_t, std::vector<fs::path>>& duplicateFiles) {
    std::cout << "\nChoose which group  duplicate files to delete (0 to keep all): ";//to modify//name of file
    int groupToDelete;
    std::cin >> groupToDelete;

    if (groupToDelete <= 0 || groupToDelete > static_cast<int>(duplicateFiles.size())) {
        std::cout << "Invalid choice. No files deleted.\n";
        return;
    }

    auto it = duplicateFiles.begin();
    std::advance(it, groupToDelete - 1);

    std::cout << "Files in Group " << groupToDelete << ":\n";
    for (size_t i = 0; i < it->second.size(); ++i) {
        std::cout << i + 1 << ". " << fs::path(it->second[i]).filename().string() << '\n';
    }

    std::cout << "Enter the numbers of files to keep (space-separated), or 0 to delete all: ";
    std::vector<int> filesToKeep;
    int fileToKeep;
    // while (std::cin >> fileToKeep && fileToKeep != 0) {
     if (std::cin >> fileToKeep && fileToKeep != 0) {
        if (fileToKeep >= 1 && fileToKeep <= static_cast<int>(it->second.size())) {
            filesToKeep.push_back(fileToKeep);
        } else {
            std::cout << "Invalid file number. Please enter a valid number or 0 to delete all.\n";
        }
    }

    if (filesToKeep.empty()) {
        // Delete all files in the group
        for (size_t i = 0; i < it->second.size(); ++i) {
            std::cout << "Moving to Trash: " << fs::path(it->second[i]).filename().string() << '\n';
            moveToTrash(it->second[i]);
        }
        std::cout << "All files in Group " << groupToDelete << " moved to Trash directory.\n";
    } else {
        // Delete files not marked to keep
        for (size_t i = 0; i < it->second.size(); ++i) {
            if (std::find(filesToKeep.begin(), filesToKeep.end(), i + 1) == filesToKeep.end()) {
                std::cout << "Moving to Trash: " << fs::path(it->second[i]).filename().string() << '\n';
                moveToTrash(it->second[i]);
            }
        }
        std::cout << "Files moved to Trash directory.\n";
    }
}

void deleteLargeFiles(const std::vector<fs::path>& largeFiles) {
    if (largeFiles.empty()) {
        std::cout << "No large files to delete.\n";
        return;
    }

    while (true) {
        std::cout << "Choose which large file to delete (0 to keep all, -1 to exit): ";
        int fileToDelete;
        std::cin >> fileToDelete;

        if (fileToDelete == -1) {
            std::cout << "Exiting delete mode.\n";
            break;
        }

        if (fileToDelete == 0) {
            std::cout << "Keeping all large files.\n";
            return;
        }

        if (fileToDelete < 1 || fileToDelete > static_cast<int>(largeFiles.size())) {
            std::cout << "Invalid choice. Please enter a valid number or 0 to keep all.\n";
            continue;
        }

        auto it = largeFiles.begin() + fileToDelete - 1;
        std::cout << "Deleting: " << fs::path(*it).filename().string() << '\n';
        // Move the file to the Trash directory instead of deleting it
        moveToTrash(*it);
    }
}

// Function to display disk space information
//i have made change here so that the size is displayed in GB and not in bytes.
void displayDiskSpaceInfo() {
    fs::space_info spaceInfo = fs::space(fs::current_path());

    double totalSpaceGB = static_cast<double>(spaceInfo.capacity) / (1024 * 1024 * 1024);
    double freeSpaceGB = static_cast<double>(spaceInfo.free) / (1024 * 1024 * 1024);
    double usedSpaceGB = totalSpaceGB - freeSpaceGB;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Total Disk Space: " << totalSpaceGB << " GB\n";
    std::cout << "Free Disk Space: " << freeSpaceGB << " GB\n";
    std::cout << "Used Disk Space: " << usedSpaceGB << " GB\n";
}


// Function to detect duplicate files using MD5 hashing
std::unordered_map<uintmax_t, std::vector<fs::path>> findDuplicateFiles(const fs::path& rootPath) {
    std::unordered_map<uintmax_t, std::vector<fs::path>> duplicateFiles;

    for (const auto& entry : fs::recursive_directory_iterator(rootPath)) {
        if (fs::is_regular_file(entry)) {
            const auto fileSize = fs::file_size(entry);
            if (fileSize == 0) {
                continue; // Ignore empty files
            }

            duplicateFiles[fileSize].push_back(entry.path());
        }
    }

    // Remove entries with a single file (no duplicates based on size)
    for (auto it = duplicateFiles.begin(); it != duplicateFiles.end();) {
        if (it->second.size() < 2) {
            it = duplicateFiles.erase(it);
        } else {
            ++it;
        }
    }

    // Compare files with the same size based on content (if needed)
    for (auto& [size, files] : duplicateFiles) {
        if (files.size() > 1) {
            std::sort(files.begin(), files.end());

            auto last = std::unique(files.begin(), files.end());
            files.erase(last, files.end());
        }
    }

    return duplicateFiles;
}

// Function to calculate mean
double calculateMean(const std::vector<uintmax_t>& fileSizes) {
    double total = std::accumulate(fileSizes.begin(), fileSizes.end(), 0.0);
    return total / fileSizes.size();
}

// Function to calculate standard deviation
double calculateStandardDeviation(const std::vector<uintmax_t>& fileSizes, double mean) {
    double sumSquaredDifferences = 0.0;
    for (uintmax_t size : fileSizes) {
        double diff = static_cast<double>(size) - mean;
        sumSquaredDifferences += diff * diff;
    }
    double variance = sumSquaredDifferences / fileSizes.size();
    return std::sqrt(variance);
}

// Function to identify large files
std::vector<fs::path> findLargeFiles(const fs::path& rootPath, double mean, double stdDev) {
    std::vector<fs::path> largeFiles;

    for (const auto& entry : fs::recursive_directory_iterator(rootPath)) {
        if (fs::is_regular_file(entry) && fs::file_size(entry) > mean + stdDev) {
            largeFiles.push_back(entry.path());
        }
    }
    // Sort the largeFiles vector in descending order based on file sizes
    std::sort(largeFiles.begin(), largeFiles.end(), [](const fs::path& a, const fs::path& b) {
        return fs::file_size(a) > fs::file_size(b);
    });
    return largeFiles;
}

int main() {
    //I have changed the file path here so now i can analyze other folder
    //this is how this folder can remane clean
    //fs::path rootPath = "C:/Users/shriy/OneDrive/Desktop/files";
    fs::path rootPath = "C:/Users/Kaushiki/OneDrive/Desktop/Shreyansh/tallycodebrewers";
    displayDiskSpaceInfo();

    //changes made to display file name only and not the whole address
    std::cout << "\nFinding duplicate files...\n";
    std::unordered_map<uintmax_t, std::vector<fs::path>> duplicateFiles = findDuplicateFiles(rootPath);
    // for (const auto& [hash, files] : duplicateFiles) {
    //     std::cout << "Duplicate files with hash " << hash << ":\n";
    //     for (const auto& file : files) {
    //         std::cout << fs::path(file).filename().string() << '\n'; // Print only the file name
    //     }
    // }
    displayDuplicateFiles(duplicateFiles);
    deleteDuplicateFiles(duplicateFiles);

    std::cout << "\nCalculating statistics and finding large files...\n";
    std::vector<uintmax_t> fileSizes;
    for (const auto& entry : fs::recursive_directory_iterator(rootPath)) {
        if (fs::is_regular_file(entry)) {
            fileSizes.push_back(fs::file_size(entry));
        }
    }
    if (!fileSizes.empty()) {
    double mean = calculateMean(fileSizes);
    double stdDev = calculateStandardDeviation(fileSizes, mean);

    std::cout << "Mean file size: " << mean << " bytes\n";
    std::cout << "Standard Deviation: " << stdDev << " bytes\n";

    std::cout << "\nFinding large files...\n";
        std::vector<fs::path> largeFiles = findLargeFiles(rootPath, mean, stdDev);
        for (size_t i = 0; i < largeFiles.size(); ++i) {
            double sizeMB = static_cast<double>(fs::file_size(largeFiles[i])) / (1024 * 1024);
            std::cout << i + 1 << ". Large file: " << fs::path(largeFiles[i]).filename().string() << " (Size: " << sizeMB << " MB)\n";
        }

        deleteLargeFiles(largeFiles);
    } else {
        std::cout << "No files found in the directory.\n";
    }
    recoverDeletedFile();
    return 0;
}