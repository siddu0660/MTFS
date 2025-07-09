/**
 * @file merkle.hpp
 * @brief Merkle Tree File System (MTFS) Header
 * @version 1.0
 * @date 2024
 *
 * This header file defines the classes and structures for implementing
 * a Merkle Tree based file system that converts directory structures
 * into Merkle trees for integrity verification and efficient storage.
 *
 * Based on the MTFS paper by Jia Kan and Kyeong Soo Kim
 * from Xi'an Jiaotong-Liverpool University
 */

#ifndef MTFS_HPP
#define MTFS_HPP

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <tuple>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>

#pragma once

using namespace std;

namespace fs = filesystem;

/**
 * @struct MerkleNode
 * @brief Represents a node in the Merkle tree structure
 *
 * Each node can represent either a file or a directory in the file system.
 * Files store content hashes and chunk information, while directories
 * store references to their children nodes.
 */
typedef struct MerkleNode
{
public:
    string name;                // Name of the file or directory
    string hash;                // Calculated Merkle hash of this node
    string contentHash;         // Hash of the file content (for files only)
    vector<string> chunkHashes; // Hashes of individual chunks (for large files)

    map<string, shared_ptr<MerkleNode>> children; // Child nodes (for directories)

    bool isFile;     // Flag indicating if this is a file (true) or directory (false)
    size_t fileSize; // Size of the file in bytes (for files only)

    /**
     * @brief Constructor for MerkleNode
     * @param name Name of the file or directory
     * @param isFile True if this represents a file, false for directory
     */
    MerkleNode(const string &name, bool isFile = false);

    /**
     * @brief Add a child node to this MerkleNode
     * @param child Shared pointer to the child node to add
     * @throws runtime_error If trying to add child to a file node
     */
    void addChild(const shared_ptr<MerkleNode> &child);

    /**
     * @brief Calculate the Merkle hash of this node
     * @return String containing the calculated hash
     *
     * For files: Returns the content hash
     * For directories: Calculates hash based on sorted children hashes
     */
    string calculateHash();

    /**
     * @brief Get the depth of this node in the tree
     * @return Depth level (0 for root)
     */
    int getDepth() const;

    /**
     * @brief Check if this node is a leaf (has no children)
     * @return True if leaf node, false otherwise
     */
    bool isLeaf() const;

    /**
     * @brief Get total size of all files under this node
     * @return Total size in bytes
     */
    size_t getTotalSize() const;

    /**
     * @brief Get the number of files under this node
     * @return Number of files
     */
    size_t getFileCount() const;

private:
    /**
     * @brief Helper function to calculate SHA-256 hash of data
     * @param data Input data to hash
     * @return Hexadecimal string representation of the hash
     */
    string sha256(const string &data);

    mutable int cachedDepth = -1; // Cached depth value for performance
};

/**
 * @class MerkleTree
 * @brief Main class for building and managing Merkle tree file systems
 *
 * This class provides functionality to:
 * - Build Merkle trees from directory structures
 * - Calculate file and directory hashes
 * - Process files in chunks for large file support
 * - Display tree structure and statistics
 */
class MerkleTree
{
public:
    /**
     * @brief Default constructor for MerkleTree
     */
    MerkleTree();

    /**
     * @brief Constructor with custom chunk size
     * @param chunkSize Size of chunks for file processing (default: 1MB)
     */
    explicit MerkleTree(size_t chunkSize);

    /**
     * @brief Destructor
     */
    ~MerkleTree() = default;

    /**
     * @brief Calculate SHA-256 hash of input data
     * @param data Input data to hash
     * @return Hexadecimal string representation of the hash
     */
    string sha256(const string &data);

    /**
     * @brief Hash file content and split into chunks
     * @param file_path Path to the file to process
     * @return Tuple containing (content_hash, file_size, chunk_hashes)
     * @throws runtime_error If file cannot be opened or read
     */
    tuple<string, size_t, vector<string>> hash_file_content(const string &file_path);

    /**
     * @brief Build Merkle tree from directory path
     * @param directory_path Path to the directory to process
     * @return Shared pointer to the root node of the built tree
     * @throws runtime_error If directory path is invalid
     */
    shared_ptr<MerkleNode> build_tree(const string &directory_path);

