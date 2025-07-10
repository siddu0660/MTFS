# Merkle Tree File System (MTFS)

A cryptographically secure file system that builds a Merkle tree from a directory structure for integrity verification and efficient storage. This project features a C++ backend for Merkle tree logic and a Go-based terminal UI (TUI) using `tcell` for an interactive user experience.

## Features

- **Build Merkle tree** from any directory
- **Print tree structure** and file objects
- **Show statistics** (files, directories, size, depth, root hash)
- **Verify tree integrity** using Merkle hashes
- **Export tree to JSON**
- **Configurable chunk size** for file processing
- **Go TUI frontend**: Clean, interactive menu and dialogs for all operations

## Project Structure

| File/Folder      | Description                                       |
|------------------|---------------------------------------------------|
| `merkle.hpp`     | C++ header: Merkle tree and node class definitions|
| `merkleNode.cpp` | C++: MerkleNode implementation                    |
| `merkleTree.cpp` | C++: MerkleTree implementation                    |
| `handler.cpp`    | C++ CLI for Merkle tree logic                     |
| `utils.cpp`      | C++: Utility functions (formatting, detection)    |
| `main.go`        | Baseline TUI created using `tcell`                |
| `ui.go`          | Interactive session designed using `tcell`        |

## Requirements

- **C++** (for backend)
- **Golang** (for frontend)
- **OpenSSL** (for SHA-256 in C++)

## Setup

### 1. Clone the repositry

```sh
git clone https://github.com/siddu0660/MTFS.git
cd MTFS
```

### 2. Build the C++ Backend

```sh
cd src
make all
```

### 3. Install Go Dependencies

```sh
go mod tidy
```

### 3. Build the Go TUI

```sh
go build -o mtfs_tui main.go
```

## Running

1. **Start the TUI:**

   ```sh
   ./mtfs_tui
   ```

2. **Navigate the UI:**
   - Use arrow keys to move through the menu.
   - Press `Tab` to naviagte between sections
   - Press `Enter` or `Digit` to select.
   - Input dialogs will appear for required fields (e.g., directory path).
   - All output from the backend is shown in Go dialogs.

## Credits

- Based on the MTFS paper by Jia Kan and Kyeong Soo Kim, Xi'an Jiaotong-Liverpool University.
