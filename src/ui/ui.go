package ui

import (
	"bufio"
	"fmt"
	"io"
	"os"
	"os/exec"
	"strconv"
	"strings"
	"time"

	"github.com/gdamore/tcell/v2"
	"github.com/rivo/tview"
)

type MerkleTUI struct {
	app           *tview.Application
	pages         *tview.Pages
	menu          *tview.List
	output        *tview.TextView
	input         *tview.InputField
	status        *tview.TextView
	cppProcess    *exec.Cmd
	stdin         io.WriteCloser
	stdout        io.ReadCloser
	scanner       *bufio.Scanner
	currentAction string
	treeBuilt     bool
	outputBuffer  []string
}

func NewMerkleTUI() *MerkleTUI {
	app := tview.NewApplication()
	
	tui := &MerkleTUI{
		app:          app,
		pages:        tview.NewPages(),
		outputBuffer: make([]string, 0),
	}
	
	tui.setupUI()
	tui.startCppProcess()
	
	return tui
}

func (tui *MerkleTUI) setupUI() {
	// Create main menu
	tui.menu = tview.NewList().
		AddItem("Build Merkle tree from directory", "Create tree structure", '1', tui.buildTree).
		AddItem("Print tree structure", "Display tree hierarchy", '2', tui.printTree).
		AddItem("Print file objects", "Show file details", '3', tui.printFiles).
		AddItem("Show statistics", "Display tree stats", '4', tui.showStats).
		AddItem("Verify tree integrity", "Check tree validity", '5', tui.verifyTree).
		AddItem("Export tree to JSON", "Export as JSON", '6', tui.exportJSON).
		AddItem("Set chunk size", "Configure chunk size", '7', tui.setChunkSize).
		AddItem("Exit", "Quit application", '8', tui.exit)

	tui.menu.SetBorder(true).SetTitle("Merkle Tree File System CLI")
	tui.menu.SetSelectedTextColor(tcell.ColorBlack)
	tui.menu.SetSelectedBackgroundColor(tcell.ColorWhite)

	// Create output area
	tui.output = tview.NewTextView().
		SetDynamicColors(true).
		SetScrollable(true).
		SetWrap(true)
	tui.output.SetBorder(true).SetTitle("Output")

	// Create input field
	tui.input = tview.NewInputField().
		SetLabel("Input: ").
		SetFieldWidth(50)
	tui.input.SetBorder(true).SetTitle("Input")

	// Create status bar
	tui.status = tview.NewTextView().
		SetDynamicColors(true).
		SetText("[green]Ready[white] | Tree: [red]Not Built[white] | Press Tab to navigate")
	tui.status.SetBorder(true).SetTitle("Status")

	// Create main layout
	mainLayout := tview.NewFlex().SetDirection(tview.FlexRow).
		AddItem(tview.NewFlex().SetDirection(tview.FlexColumn).
			AddItem(tui.menu, 0, 1, true).
			AddItem(tui.output, 0, 2, false), 0, 4, false).
		AddItem(tui.input, 3, 0, false).
		AddItem(tui.status, 3, 0, false)

	// Input field handlers
	tui.input.SetDoneFunc(func(key tcell.Key) {
		if key == tcell.KeyEnter {
			tui.handleInput()
		}
	})

	// Set up key bindings
	tui.app.SetInputCapture(func(event *tcell.EventKey) *tcell.EventKey {
		switch event.Key() {
		case tcell.KeyTab:
			if tui.app.GetFocus() == tui.menu {
				tui.app.SetFocus(tui.input)
			} else {
				tui.app.SetFocus(tui.menu)
			}
			return nil
		case tcell.KeyEscape:
			tui.app.SetFocus(tui.menu)
			return nil
		}
		return event
	})

	tui.pages.AddPage("main", mainLayout, true, true)
}

