#include "merkle.hpp"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
#include <fstream>

/**
 * @brief Default constructor for MerkleTree
 */
MerkleTree::MerkleTree() : CHUNK_SIZE(MTFSConstants::DEFAULT_CHUNK_SIZE)
{
    root = nullptr;
    file_objects.clear();
    nodes.clear();
}

/**
 * @brief Constructor with custom chunk size
 * @param chunkSize Size of chunks for file processing (default: 1MB)
 */
MerkleTree::MerkleTree(size_t chunkSize) : CHUNK_SIZE(chunkSize)
{
    if (chunkSize < MTFSConstants::MIN_CHUNK_SIZE || chunkSize > MTFSConstants::MAX_CHUNK_SIZE)
    {
        throw runtime_error("Invalid chunk size. Must be between " +
                            to_string(MTFSConstants::MIN_CHUNK_SIZE) + " and " +
                            to_string(MTFSConstants::MAX_CHUNK_SIZE) + " bytes");
    }

    root = nullptr;
    file_objects.clear();
    nodes.clear();
}

/**
 * @brief Calculate SHA-256 hash of input data
 * @param data Input data to hash
 * @return Hexadecimal string representation of the hash
 */
string MerkleTree::sha256(const string &data)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256_ctx;
    SHA256_Init(&sha256_ctx);
    SHA256_Update(&sha256_ctx, data.c_str(), data.length());
    SHA256_Final(hash, &sha256_ctx);

    // Convert to hex string
    stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
    {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }

    return ss.str();
}

/**
 * @brief Hash file content and split into chunks
 * @param file_path Path to the file to process
 * @return Tuple containing (content_hash, file_size, chunk_hashes)
 * @throws runtime_error If file cannot be opened or read
 */
tuple<string, size_t, vector<string>> MerkleTree::hash_file_content(const string &file_path)
{
    ifstream file(file_path, ios::binary);
    if (!file.is_open())
    {
        throw runtime_error("Cannot open file: " + file_path);
    }

    // Get file size
    file.seekg(0, ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, ios::beg);

    vector<string> chunkHashes;
    string entireContent;

    // Read file in chunks
    char *buffer = new char[CHUNK_SIZE];
    size_t bytesRead;

    try
    {
        while (file.read(buffer, CHUNK_SIZE) || file.gcount() > 0)
        {
            bytesRead = file.gcount();
            string chunk(buffer, bytesRead);

            // Add to entire content for overall hash
            entireContent += chunk;

            // Calculate chunk hash
            string chunkHash = sha256(chunk);
            chunkHashes.push_back(chunkHash);
        }
    }
    catch (const exception &e)
    {
        delete[] buffer;
        throw runtime_error("Error reading file: " + file_path + " - " + e.what());
    }

    delete[] buffer;
    file.close();

    // Calculate hash of entire content
    string contentHash = sha256(entireContent);

    return make_tuple(contentHash, fileSize, chunkHashes);
}

/**
 * @brief Build Merkle tree from directory path
 * @param directory_path Path to the directory to process
 * @return Shared pointer to the root node of the built tree
 * @throws runtime_error If directory path is invalid
 */
shared_ptr<MerkleNode> MerkleTree::build_tree(const string &directory_path)
{
    if (!fs::exists(directory_path))
    {
        throw runtime_error("Directory does not exist: " + directory_path);
    }

    if (!fs::is_directory(directory_path))
    {
        throw runtime_error("Path is not a directory: " + directory_path);
    }

    // Clear previous tree data
    file_objects.clear();
    nodes.clear();

    // Build tree from directory
    root = build_node(fs::path(directory_path));

    // Calculate all hashes
    if (root)
    {
        root->calculateHash();
    }

    return root;
}

/**
 * @brief Build a single node from filesystem path
 * @param path Filesystem path to process
 * @return Shared pointer to the created node
 * @throws runtime_error If path is invalid or inaccessible
 */
shared_ptr<MerkleNode> MerkleTree::build_node(const fs::path &path)
{
    if (!fs::exists(path))
    {
        throw runtime_error("Path does not exist: " + path.string());
    }

    string nodeName = path.filename().string();
    bool isFile = fs::is_regular_file(path);

    auto node = make_shared<MerkleNode>(nodeName, isFile);
    nodes.push_back(node);

    if (isFile)
    {
        // Process file
        try
        {
            auto [contentHash, fileSize, chunkHashes] = hash_file_content(path.string());

            node->contentHash = contentHash;
            node->fileSize = fileSize;
            node->chunkHashes = chunkHashes;

            // Store in file_objects map
            file_objects[contentHash] = node;
        }
        catch (const exception &e)
        {
            throw runtime_error("Error processing file " + path.string() + ": " + e.what());
        }
    }
    else if (fs::is_directory(path))
    {
        // Process directory
        try
        {
            for (const auto &entry : fs::directory_iterator(path))
            {
                try
                {
                    auto childNode = build_node(entry.path());
                    node->addChild(childNode);
                }
                catch (const exception &e)
                {
                    // Log error but continue processing other entries
                    cerr << "Warning: Skipping " << entry.path().string() << " - " << e.what() << endl;
                }
            }
        }
        catch (const exception &e)
        {
            throw runtime_error("Error reading directory " + path.string() + ": " + e.what());
        }
    }

    return node;
}

