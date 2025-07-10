package main

import (
	"fmt"
	"log"

	ui "MTFS/ui"

	"github.com/gdamore/tcell/v2"
)

type App struct {
	screen tcell.Screen
	quit   chan struct{}
	focus  int
}

func NewApp() *App {
	return &App{
		quit:  make(chan struct{}),
		focus: 0,
	}
}

func (a *App) Init() error {
	screen, err := tcell.NewScreen()
	if err != nil {
		return err
	}

	if err := screen.Init(); err != nil {
		return err
	}

	screen.SetStyle(tcell.StyleDefault.Background(tcell.ColorBlack).Foreground(tcell.ColorWhite))
	screen.Clear()

	a.screen = screen
	return nil
}

func (a *App) drawText(x, y int, text string, style tcell.Style) {
	for i, r := range text {
		a.screen.SetContent(x+i, y, r, nil, style)
	}
}

func (a *App) drawCenteredText(y int, text string, style tcell.Style) {
	width, _ := a.screen.Size()
	x := (width - len(text)) / 2
	if x < 0 {
		x = 0
	}
	a.drawText(x, y, text, style)
}

func (a *App) drawButton(x, y, width int, text string, focused bool) {
	style := tcell.StyleDefault.Background(tcell.ColorLime).Foreground(tcell.ColorBlack)
	if focused {
		style = style.Bold(true).Reverse(true)
	}

	for i := 0; i < width; i++ {
		a.screen.SetContent(x+i, y, ' ', nil, style)
	}

	textX := x + (width-len(text))/2
	a.drawText(textX, y, text, style)
}

func (a *App) drawCenteredButton(y int, text string, focused bool) {
	width, _ := a.screen.Size()
	buttonWidth := len(text) + 4 // Add padding
	x := (width - buttonWidth) / 2
	if x < 0 {
		x = 0
	}
	a.drawButton(x, y, buttonWidth, text, focused)
}

func (a *App) draw() {
	a.screen.Clear()
	width, height := a.screen.Size()

	blackStyle := tcell.StyleDefault.Background(tcell.ColorBlack)
	for y := 0; y < height; y++ {
		for x := 0; x < width; x++ {
			a.screen.SetContent(x, y, ' ', nil, blackStyle)
		}
	}

	// ASCII art title
	titleLines := []string{
		"MMMMMMMM               MMMMMMMM TTTTTTTTTTTTTTTTTTTTTTT FFFFFFFFFFFFFFFFFFFFFF    SSSSSSSSSSSSSSS ",
		"M:::::::M             M:::::::M T:::::::::::::::::::::T F::::::::::::::::::::F  SS:::::::::::::::S",
		"M::::::::M           M::::::::M T:::::::::::::::::::::T F::::::::::::::::::::F S:::::SSSSSS::::::S",
		"M:::::::::M         M:::::::::M T:::::TT:::::::TT:::::T FF::::::FFFFFFFFF::::F S:::::S     SSSSSSS",
		"M::::::::::M       M::::::::::M TTTTTT  T:::::T  TTTTTT   F:::::F       FFFFFF S:::::S            ",
		"M:::::::::::M     M:::::::::::M         T:::::T           F:::::F              S:::::S            ",
		"M:::::::M::::M   M::::M:::::::M         T:::::T           F::::::FFFFFFFFFF     S::::SSSS         ",
		"M::::::M M::::M M::::M M::::::M         T:::::T           F:::::::::::::::F      SS::::::SSSSS    ",
		"M::::::M  M::::M::::M  M::::::M         T:::::T           F:::::::::::::::F        SSS::::::::SS  ",
		"M::::::M   M:::::::M   M::::::M         T:::::T           F::::::FFFFFFFFFF           SSSSSS::::S ",
		"M::::::M    M:::::M    M::::::M         T:::::T           F:::::F                          S:::::S",
		"M::::::M     MMMMM     M::::::M         T:::::T           F:::::F                          S:::::S",
		"M::::::M               M::::::M       TT:::::::TT       FF:::::::FF            SSSSSSS     S:::::S",
		"M::::::M               M::::::M       T:::::::::T       F::::::::FF            S::::::SSSSSS:::::S",
		"M::::::M               M::::::M       T:::::::::T       F::::::::FF            S:::::::::::::::SS ",
		"MMMMMMMM               MMMMMMMM       TTTTTTTTTTT       FFFFFFFFFFF             SSSSSSSSSSSSSSS   ",
		"",
		"MERKLE TREE based FILE SYSTEM",
	}

	totalContentHeight := len(titleLines) + 2 + 1 + 2 + 1 + 2 + 1 // title + spacing + subtitle + spacing + button + spacing + help
	startY := (height - totalContentHeight) / 2
	if startY < 0 {
		startY = 0
	}

	titleStyle := tcell.StyleDefault.Background(tcell.ColorBlack).Foreground(tcell.ColorLime).Bold(true)
	for i, line := range titleLines {
		a.drawCenteredText(startY+i, line, titleStyle)
	}

	subtitleY := startY + len(titleLines) + 2
	subtitleStyle := tcell.StyleDefault.Background(tcell.ColorBlack).Foreground(tcell.ColorGray).Italic(true)
	a.drawCenteredText(subtitleY, "A cryptographically secure file system", subtitleStyle)

	buttonY := subtitleY + 3
	a.drawCenteredButton(buttonY, "🚀 GET STARTED", a.focus == 0)

	helpY := buttonY + 2
	helpStyle := tcell.StyleDefault.Background(tcell.ColorBlack).Foreground(tcell.ColorDarkGray)
	a.drawCenteredText(helpY, "Press ENTER to get started • Press q or ESC to quit", helpStyle)

	a.screen.Show()
}

func (a *App) handleEvent(ev tcell.Event) {
	switch ev := ev.(type) {
	case *tcell.EventKey:
		switch ev.Key() {
		case tcell.KeyEscape, tcell.KeyCtrlC:
			close(a.quit)
		case tcell.KeyEnter:
			if a.focus == 0 {
				ui.NewMerkleTUI().Run()
			}
		case tcell.KeyRune:
			switch ev.Rune() {
			case 'q', 'Q':
				close(a.quit)
			case ' ':
				if a.focus == 0 {
					close(a.quit)
				}
			}
		}
	case *tcell.EventResize:
		a.screen.Sync()
	}
}

func (a *App) Run() {
	defer a.screen.Fini()

	a.draw()

	for {
		select {
		case <-a.quit:
			return
		default:
			ev := a.screen.PollEvent()
			if ev != nil {
				a.handleEvent(ev)
				a.draw()
			}
		}
	}
}

func main() {
	app := NewApp()

	if err := app.Init(); err != nil {
		log.Fatalf("Failed to initialize: %v", err)
	}

	fmt.Print("\033[40m\033[2J\033[H")
	defer fmt.Print("\033[0m\033[2J\033[H")

	app.Run()
}