    /**
     * @brief Build a single node from filesystem path
     * @param path Filesystem path to process
     * @return Shared pointer to the created node
     * @throws runtime_error If path is invalid or inaccessible
     */
    shared_ptr<MerkleNode> build_node(const fs::path &path);

    /**
     * @brief Print detailed tree structure
     * @param node Root node to start printing from
     * @param depth Current depth level for indentation
     */
    void print_tree_details(shared_ptr<MerkleNode> node, int depth = 0);

    /**
     * @brief Print file objects and their chunk information
     */
    void print_file_objects();

    /**
     * @brief Process directory and display complete analysis
     * @param directory_path Path to the directory to process
     */
    void process_directory(const string &directory_path);

    /**
     * @brief Get the root node of the tree
     * @return Shared pointer to root node
     */
    shared_ptr<MerkleNode> getRoot() const;

    /**
     * @brief Get tree statistics
     * @return Tuple containing (total_files, total_directories, total_size)
     */
    tuple<size_t, size_t, size_t> getTreeStats() const;

    /**
     * @brief Verify tree integrity by recalculating hashes
     * @return True if all hashes are valid, false otherwise
     */
    bool verifyTreeIntegrity();

    /**
     * @brief Find a node by name in the tree
     * @param name Name to search for
     * @return Shared pointer to found node, nullptr if not found
     */
    shared_ptr<MerkleNode> findNode(const string &name);

    /**
     * @brief Export tree structure to JSON format
     * @return JSON string representation of the tree
     */
    string exportToJson() const;

    /**
     * @brief Set custom chunk size for file processing
     * @param chunkSize New chunk size in bytes
     */
    void setChunkSize(size_t chunkSize);

    /**
     * @brief Get current chunk size
     * @return Current chunk size in bytes
     */
    size_t getChunkSize() const;

private:
    shared_ptr<MerkleNode> root;                      // Root node of the Merkle tree
    map<string, shared_ptr<MerkleNode>> file_objects; // Map of content hash to file nodes
    vector<shared_ptr<MerkleNode>> nodes;             // Vector of all nodes in the tree
    size_t CHUNK_SIZE;                                // Size of chunks for file processing (default: 1MB)

    /**
     * @brief Recursive helper for finding nodes
     * @param node Current node to search in
     * @param name Name to search for
     * @return Shared pointer to found node, nullptr if not found
     */
    shared_ptr<MerkleNode> findNodeRecursive(shared_ptr<MerkleNode> node, const string &name);

    /**
     * @brief Helper function to export node to JSON
     * @param node Node to export
     * @param depth Current depth for indentation
     * @return JSON string representation of the node
     */
    string nodeToJson(shared_ptr<MerkleNode> node, int depth = 0);

    /**
     * @brief Calculate statistics recursively
     * @param node Current node
     * @param files Reference to file count
     * @param directories Reference to directory count
     * @param totalSize Reference to total size
     */
    void calculateStatsRecursive(shared_ptr<MerkleNode> node, size_t &files, size_t &directories, size_t &totalSize);

    /**
     * @brief Verify node integrity recursively
     * @param node Node to verify
     * @return True if node and all children are valid
     */
    bool verifyNodeIntegrity(shared_ptr<MerkleNode> node);
};

/**
 * @brief Utility function to format file size in human-readable format
 * @param bytes Size in bytes
 * @return Formatted string (e.g., "1 MB", "2 KB")
 */
string formatFileSize(size_t bytes);

/**
 * @brief Utility function to get file extension
 * @param filename Name of the file
 * @return File extension (including the dot)
 */
string getFileExtension(const string &filename);

/**
 * @brief Utility function to check if file is binary
 * @param filepath Path to the file
 * @return True if file appears to be binary
 */
bool isBinaryFile(const string &filepath);

// Constants
namespace MTFSConstants
{
    const size_t DEFAULT_CHUNK_SIZE = 1024 * 1024;   // Default chunk size (1MB)
    const size_t MAX_CHUNK_SIZE = 100 * 1024 * 1024; // Maximum chunk size (100MB)
    const size_t MIN_CHUNK_SIZE = 1024;              // Minimum chunk size (1KB)
    const int MAX_TREE_DEPTH = 10;                   // Maximum allowed tree depth
    const string MTFS_VERSION = "1.0";               // MTFS version
}

#endif