/**
 * @brief Print detailed tree structure
 * @param node Root node to start printing from
 * @param depth Current depth level for indentation
 */
void MerkleTree::print_tree_details(shared_ptr<MerkleNode> node, int depth)
{
    if (!node)
        return;

    string indent(depth * 2, ' ');
    cout << indent << node->name;

    if (node->isFile)
    {
        cout << " (File, Size: " << node->fileSize << " bytes, Hash: "
             << node->contentHash.substr(0, 8) << "...)";

        if (node->chunkHashes.size() > 1)
        {
            cout << " [" << node->chunkHashes.size() << " chunks]";
        }
    }
    else
    {
        cout << " (Directory, Children: " << node->children.size() << ")";
    }

    cout << endl;

    // Print children recursively
    if (!node->isFile)
    {
        vector<string> childNames;
        for (const auto &child : node->children)
        {
            childNames.push_back(child.first);
        }
        sort(childNames.begin(), childNames.end());

        for (const string &childName : childNames)
        {
            print_tree_details(node->children[childName], depth + 1);
        }
    }
}

/**
 * @brief Print file objects and their chunk information
 */
void MerkleTree::print_file_objects()
{
    cout << "\n=== File Objects ===" << endl;

    for (const auto &entry : file_objects)
    {
        const string &hash = entry.first;
        const auto &node = entry.second;

        cout << "Content Hash: " << hash << endl;
        cout << "  File: " << node->name << endl;
        cout << "  Size: " << node->fileSize << " bytes" << endl;
        cout << "  Chunks: " << node->chunkHashes.size() << endl;

        if (node->chunkHashes.size() > 1)
        {
            cout << "  Chunk Hashes:" << endl;
            for (size_t i = 0; i < node->chunkHashes.size(); ++i)
            {
                cout << "    [" << i << "] " << node->chunkHashes[i] << endl;
            }
        }
        cout << endl;
    }
}

/**
 * @brief Process directory and display complete analysis
 * @param directory_path Path to the directory to process
 */
void MerkleTree::process_directory(const string &directory_path)
{
    cout << "Processing directory: " << directory_path << endl;
    cout << "Chunk size: " << CHUNK_SIZE << " bytes" << endl;

    try
    {
        auto root = build_tree(directory_path);

        cout << "\n=== Tree Structure ===" << endl;
        print_tree_details(root);

        auto [totalFiles, totalDirs, totalSize] = getTreeStats();
        cout << "\n=== Statistics ===" << endl;
        cout << "Total files: " << totalFiles << endl;
        cout << "Total directories: " << totalDirs << endl;
        cout << "Total size: " << totalSize << " bytes" << endl;
        cout << "Tree depth: " << root->getDepth() << endl;
        cout << "Root hash: " << root->hash << endl;

        print_file_objects();
    }
    catch (const exception &e)
    {
        throw runtime_error("Error processing directory: " + string(e.what()));
    }
}

/**
 * @brief Get the root node of the tree
 * @return Shared pointer to root node
 */
shared_ptr<MerkleNode> MerkleTree::getRoot() const
{
    return root;
}

/**
 * @brief Get tree statistics
 * @return Tuple containing (total_files, total_directories, total_size)
 */
tuple<size_t, size_t, size_t> MerkleTree::getTreeStats() const
{
    if (!root)
    {
        return make_tuple(0, 0, 0);
    }

    size_t files = 0, directories = 0, totalSize = 0;
    calculateStatsRecursive(root, files, directories, totalSize);

    return make_tuple(files, directories, totalSize);
}

/**
 * @brief Verify tree integrity by recalculating hashes
 * @return True if all hashes are valid, false otherwise
 */
bool MerkleTree::verifyTreeIntegrity()
{
    if (!root)
    {
        return true; // Empty tree is valid
    }

    return verifyNodeIntegrity(root);
}

/**
 * @brief Find a node by name in the tree
 * @param name Name to search for
 * @return Shared pointer to found node, nullptr if not found
 */
