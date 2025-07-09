#include "../merkle/merkle.hpp"
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>

/**
 * @brief Returns a human-readable representation of file size
 * 
 * @param bytes  Size in bytes
 * @return Formatted size with appropriate unit (B, KB, MB, GB) 
 */
std::string formatFileSize(size_t bytes)
{
    const char *units[] = {"B", "KB", "MB", "GB"};
    double size = static_cast<double>(bytes);
    int unit = 0;
    while (size >= 1024 && unit < 4)
    {
        size /= 1024;
        ++unit;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(size < 10 && unit > 0 ? 1 : 0) << size << " " << units[unit];
    return oss.str();
}

/**
 * @brief Get the File Extension
 * 
 * @param filename Name of the file
 * @return File extension (including the dot)
 */
std::string getFileExtension(const std::string &filename)
{
    size_t pos = filename.find_last_of('.');
    if (pos == std::string::npos || pos == 0 || pos == filename.length() - 1)
        return "";
    return filename.substr(pos);
}

/**
 * @brief Function to check if a file is binary
 * 
 * @param filepath Path to the file
 * @return True if the file is binary
 */
bool isBinaryFile(const std::string &filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file)
        return false; // Treat as not binary if file can't be opened

    const size_t checkSize = 512;
    char buffer[checkSize];
    file.read(buffer, checkSize);
    std::streamsize bytesRead = file.gcount();

    for (std::streamsize i = 0; i < bytesRead; ++i)
    {
        unsigned char c = static_cast<unsigned char>(buffer[i]);
        // Check for non-printable characters except common whitespace
        if ((c < 32 && c != 9 && c != 10 && c != 13) || c == 0)
        {
            return true;
        }
    }
    return false;
}