func (tui *MerkleTUI) startCppProcess() {
	// Start the C++ executable
	tui.cppProcess = exec.Command("merkle/mtfs")
	
	var err error
	tui.stdin, err = tui.cppProcess.StdinPipe()
	if err != nil {
		tui.writeOutput(fmt.Sprintf("[red]Error creating stdin pipe: %v[white]", err))
		return
	}
	
	tui.stdout, err = tui.cppProcess.StdoutPipe()
	if err != nil {
		tui.writeOutput(fmt.Sprintf("[red]Error creating stdout pipe: %v[white]", err))
		return
	}
	
	err = tui.cppProcess.Start()
	if err != nil {
		tui.writeOutput(fmt.Sprintf("[red]Error starting C++ process: %v[white]", err))
		return
	}
	
	tui.scanner = bufio.NewScanner(tui.stdout)
	
	// Start reading output in a goroutine
	go tui.readOutput()
}

func (tui *MerkleTUI) readOutput() {
	for tui.scanner.Scan() {
		line := tui.scanner.Text()
		tui.outputBuffer = append(tui.outputBuffer, line)
		
		// Process output based on current action
		tui.app.QueueUpdateDraw(func() {
			tui.processOutput(line)
		})
	}
}

func (tui *MerkleTUI) processOutput(line string) {
	switch tui.currentAction {
	case "build":
		tui.processBuildOutput(line)
	case "print_tree":
		tui.processPrintTreeOutput(line)
	case "print_files":
		tui.processPrintFilesOutput(line)
	case "stats":
		tui.processStatsOutput(line)
	case "verify":
		tui.processVerifyOutput(line)
	case "export":
		tui.processExportOutput(line)
	case "chunk":
		tui.processChunkOutput(line)
	default:
		tui.writeOutput(line)
	}
}

func (tui *MerkleTUI) processBuildOutput(line string) {
	if strings.Contains(line, "Merkle tree built successfully") {
		tui.writeOutput("[green]✓ Merkle tree built successfully![white]")
		tui.writeOutput("[blue]Tree is now ready for operations.[white]")
	} else if strings.Contains(line, "Error:") {
		tui.writeOutput(fmt.Sprintf("[red]✗ %s[white]", line))
	} else if strings.Contains(line, "Enter directory path:") {
		// Skip this line as we handle it in UI
		return
	} else {
		tui.writeOutput(fmt.Sprintf("[yellow]%s[white]", line))
	}
}

func (tui *MerkleTUI) processPrintTreeOutput(line string) {
	if strings.HasPrefix(line, "├─") || strings.HasPrefix(line, "└─") || strings.HasPrefix(line, "│") {
		// Tree structure lines
		tui.writeOutput(fmt.Sprintf("[cyan]%s[white]", line))
	} else if strings.Contains(line, "Hash:") {
		tui.writeOutput(fmt.Sprintf("[green]%s[white]", line))
	} else if strings.Contains(line, "Size:") {
		tui.writeOutput(fmt.Sprintf("[blue]%s[white]", line))
	} else {
		tui.writeOutput(line)
	}
}

func (tui *MerkleTUI) processPrintFilesOutput(line string) {
	if strings.Contains(line, "File:") {
		tui.writeOutput(fmt.Sprintf("[yellow]📁 %s[white]", line))
	} else if strings.Contains(line, "Hash:") {
		tui.writeOutput(fmt.Sprintf("   [green]🔐 %s[white]", line))
	} else if strings.Contains(line, "Size:") {
		tui.writeOutput(fmt.Sprintf("   [blue]📏 %s[white]", line))
	} else if strings.Contains(line, "Chunks:") {
		tui.writeOutput(fmt.Sprintf("   [magenta]🧩 %s[white]", line))
	} else {
		tui.writeOutput(fmt.Sprintf("   %s", line))
	}
}

