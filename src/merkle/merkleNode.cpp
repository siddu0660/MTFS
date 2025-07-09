#include "merkle.hpp"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>

/**
 * @brief Constructor for MerkleNode
 * @param name Name of the file or directory
 * @param isFile True if this represents a file, false for directory
 */
MerkleNode::MerkleNode(const string &name, bool isFile)
    : name(name), isFile(isFile), fileSize(0), cachedDepth(-1)
{
    // Initialize empty hash - will be calculated later
    hash = "";
    contentHash = "";
    chunkHashes.clear();
    children.clear();
}

/**
 * @brief Add a child node to this MerkleNode
 * @param child Shared pointer to the child node to add
 * @throws runtime_error If trying to add child to a file node
 */
void MerkleNode::addChild(const shared_ptr<MerkleNode> &child)
{
    if (isFile)
    {
        throw runtime_error("Cannot add child to a file node: " + name);
    }

    if (!child)
    {
        throw runtime_error("Cannot add null child to node: " + name);
    }

    children[child->name] = child;
    // Reset cached depth when structure changes
    cachedDepth = -1;
}

/**
 * @brief Calculate the Merkle hash of this node
 * @return String containing the calculated hash
 *
 * For files: Returns the content hash
 * For directories: Calculates hash based on sorted children hashes
 */
string MerkleNode::calculateHash()
{
    if (isFile)
    {
        // For files, the hash is the content hash
        hash = contentHash;
        return hash;
    }
    else
    {
        // For directories, calculate hash based on sorted children
        if (children.empty())
        {
            // Empty directory gets hash of its name
            hash = sha256(name);
            return hash;
        }

        // Sort children by name for consistent hashing
        vector<string> sortedNames;
        for (const auto &child : children)
        {
            sortedNames.push_back(child.first);
        }
        sort(sortedNames.begin(), sortedNames.end());

        // Concatenate all children hashes
        string combined = "";
        for (const string &childName : sortedNames)
        {
            auto child = children[childName];
            string childHash = child->calculateHash();
            combined += childName + ":" + childHash + ";";
        }

        // Hash the combined string
        hash = sha256(combined);
        return hash;
    }
}

/**
 * @brief Get the depth of this node in the tree
 * @return Depth level (0 for root)
 */
int MerkleNode::getDepth() const
{
    if (cachedDepth != -1)
    {
        return cachedDepth;
    }

    if (children.empty())
    {
        cachedDepth = 0;
        return cachedDepth;
    }

    int maxChildDepth = 0;
    for (const auto &child : children)
    {
        int childDepth = child.second->getDepth();
        maxChildDepth = max(maxChildDepth, childDepth);
    }

    cachedDepth = maxChildDepth + 1;
    return cachedDepth;
}

/**
 * @brief Check if this node is a leaf (has no children)
 * @return True if leaf node, false otherwise
 */
bool MerkleNode::isLeaf() const
{
    return children.empty();
}

/**
 * @brief Get total size of all files under this node
 * @return Total size in bytes
 */
size_t MerkleNode::getTotalSize() const
{
    if (isFile)
    {
        return fileSize;
    }

    size_t totalSize = 0;
    for (const auto &child : children)
    {
        totalSize += child.second->getTotalSize();
    }

    return totalSize;
}

/**
 * @brief Get the number of files under this node
 * @return Number of files
 */
size_t MerkleNode::getFileCount() const
{
    if (isFile)
    {
        return 1;
    }

    size_t fileCount = 0;
    for (const auto &child : children)
    {
        fileCount += child.second->getFileCount();
    }

    return fileCount;
}

/**
 * @brief Helper function to calculate SHA-256 hash of data
 * @param data Input data to hash
 * @return Hexadecimal string representation of the hash
 */
string MerkleNode::sha256(const string &data)
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