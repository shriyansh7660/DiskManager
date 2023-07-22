#include <iostream>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <chrono>
#include <thread>
#include <mutex>
#include <iomanip>

namespace fs = std::filesystem;

enum class FileType
{
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
// Data structure to hold file type information
struct FileExtension
{
    std::string extension;
    unsigned long long size;
};
// Global variable to store the temporary directory name
const std::string TRASH_DIR_NAME = "Trash";

// Function to convert std::filesystem::file_time_type to time_t
std::time_t to_time_t(const std::filesystem::file_time_type &ftime)
{
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    return std::chrono::system_clock::to_time_t(sctp);
}

std::string sizeToString(unsigned long long sizeInBytes) {
    static const char* suffixes[] = {"bytes", "KB", "MB", "GB"};
    const int numSuffixes = sizeof(suffixes) / sizeof(suffixes[0]);

    int suffixIndex = 0;
    double size = static_cast<double>(sizeInBytes);

    while (size >= 1024 && suffixIndex < numSuffixes - 1) {
        size /= 1024;
        suffixIndex++;
    }

    std::string result;
    // if (suffixIndex == 0) {
    //     result = std::to_string(size);
    // } else {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << size;
        result = oss.str();
   // }

    return result + " " + suffixes[suffixIndex];
}

// Function to perform manual cleanup of the Trash directory
void manualCleanupTrashDirectory()
{
    fs::path trashPath = fs::current_path() / TRASH_DIR_NAME;
    if (fs::exists(trashPath) && fs::is_directory(trashPath))
    {
        std::time_t now = std::time(nullptr);
        bool deletedFiles = false;

        for (const auto &entry : fs::directory_iterator(trashPath))
        {
            if (fs::is_regular_file(entry))
            {
                std::time_t fileTime = to_time_t(fs::last_write_time(entry));
                double timeDiffDays = std::difftime(now, fileTime) / (60 * 60 * 24);
                if (timeDiffDays >= 7)
                {
                    fs::remove(entry.path());
                    std::cout << "Deleted: " << entry.path().filename().string() << " from Trash directory (File exceeded 7 days).\n";
                    deletedFiles = true;
                }
            }
        }

        if (!deletedFiles)
        {
            std::cout << "No files found in the Trash directory that have not been recovered before 7 days.\n";
        }
    }
    else
    {
        std::cout << "Trash directory does not exist or is empty. Nothing to clean up.\n";
    }
}
// Function to move a file to the Trash directory
void moveToTrash(const fs::path &filePath)
{
    fs::create_directory(TRASH_DIR_NAME);
    fs::path trashPath = fs::current_path() / TRASH_DIR_NAME;
    fs::rename(filePath, trashPath / filePath.filename());
}
// Function to recover a deleted file from the Trash directory
void recoverDeletedFile()
{
    fs::path trashPath = fs::current_path() / TRASH_DIR_NAME;

    if (!fs::exists(trashPath) || !fs::is_directory(trashPath))
    {
        std::cout << "Trash directory is empty. No files to recover.\n";
        return;
    }

    std::cout << "Files available in Trash directory:\n";
    int fileNumber = 1;
    std::vector<fs::path> filesToRecover;

    for (const auto &entry : fs::directory_iterator(trashPath))
    {
        if (fs::is_regular_file(entry))
        {
            std::cout << fileNumber << ". " << entry.path().filename().string() << '\n';
            ++fileNumber;
            filesToRecover.push_back(entry.path());
        }
    }

    std::cout << "Enter the numbers of the files you want to recover (space-separated), or 0 to cancel: ";
    int fileToRecover;
    while (std::cin >> fileToRecover && fileToRecover != 0)
    {
        if (fileToRecover >= 1 && fileToRecover <= fileNumber - 1)
        {
            fs::path recoveredFilePath = filesToRecover[fileToRecover - 1];
            fs::rename(recoveredFilePath, fs::current_path() / recoveredFilePath.filename());
            std::cout << "File " << recoveredFilePath.filename().string() << " successfully recovered.\n";
        }
        else
        {
            std::cout << "Invalid file number. Please enter a valid number or 0 to cancel.\n";
        }
        std::cout << "Enter more file numbers to recover, or 0 to cancel: ";
    }
    std::cout << "File recovery process completed.\n";
}

void displayDuplicateFiles(const std::unordered_map<uintmax_t, std::vector<fs::path>> &duplicateFiles)
{
    std::cout << "\nDuplicate Files:\n";
    int groupNumber = 1;
    for (const auto &[size, files] : duplicateFiles)
    {
        std::cout << "Group " << groupNumber << " (Size: " << sizeToString( size) << " ):\n";
        for (size_t i = 0; i < files.size(); ++i)
        {
            std::cout << i + 1 << ". " << files[i].filename().string() << '\n';
        }
        ++groupNumber;
    }
}

void deleteDuplicateFiles(const std::unordered_map<uintmax_t, std::vector<fs::path>> &duplicateFiles)
{
    std::cout << "\nChoose group in which duplicate files to be deleted (0 to keep all): ";
    int groupToDelete;
    std::cin >> groupToDelete;

    if (groupToDelete <= 0 || groupToDelete > static_cast<int>(duplicateFiles.size()))
    {
        std::cout << "Invalid choice. No files deleted.\n";
        return;
    }

    auto it = duplicateFiles.begin();
    std::advance(it, groupToDelete - 1);

    std::cout << "Files in Group " << groupToDelete << ":\n";
    for (size_t i = 0; i < it->second.size(); ++i)
    {
        std::cout << i + 1 << ". " << fs::path(it->second[i]).filename().string() << '\n';
    }

    std::cout << "Enter the numbers of files to keep (space-separated), or 0 to delete all: ";
    std::vector<int> filesToKeep;
    int fileToKeep;
    std::cin >> fileToKeep;
    // while (std::cin >> fileToKeep && fileToKeep != 0) {
    if (fileToKeep >= 1 && fileToKeep <= static_cast<int>(it->second.size()))
    {
        filesToKeep.push_back(fileToKeep);
    }
    else
    {
        std::cout << "Invalid file number. Please enter a valid number or 0 to delete all.\n";
    }
    //    }

    if (filesToKeep.empty())
    {
        // Delete all files in the group
        for (size_t i = 0; i < it->second.size(); ++i)
        {
            std::cout << "Moving to Trash: " << fs::path(it->second[i]).filename().string() << '\n';
            moveToTrash(it->second[i]);
        }
        std::cout << "All files in Group " << groupToDelete << " moved to Trash directory.\n";
    }
    else
    {
        // Delete files not marked to keep
        for (size_t i = 0; i < it->second.size(); ++i)
        {
            if (std::find(filesToKeep.begin(), filesToKeep.end(), i + 1) == filesToKeep.end())
            {
                std::cout << "Moving to Trash: " << fs::path(it->second[i]).filename().string() << '\n';
                moveToTrash(it->second[i]);
            }
        }
        std::cout << "Files moved to Trash directory.\n";
    }
}

void deleteLargeFiles(const std::vector<fs::path> &largeFiles)
{
    if (largeFiles.empty())
    {
        std::cout << "No large files to delete.\n";
        return;
    }

    while (true)
    {
        std::cout << "Choose which large file to delete (0 to keep all, -1 to exit): ";
        int fileToDelete;
        std::cin >> fileToDelete;

        if (fileToDelete == -1)
        {
            std::cout << "Exiting delete mode.\n";
            break;
        }

        if (fileToDelete == 0)
        {
            std::cout << "Keeping all large files.\n";
            return;
        }

        if (fileToDelete < 1 || fileToDelete > static_cast<int>(largeFiles.size()))
        {
            std::cout << "Invalid choice. Please enter a valid number or 0 to keep all.\n";
            continue;
        }

        auto it = largeFiles.begin() + fileToDelete - 1;
        std::cout << "Deleting: " << fs::path(*it).filename().string() << '\n';
        // Move the file to the Trash directory instead of deleting it
        moveToTrash(*it);
    }
}


// Function to detect duplicate files using MD5 hashing
std::unordered_map<uintmax_t, std::vector<fs::path>> findDuplicateFiles(const fs::path &rootPath)
{
    std::unordered_map<uintmax_t, std::vector<fs::path>> duplicateFiles;

    for (const auto &entry : fs::recursive_directory_iterator(rootPath))
    {
        if (fs::is_regular_file(entry))
        {
            const auto fileSize = fs::file_size(entry);
            if (fileSize == 0)
            {
                continue; // Ignore empty files
            }

            duplicateFiles[fileSize].push_back(entry.path());
        }
    }

    // Remove entries with a single file (no duplicates based on size)
    for (auto it = duplicateFiles.begin(); it != duplicateFiles.end();)
    {
        if (it->second.size() < 2)
        {
            it = duplicateFiles.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Compare files with the same size based on content (if needed)
    for (auto &[size, files] : duplicateFiles)
    {
        if (files.size() > 1)
        {
            std::sort(files.begin(), files.end());

            auto last = std::unique(files.begin(), files.end());
            files.erase(last, files.end());
        }
    }

    return duplicateFiles;
}

// Function to calculate mean
double calculateMean(const std::vector<uintmax_t> &fileSizes)
{
    double total = std::accumulate(fileSizes.begin(), fileSizes.end(), 0.0);
    return total / fileSizes.size();
}

// Function to calculate standard deviation
double calculateStandardDeviation(const std::vector<uintmax_t> &fileSizes, double mean)
{
    double sumSquaredDifferences = 0.0;
    for (uintmax_t size : fileSizes)
    {
        double diff = static_cast<double>(size) - mean;
        sumSquaredDifferences += diff * diff;
    }
    double variance = sumSquaredDifferences / fileSizes.size();
    return std::sqrt(variance);
}

// Function to identify large files
std::vector<fs::path> findLargeFiles(const fs::path &rootPath, double mean, double stdDev)
{
    std::vector<fs::path> largeFiles;

    for (const auto &entry : fs::recursive_directory_iterator(rootPath))
    {
        if (fs::is_regular_file(entry) && fs::file_size(entry) > mean + stdDev)
        {
            largeFiles.push_back(entry.path());
        }
    }
    // Sort the largeFiles vector in descending order based on file sizes
    std::sort(largeFiles.begin(), largeFiles.end(), [](const fs::path &a, const fs::path &b)
              { return fs::file_size(a) > fs::file_size(b); });
    return largeFiles;
}

// case 3
void traverse_directories(const std::string &directory, std::vector<FileExtension> &file_types, std::vector<std::string> &inaccessibleDirs)
{
    try
    {
        for (const auto &entry : fs::recursive_directory_iterator(directory))
        {
            try
            {
                if (fs::is_regular_file(entry))
                {
                    std::string extension = entry.path().extension().string();
                    // Convert extension to lowercase for case-insensitivity
                    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                    // Add other file type checks based on your specific use case
                    // For example, checking for image file extensions like .jpg, .png, etc.

                    // Accumulate space utilization by file type
                    auto it = std::find_if(file_types.begin(), file_types.end(),
                                           [&extension](const FileExtension &ft)
                                           { return ft.extension == extension; });

                    if (it != file_types.end())
                    {
                        it->size += fs::file_size(entry);
                    }
                    else
                    {
                        FileExtension ft;
                        ft.extension = extension;
                        ft.size = fs::file_size(entry);
                        file_types.push_back(ft);
                    }
                }
            }
            catch (const std::exception &e)
            {
                // Handle exceptions while accessing individual files, if needed
                std::cerr << "Error while accessing file: " << entry.path() << ": " << e.what() << '\n';
                const fs::path dirPath = directory;
                inaccessibleDirs.push_back(dirPath.string());
            }
        }
    }
    catch (const std::exception &e)
    {
        // Handle the exception while traversing directories, e.g., print an error message
        const fs::path dirPath = directory;
        std::cout << "Drive - " << dirPath;
        // std::cerr << "Error while traversing directories: " << dirPath.string()<< " "<< e.what() << '\n';
    }
    std::cout << "\n";

    // Display inaccessible directories
}
bool sortBySize(const FileExtension &a, const FileExtension &b)
{
    return a.size > b.size;
}
// Function to calculate the space utilization breakdown for specific file types
void calculateSpaceUtilization(const std::string &drive)
{
    std::vector<FileExtension> file_types;
    std::vector<std::string> inaccessibleDirs;
    traverse_directories(drive, file_types, inaccessibleDirs);
    std::sort(file_types.begin(), file_types.end(), sortBySize);

    // Display the breakdown of space utilization
    std::cout << "Space Utilization Breakdown:\n";
    for (const auto &ft : file_types)
    {
        std::cout << "File Type: " << ft.extension << ", Size: " << sizeToString(ft.size) << " \n";
        if (!inaccessibleDirs.empty())
        {
            std::cout << "Inaccessible Directories:\n";
            for (const auto &dir : inaccessibleDirs)
            {
                std::cout << dir << "\n";
            }
            std::cout << "\n";
        }
    }
}

// Function to categorize files based on their extensions
FileType categorizeFile(const fs::path &filePath)
{
    auto extension = filePath.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower); // Convert extension to lowercase
    if (fileTypeMap.find(extension) != fileTypeMap.end())
    {
        return fileTypeMap[extension];
    }
    return FileType::Unknown;
}

// Function to get the name of the file type as a string
std::string getFileTypeName(FileType type)
{
    switch (type)
    {
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
void traverseDirectory(const fs::path &dirPath, const std::vector<FileType> &fileTypesToScan, std::unordered_map<FileType, uintmax_t> &fileTypeUsage, std::vector<std::string> &inaccessibleDirs)
{
    //std::mutex mtx; // Mutex to synchronize access to the shared data structures
  //  std::vector<std::thread> threads; // Vector to hold thread objects
    try
    {
        for (const auto &entry : fs::directory_iterator(dirPath))
        {
            if (fs::is_regular_file(entry))
            {
                FileType fileType = categorizeFile(entry.path());
                if (fileType != FileType::Unknown && std::find(fileTypesToScan.begin(), fileTypesToScan.end(), fileType) != fileTypesToScan.end())
                {
                    // std::lock_guard<std::mutex> lock(mtx); // Lock the mutex to safely modify the shared fileTypeUsage map
                    fileTypeUsage[fileType] += fs::file_size(entry.path());
                }
            }
            else if (fs::is_directory(entry))
            {
                traverseDirectory(entry.path(), fileTypesToScan, fileTypeUsage, inaccessibleDirs);
                //  threads.emplace_back([&]() {
                //         traverseDirectory(entry.path(), fileTypesToScan, fileTypeUsage, inaccessibleDirs);
                //     });
            }
        }
    }
    catch (const fs::filesystem_error &e)
    {
        // Handle the error while iterating through directories
        // std::cerr << "Error accessing file/directory: " << " - " << e.what() << "\n";
        //  std::cerr << "Error accessing file/directory: " << dirPath.string() << " - " << e.what() << "\n";
        inaccessibleDirs.push_back(dirPath.string());
    }
}

// Function to calculate the space utilization breakdown for specific file types
void calculateSpaceUtilization(const fs::path &drive, const std::vector<FileType> &fileTypesToScan)
{
    fs::space_info spaceInfo;
    try
    {
        spaceInfo = fs::space(drive);
    }
    catch (const fs::filesystem_error &e)
    {
        // Handle the error while accessing the main directory
        std::cerr << "Error accessing the drive: " << drive << " - " << e.what() << "\n";
        return;
    }

    std::cout << "Drive: " << drive << "\n";
    std::cout << "Total Space: " << sizeToString( spaceInfo.capacity) << " \n";
    std::cout << "Free Space: " << sizeToString( spaceInfo.free) << " \n";

    // Data structure to store space utilization breakdown by file type
    std::unordered_map<FileType, uintmax_t> fileTypeUsage;

    std::vector<std::string> inaccessibleDirs;

    // Perform custom recursive directory traversal
    traverseDirectory(drive, fileTypesToScan, fileTypeUsage, inaccessibleDirs);

    // Display space utilization breakdown by file type
    for (const auto &[type, size] : fileTypeUsage)
    {
        std::cout << getFileTypeName(type) << ": " << sizeToString(size) << " \n";
    }

    std::cout << "\n";

    // Display inaccessible directories
    if (!inaccessibleDirs.empty())
    {
        std::cout << "Inaccessible Directories:\n";
        for (const auto &dir : inaccessibleDirs)
        {
            std::cout << dir << "\n";
        }
        std::cout << "\n";
    }
}

// Function to delete files of specific types using multi-threading
void delete_files_of_type(const fs::path&  directory, const std::string& file_type) {
    std::vector<std::thread> threads;

    try {
        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            try {
                if (fs::is_regular_file(entry)) {
                    std::string extension = entry.path().extension().string();
                    // Convert extension to lowercase for case-insensitivity
                    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

                    if (extension == file_type) {
                        std::cout << "Deleting file: " << entry.path() << '\n';
                        threads.emplace_back([](const fs::directory_entry& e) {
                            moveToTrash (e);
                        }, entry);
                    }
                }
            } catch (const std::exception& e) {
                // Handle exceptions while accessing individual files, if needed
                std::cerr << "Error while accessing file: " << entry.path() << ": " << e.what() << '\n';
            }
        }
    } catch (const std::exception& e) {
        // Handle the exception while traversing directories
        std::cerr << "Error while traversing directory: " << directory << ": " << e.what() << '\n';
    }

    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }
}

int main()
{
    std::vector<std::string> drives = {"C:/", "D:/", "F:/"}; // Replace with available drives on your system
    std::unordered_map<uintmax_t, std::vector<fs::path>> duplicateFiles;
    std::vector<uintmax_t> fileSizes;
    std::vector<fs::path> largeFiles;
    std::vector<FileType> fileTypesToScan = {FileType::Video, FileType::Image, FileType::Document}; // User-specified file types to scan
    std::string file_type_to_delete;
    // std::vector<FileExtension> fileExtensionsToScan = { FileExtension::Video, FileExtension::Image, FileExtension::Document };
    fs::path rootPath = fs::current_path();
    int choice = 1;
    do
    {
        std::cout << "Select the feature you want to perform:\n";
        std::cout << "1. Display amount of free space\n";
        std::cout << "2. Display amount of space utilized\n";
        std::cout << "3. Provide breakdown of space utilization\n";
        std::cout << "4. Detect duplicate files\n";
        std::cout << "5. Identify large files\n";
        std::cout << "6. Scan specific file types\n";
        std::cout << "7. Delete files of specific types\n";
        // std::cout << "Enter 0 to exit\n";

        std::cin >> choice;

        switch (choice)
        {
        case 1:
            for (const auto &drive : drives)
            {
                fs::space_info spaceInfo = fs::space(drive);
                std::cout << "Drive: " << drive << "\n";
                //std::cout << std::fixed << std::setprecision(2);
               // double freeSpaceGB = static_cast<double>(spaceInfo.free) / (1024 * 1024 * 1024);

                std::cout << "Free Space: " << sizeToString(spaceInfo.free) << "\n\n";
//                std::cout << "Free Space: " << freeSpaceGB << " GB\n\n";
            }
            break;
        case 2:
            for (const auto &drive : drives)
            {
                fs::space_info spaceInfo = fs::space(drive);
                std::cout << "Drive: " << drive << "\n";
                std::cout << std::fixed << std::setprecision(2);
               // double usedSpaceGB = static_cast<double>(spaceInfo.capacity - spaceInfo.free) / (1024 * 1024 * 1024);
                std::cout << "Used Space: " << sizeToString(spaceInfo.capacity - spaceInfo.free) << "\n\n";
            }
            break;
        case 3:
            for (const auto &drive : drives)
            {

                calculateSpaceUtilization(drive);
            }
            break;
        case 4:
            // Implement the function for detecting duplicate files
            std::cout << "\nFinding duplicate files...\n";
            duplicateFiles = findDuplicateFiles(rootPath);
            displayDuplicateFiles(duplicateFiles);
            std::cout << "Do you want to delete duplicate files( y / n)?";
            char dupli;
            std::cin >> dupli;
            if (dupli == 'y')
                deleteDuplicateFiles(duplicateFiles);
            break;
        case 5:
            // Implement the function for identifying large files

            std::cout << "\nCalculating statistics and finding large files...\n";
            for (const auto &entry : fs::recursive_directory_iterator(rootPath))
            {
                if (fs::is_regular_file(entry))
                {
                    fileSizes.push_back(fs::file_size(entry));
                }
            }
            if (!fileSizes.empty())
            {
                double mean = calculateMean(fileSizes);
                double stdDev = calculateStandardDeviation(fileSizes, mean);

                std::cout << "Mean file size: " <<  sizeToString(mean) << " \n";
                std::cout << "Standard Deviation: " << sizeToString(stdDev) << " \n";

                std::cout << "\nFinding large files...\n";
                largeFiles = findLargeFiles(rootPath, mean, stdDev);
                for (size_t i = 0; i < largeFiles.size(); ++i)
                {
                    double sizeMB = static_cast<double>(fs::file_size(largeFiles[i])) / (1024 * 1024);
                    std::cout << i + 1 << ". Large file: " << fs::path(largeFiles[i]).filename().string() << " (Size: " << sizeMB << " MB)\n";
                }
            }
            else
            {
                std::cout << "No files found in the directory.\n";
            }
            std::cout << "Do you want to delete large files( y / n)?";
            char larger;
            std::cin >> larger;
            if (larger == 'y')
                deleteLargeFiles(largeFiles);
            break;
        case 6:
            // Implement the function for scanning specific file types

            // for (const auto& drive : drives) {
            calculateSpaceUtilization("F:/", fileTypesToScan);
            //    }

            std::cout << "\nNOTE: If some directories are inaccessible, try running the program as an administrator to access all files.\n";

            break;
        case 7:
            // Allow users to delete files of specific types
           
             std::cout << "\nEnter the file type to delete (e.g., .txt, .jpg, etc.): ";
             std::cin >> file_type_to_delete;
             delete_files_of_type(rootPath, file_type_to_delete);
            break;
        default:
            std::cout << " Exiting...\n";
        }
        char cmdRepeat;
        std::cout << "Do you want to continue ('y' / 'n') ? ";
        std::cin >> cmdRepeat;
        if (cmdRepeat == 'n')
            choice = 0;
    } while (choice != 0);

    return 0;
}