func (tui *MerkleTUI) processStatsOutput(line string) {
	if strings.Contains(line, "Total files:") {
		tui.writeOutput(fmt.Sprintf("[yellow]📄 %s[white]", line))
	} else if strings.Contains(line, "Total directories:") {
		tui.writeOutput(fmt.Sprintf("[blue]📁 %s[white]", line))
	} else if strings.Contains(line, "Total size:") {
		tui.writeOutput(fmt.Sprintf("[green]💾 %s[white]", line))
	} else if strings.Contains(line, "Tree depth:") {
		tui.writeOutput(fmt.Sprintf("[magenta]🌳 %s[white]", line))
	} else if strings.Contains(line, "Root hash:") {
		tui.writeOutput(fmt.Sprintf("[cyan]🔐 %s[white]", line))
	} else {
		tui.writeOutput(line)
	}
}

func (tui *MerkleTUI) processVerifyOutput(line string) {
	if strings.Contains(line, "Tree integrity verified: OK") {
		tui.writeOutput("[green]✓ Tree integrity verified: OK[white]")
		tui.writeOutput("[green]All hashes are valid and consistent.[white]")
	} else if strings.Contains(line, "Tree integrity check FAILED!") {
		tui.writeOutput("[red]✗ Tree integrity check FAILED![white]")
		tui.writeOutput("[red]Some hashes are invalid or inconsistent.[white]")
	} else {
		tui.writeOutput(line)
	}
}

func (tui *MerkleTUI) processExportOutput(line string) {
	if strings.HasPrefix(line, "{") && strings.HasSuffix(line, "}") {
		// JSON output - format it nicely
		tui.writeOutput("[green]JSON Export:[white]")
		tui.writeOutput("[cyan]" + line + "[white]")
		tui.writeOutput("[green]Export completed successfully![white]")
	} else {
		tui.writeOutput(line)
	}
}

func (tui *MerkleTUI) processChunkOutput(line string) {
	if strings.Contains(line, "Chunk size set to") {
		tui.writeOutput(fmt.Sprintf("[green]✓ %s[white]", line))
	} else if strings.Contains(line, "Error:") {
		tui.writeOutput(fmt.Sprintf("[red]✗ %s[white]", line))
	} else {
		tui.writeOutput(line)
	}
}

func (tui *MerkleTUI) writeOutput(text string) {
	fmt.Fprintf(tui.output, "%s\n", text)
	tui.output.ScrollToEnd()
}

func (tui *MerkleTUI) sendCommand(cmd string) {
	if tui.stdin != nil {
		fmt.Fprintf(tui.stdin, "%s\n", cmd)
	}
}

func (tui *MerkleTUI) updateStatus(message string) {
	treeStatus := "[red]Not Built[white]"
	if tui.treeBuilt {
		treeStatus = "[green]Built[white]"
	}
	tui.status.SetText(fmt.Sprintf("[green]%s[white] | Tree: %s | Press Tab to navigate", message, treeStatus))
}

func (tui *MerkleTUI) buildTree() {
	tui.currentAction = "build"
	tui.updateStatus("Building tree...")
	tui.writeOutput("[yellow]═══ Building Merkle Tree ═══[white]")
	tui.writeOutput("[blue]Please enter the directory path to build the tree.[white]")
	tui.sendCommand("1")
	tui.input.SetLabel("Directory path: ")
	tui.app.SetFocus(tui.input)
}

func (tui *MerkleTUI) printTree() {
	if !tui.treeBuilt {
		tui.writeOutput("[red]✗ Build the tree first (option 1).[white]")
		return
	}
	tui.currentAction = "print_tree"
	tui.updateStatus("Printing tree structure...")
	tui.writeOutput("[yellow]═══ Tree Structure ═══[white]")
	tui.sendCommand("2")
}

func (tui *MerkleTUI) printFiles() {
	if !tui.treeBuilt {
		tui.writeOutput("[red]✗ Build the tree first (option 1).[white]")
		return
	}
	tui.currentAction = "print_files"
	tui.updateStatus("Printing file objects...")
	tui.writeOutput("[yellow]═══ File Objects ═══[white]")
	tui.sendCommand("3")
}

