#include "merkle.hpp"
#include <iostream>
#include <string>

using namespace std;

void print_menu() 
{
    cout << "\n==== Merkle Tree File System CLI ====\n";
    cout << "1. Build Merkle tree from directory\n";
    cout << "2. Print tree structure\n";
    cout << "3. Print file objects\n";
    cout << "4. Show statistics\n";
    cout << "5. Verify tree integrity\n";
    cout << "6. Export tree to JSON\n";
    cout << "7. Set chunk size\n";
    cout << "8. Exit\n";
    cout << "Choose an option: ";
}

int main() 
{
    MerkleTree mtree;
    shared_ptr<MerkleNode> root = nullptr;
    string directory;
    bool tree_built = false;

    while (true) 
    {
        print_menu();
        int choice;
        cin >> choice;
        cin.ignore();

        switch (choice) 
        {
            case 1: 
            {
                cout << "Enter directory path: ";
                getline(cin, directory);
                try 
                {
                    root = mtree.build_tree(directory);
                    tree_built = true;
                    cout << "Merkle tree built successfully.\n";
                } 
                catch (const exception &e) 
                {
                    cerr << "Error: " << e.what() << endl;
                }
                break;
            }
            case 2: 
            {
                if (!tree_built) 
                {
                    cout << "Build the tree first (option 1).\n";
                    break;
                }
                mtree.print_tree_details(root);
                break;
            }
            case 3: 
            {
                if (!tree_built) 
                {
                    cout << "Build the tree first (option 1).\n";
                    break;
                }
                mtree.print_file_objects();
                break;
            }
            case 4: 
            {
                if (!tree_built) 
                {
                    cout << "Build the tree first (option 1).\n";
                    break;
                }
                auto [totalFiles, totalDirs, totalSize] = mtree.getTreeStats();
                cout << "Total files: " << totalFiles << endl;
                cout << "Total directories: " << totalDirs << endl;
                cout << "Total size: " << formatFileSize(totalSize) << endl;
                cout << "Tree depth: " << root->getDepth() << endl;
                cout << "Root hash: " << root->hash << endl;
                break;
            }
            case 5: 
            {
                if (!tree_built) 
                {
                    cout << "Build the tree first (option 1).\n";
                    break;
                }
                bool valid = mtree.verifyTreeIntegrity();
                cout << (valid ? "Tree integrity verified: OK" : "Tree integrity check FAILED!") << endl;
                break;
            }
            case 6: 
            {
                if (!tree_built) 
                {
                    cout << "Build the tree first (option 1).\n";
                    break;
                }
                string json = mtree.exportToJson();
                cout << json << endl;
                break;
            }
            case 7: 
            {
                cout << "Enter new chunk size in bytes: ";
                size_t chunkSize;
                cin >> chunkSize;
                cin.ignore();
                try {
                    mtree.setChunkSize(chunkSize);
                    cout << "Chunk size set to " << mtree.getChunkSize() << " bytes.\n";
                } catch (const exception &e) {
                    cerr << "Error: " << e.what() << endl;
                }
                break;
            }
            case 8: 
            {
                cout << "Exiting.\n";
                return 0;
            }
            default:
                cout << "Invalid option. Try again.\n";
                break;
        }
    }

    return 0;
}