shared_ptr<MerkleNode> MerkleTree::findNode(const string &name)
{
    if (!root)
    {
        return nullptr;
    }

    return findNodeRecursive(root, name);
}

/**
 * @brief Export tree structure to JSON format
 * @return JSON string representation of the tree
 */
string MerkleTree::exportToJson() const
{
    if (!root)
    {
        return "{}";
    }

    return "{\n" + nodeToJson(root, 1) + "\n}";
}

/**
 * @brief Set custom chunk size for file processing
 * @param chunkSize New chunk size in bytes
 */
void MerkleTree::setChunkSize(size_t chunkSize)
{
    if (chunkSize < MTFSConstants::MIN_CHUNK_SIZE || chunkSize > MTFSConstants::MAX_CHUNK_SIZE)
    {
        throw runtime_error("Invalid chunk size. Must be between " +
                            to_string(MTFSConstants::MIN_CHUNK_SIZE) + " and " +
                            to_string(MTFSConstants::MAX_CHUNK_SIZE) + " bytes");
    }

    CHUNK_SIZE = chunkSize;
}

/**
 * @brief Get current chunk size
 * @return Current chunk size in bytes
 */
size_t MerkleTree::getChunkSize() const
{
    return CHUNK_SIZE;
}

/**
 * @brief Recursive helper for finding nodes
 * @param node Current node to search in
 * @param name Name to search for
 * @return Shared pointer to found node, nullptr if not found
 */
shared_ptr<MerkleNode> MerkleTree::findNodeRecursive(shared_ptr<MerkleNode> node, const string &name)
{
    if (!node)
    {
        return nullptr;
    }

    if (node->name == name)
    {
        return node;
    }

    // Search in children
    for (const auto &child : node->children)
    {
        auto result = findNodeRecursive(child.second, name);
        if (result)
        {
            return result;
        }
    }

    return nullptr;
}

/**
 * @brief Helper function to export node to JSON
 * @param node Node to export
 * @param depth Current depth for indentation
 * @return JSON string representation of the node
 */
string MerkleTree::nodeToJson(shared_ptr<MerkleNode> node, int depth) const
{
    if (!node)
    {
        return "null";
    }

    string indent(depth * 2, ' ');
    string childIndent((depth + 1) * 2, ' ');

    stringstream ss;
    ss << indent << "\"" << node->name << "\": {\n";
    ss << childIndent << "\"type\": \"" << (node->isFile ? "file" : "directory") << "\",\n";
    ss << childIndent << "\"hash\": \"" << node->hash << "\"";

    if (node->isFile)
    {
        ss << ",\n"
           << childIndent << "\"size\": " << node->fileSize;
        ss << ",\n"
           << childIndent << "\"chunks\": " << node->chunkHashes.size();
        ss << ",\n"
           << childIndent << "\"content_hash\": \"" << node->contentHash << "\"";
    }
    else if (!node->children.empty())
    {
        ss << ",\n"
           << childIndent << "\"children\": {\n";

        vector<string> childNames;
        for (const auto &child : node->children)
        {
            childNames.push_back(child.first);
        }
        sort(childNames.begin(), childNames.end());

        for (size_t i = 0; i < childNames.size(); ++i)
        {
            ss << nodeToJson(node->children[childNames[i]], depth + 2);
            if (i < childNames.size() - 1)
            {
                ss << ",";
            }
            ss << "\n";
        }

        ss << childIndent << "}";
    }

    ss << "\n"
       << indent << "}";
    return ss.str();
}

/**
 * @brief Calculate statistics recursively
 * @param node Current node
 * @param files Reference to file count
 * @param directories Reference to directory count
 * @param totalSize Reference to total size
 */
void MerkleTree::calculateStatsRecursive(shared_ptr<MerkleNode> node, size_t &files, size_t &directories, size_t &totalSize) const
{
    if (!node)
    {
        return;
    }

    if (node->isFile)
    {
        files++;
        totalSize += node->fileSize;
    }
    else
    {
        directories++;
        for (const auto &child : node->children)
        {
            calculateStatsRecursive(child.second, files, directories, totalSize);
        }
    }
}

/**
 * @brief Verify node integrity recursively
 * @param node Node to verify
 * @return True if node and all children are valid
 */
bool MerkleTree::verifyNodeIntegrity(shared_ptr<MerkleNode> node)
{
    if (!node)
    {
        return true;
    }

    // Store original hash
    string originalHash = node->hash;

    // Recalculate hash
    string calculatedHash = node->calculateHash();

    // Verify hash matches
    if (originalHash != calculatedHash)
    {
        return false;
    }

    // Verify all children recursively
    for (const auto &child : node->children)
    {
        if (!verifyNodeIntegrity(child.second))
        {
            return false;
        }
    }

    return true;
}