func (tui *MerkleTUI) showStats() {
	if !tui.treeBuilt {
		tui.writeOutput("[red]✗ Build the tree first (option 1).[white]")
		return
	}
	tui.currentAction = "stats"
	tui.updateStatus("Showing statistics...")
	tui.writeOutput("[yellow]═══ Tree Statistics ═══[white]")
	tui.sendCommand("4")
}

func (tui *MerkleTUI) verifyTree() {
	if !tui.treeBuilt {
		tui.writeOutput("[red]✗ Build the tree first (option 1).[white]")
		return
	}
	tui.currentAction = "verify"
	tui.updateStatus("Verifying tree integrity...")
	tui.writeOutput("[yellow]═══ Tree Verification ═══[white]")
	tui.sendCommand("5")
}

func (tui *MerkleTUI) exportJSON() {
	if !tui.treeBuilt {
		tui.writeOutput("[red]✗ Build the tree first (option 1).[white]")
		return
	}
	tui.currentAction = "export"
	tui.updateStatus("Exporting to JSON...")
	tui.writeOutput("[yellow]═══ JSON Export ═══[white]")
	tui.sendCommand("6")
}

func (tui *MerkleTUI) setChunkSize() {
	tui.currentAction = "chunk"
	tui.updateStatus("Setting chunk size...")
	tui.writeOutput("[yellow]═══ Chunk Size Configuration ═══[white]")
	tui.sendCommand("7")
	tui.input.SetLabel("Chunk size (bytes): ")
	tui.app.SetFocus(tui.input)
}

func (tui *MerkleTUI) exit() {
	tui.updateStatus("Exiting...")
	tui.writeOutput("[yellow]═══ Exiting Application ═══[white]")
	tui.sendCommand("8")
	time.Sleep(100 * time.Millisecond) // Give time for cleanup
	tui.app.Stop()
}

func (tui *MerkleTUI) handleInput() {
	inputText := tui.input.GetText()
	tui.input.SetText("")
	
	switch tui.currentAction {
	case "build":
		tui.sendCommand(inputText)
		tui.writeOutput(fmt.Sprintf("[blue]🔨 Building tree from: %s[white]", inputText))
		tui.treeBuilt = true
		tui.currentAction = ""
		tui.input.SetLabel("Input: ")
		tui.app.SetFocus(tui.menu)
		
	case "chunk":
		// Validate chunk size
		if _, err := strconv.Atoi(inputText); err != nil {
			tui.writeOutput("[red]✗ Invalid chunk size. Please enter a number.[white]")
			return
		}
		tui.sendCommand(inputText)
		tui.writeOutput(fmt.Sprintf("[blue]🔧 Setting chunk size to: %s bytes[white]", inputText))
		tui.currentAction = ""
		tui.input.SetLabel("Input: ")
		tui.app.SetFocus(tui.menu)
		
	default:
		// Handle general input
		tui.sendCommand(inputText)
		tui.app.SetFocus(tui.menu)
	}
	
	tui.updateStatus("Ready")
}

func (tui *MerkleTUI) Run() error {
	return tui.app.SetRoot(tui.pages, true).Run()
}

func (tui *MerkleTUI) cleanup() {
	if tui.stdin != nil {
		tui.stdin.Close()
	}
	if tui.stdout != nil {
		tui.stdout.Close()
	}
	if tui.cppProcess != nil {
		tui.cppProcess.Process.Kill()
		tui.cppProcess.Wait()
	}
}

func main() {
	tui := NewMerkleTUI()
	defer tui.cleanup()
	
	if err := tui.Run(); err != nil {
		fmt.Fprintf(os.Stderr, "Error running TUI: %v\n", err)
		os.Exit(1)
	}
}