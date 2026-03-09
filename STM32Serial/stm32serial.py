#!/usr/bin/env python3
"""
STM32 Serial Monitor - Dedicated TUI terminal for STM32 ASM Interpreter.

A rich terminal UI for serial communication with the STM32L476RG ARM
Assembly Interpreter. Provides color-coded log output and an interactive
command input.

Usage:
    python stm32serial.py --port COM3
    python stm32serial.py --port /dev/ttyACM0 --baud 115200
"""

import argparse
import re
import threading
from queue import Empty, Queue
from datetime import datetime

import serial
from rich.text import Text
from textual.app import App, ComposeResult
from textual.binding import Binding
from textual.containers import Vertical
from textual.css.query import NoMatches
from textual.events import Key
from textual.widgets import Input, RichLog, Static
from textual.widgets._input import Selection as InputSelection


# ── Log level styling ────────────────────────────────────────────────
LOG_STYLES: dict[str, str] = {
    "CRITICAL": "#8B0000",   # dark red / burgundy
    "ERROR":    "#FF4444",   # red
    "WARN":     "#FF8C00",   # orange
    "DEBUG":    "#A07EAA",   # purple
}

TAG_PATTERN = re.compile(r"^\[(CRITICAL|ERROR|WARN|DEBUG)\]\s*")


def style_line(raw: str) -> Text:
    """Return a Rich Text object with appropriate colouring for *raw*."""
    match = TAG_PATTERN.match(raw)
    if match:
        level = match.group(1)
        style = LOG_STYLES.get(level, "")
        return Text(raw, style=style)
    return Text(raw, style="white")


# ── Status bar widget ────────────────────────────────────────────────
class StatusBar(Static):
    """Thin status strip showing connection info."""

    def __init__(self, port: str, baud: int, **kwargs) -> None:
        super().__init__(**kwargs)
        self._port = port
        self._baud = baud
        self._connected = False

    def set_connected(self, connected: bool) -> None:
        self._connected = connected
        self._refresh_text()

    def _refresh_text(self) -> None:
        icon = "*" if self._connected else "o"
        colour = "green" if self._connected else "red"
        self.update(
            Text.assemble(
                (f" {icon} ", colour),
                (f"{self._port} @ {self._baud} baud", ""),
                ("  |  ", "dim"),
                (datetime.now().strftime("%H:%M:%S"), "dim"),
            )
        )


# ── Custom Input widget ──────────────────────────────────────────────
class CmdInput(Input):
    """Custom Input that binds ctrl+a to select all."""
    BINDINGS = [
        Binding("ctrl+a", "select_all", "Zaznacz wszystko", show=False),
    ]


# ── Main application ─────────────────────────────────────────────────
class STM32SerialApp(App):
    """Textual TUI application for STM32 serial communication."""

    TITLE = "STM32 Serial Monitor"
    SUB_TITLE = "ARM Assembly Interpreter"

    CSS = """
    Screen {
        layout: vertical;
        background: #000000;
        background-tint: transparent;
    }

    *:focus,
    *:focus-within {
        background-tint: transparent;
    }

    #status-bar {
        height: 1;
        background: #0d0d0d;
        color: #cccccc;
        padding: 0 1;
    }

    #input-container,
    #input-container:focus-within {
        height: auto;
        padding: 0 1;
        background: #000000;
    }

    #cmd-input,
    #cmd-input:focus,
    #cmd-input:focus-within {
        width: 100%;
        border: round #514161;
        background: #000000;
        background-tint: transparent;
        color: #ffffff;
    }

    #cmd-input > .input--selection {
        background: #2a2a2a;
    }

    #log-view,
    #log-view:focus,
    #log-view:focus-within {
        height: 1fr;
        border: round #514161;
        background: #000000;
        background-tint: transparent;
        color: #ffffff;
        padding: 0 1;
        margin: 0 1 1 1;
        scrollbar-size: 1 1;
        scrollbar-color: #514161;
    }
    """

    BINDINGS = [
        Binding("ctrl+q", "quit", "Wyjdz", show=False),
        Binding("ctrl+c", "quit", "Wyjdz", show=False, priority=True),
        Binding("ctrl+l", "clear_log", "Wyczysc log", show=False),
        Binding("ctrl+d", "toggle_timestamps", "Znaczniki czasu", show=False),
    ]

    def __init__(self, port: str, baud: int) -> None:
        super().__init__()
        self._port = port
        self._baud = baud
        self._serial: serial.Serial | None = None
        self._reader_thread: threading.Thread | None = None
        self._running = False
        self._show_timestamps = False
        self._command_history: list[str] = []
        self._history_idx = -1
        self._pending_lines: Queue[str] = Queue()
        self._max_lines_per_flush = 8

    # ── UI composition ────────────────────────────────────────────
    def compose(self) -> ComposeResult:
        yield StatusBar(self._port, self._baud, id="status-bar")
        with Vertical(id="input-container"):
            yield CmdInput(
                placeholder="MOV R0, #42  |  .reg  |  .mem 0x20000000",
                select_on_focus=False,
                id="cmd-input",
            )
        yield RichLog(
            highlight=False,
            markup=False,
            wrap=True,
            auto_scroll=True,
            id="log-view",
        )

    # ── Lifecycle ─────────────────────────────────────────────────
    def on_mount(self) -> None:
        """Open serial port and start reader thread."""
        log = self.query_one("#log-view", RichLog)

        # Start clock timer to update status bar every second
        self.set_interval(1.0, self._update_status_clock)
        self.set_interval(1 / 60, self._flush_pending_lines)

        self._connect_serial()

    def _update_status_clock(self) -> None:
        """Refresh the status bar clock."""
        try:
            status = self.query_one("#status-bar", StatusBar)
            status._refresh_text()
        except NoMatches:
            pass

    def _connect_serial(self) -> None:
        """Attempt to open the serial port."""
        log = self.query_one("#log-view", RichLog)
        status = self.query_one("#status-bar", StatusBar)

        try:
            self._serial = serial.Serial(
                port=self._port,
                baudrate=self._baud,
                timeout=0.005,
            )
            status.set_connected(True)
            log.write(
                Text(f"Polaczono z {self._port} @ {self._baud} baud", style="green")
            )
            log.write("")
            self._running = True
            self._reader_thread = threading.Thread(
                target=self._read_serial, daemon=True
            )
            self._reader_thread.start()

        except serial.SerialException as exc:
            status.set_connected(False)
            log.write(
                Text(f"Nie mozna otworzyc {self._port}: {exc}", style="red")
            )
            log.write(
                Text(
                    "  Sprawdz czy port jest poprawny i nie jest zajety.",
                    style="dim",
                )
            )

    # ── Serial I/O ────────────────────────────────────────────────
    def _read_serial(self) -> None:
        """Background thread: read lines from serial and queue for display."""
        buffer = ""
        while self._running and self._serial and self._serial.is_open:
            try:
                waiting = self._serial.in_waiting
                if waiting > 0:
                    data = self._serial.read(waiting)
                else:
                    data = self._serial.read(1)
                if data:
                    text = data.decode("utf-8", errors="replace")
                    buffer += text
                    lines_to_send: list[str] = []
                    while "\r\n" in buffer or "\n" in buffer:
                        rn_pos = buffer.find("\r\n")
                        n_pos = buffer.find("\n")
                        if rn_pos != -1 and (n_pos == -1 or rn_pos <= n_pos):
                            line = buffer[:rn_pos]
                            buffer = buffer[rn_pos + 2:]
                        else:
                            line = buffer[:n_pos]
                            buffer = buffer[n_pos + 1:]
                        line = line.rstrip("\r")
                        lines_to_send.append(line)
                    if lines_to_send:
                        self._queue_lines(lines_to_send)
            except serial.SerialException:
                if buffer:
                    self._queue_lines([buffer.rstrip("\r")])
                self.call_from_thread(
                    self._append_styled,
                    "Polaczenie przerwane!",
                    "red",
                )
                break
            except Exception:
                pass

    def _queue_lines(self, lines: list[str]) -> None:
        """Queue received lines for steady UI flushing."""
        for line in lines:
            self._pending_lines.put(line)

    def _flush_pending_lines(self) -> None:
        """Flush a small batch of queued lines to the log each frame."""
        lines: list[str] = []
        for _ in range(self._max_lines_per_flush):
            try:
                lines.append(self._pending_lines.get_nowait())
            except Empty:
                break

        if lines:
            self._append_lines_batch(lines)

    def _append_lines_batch(self, lines: list[str]) -> None:
        """Append multiple lines at once (runs on main thread)."""
        try:
            log = self.query_one("#log-view", RichLog)
        except NoMatches:
            return
        for raw in lines:
            if self._show_timestamps:
                ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                prefix = Text(f"[{ts}] ", style="dim")
                styled = style_line(raw)
                combined = Text()
                combined.append_text(prefix)
                combined.append_text(styled)
                log.write(combined)
            else:
                log.write(style_line(raw))

    def _append_styled(self, msg: str, style: str) -> None:
        """Append a message with explicit style."""
        try:
            log = self.query_one("#log-view", RichLog)
            log.write(Text(msg, style=style))
        except NoMatches:
            pass

    def _send_command(self, cmd: str) -> None:
        """Send a string to the STM32 over serial."""
        if self._serial and self._serial.is_open:
            try:
                self._serial.write((cmd + "\n").encode("utf-8"))
            except serial.SerialException:
                self._append_styled(
                    "Blad zapisu na port szeregowy!", "red"
                )

    # ── Input handling ────────────────────────────────────────────
    def on_input_submitted(self, event: Input.Submitted) -> None:
        """Handle <Enter> in the command input."""
        cmd = event.value.strip()
        if not cmd:
            return

        # Command history
        self._command_history.append(cmd)
        self._history_idx = -1

        # Echo the sent command in the log
        log = self.query_one("#log-view", RichLog)
        prompt_text = Text()
        if self._show_timestamps:
            ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]
            prompt_text.append(f"[{ts}] ", style="dim")
        prompt_text.append("> ", style="#a57dcc")
        prompt_text.append(cmd, style="white")
        log.write("")
        log.write(prompt_text)

        # Send over serial
        self._send_command(cmd)

        # Clear input
        event.input.value = ""

    # ── Actions ───────────────────────────────────────────────────
    def action_clear_log(self) -> None:
        """Clear the log view."""
        log = self.query_one("#log-view", RichLog)
        log.clear()

    def action_toggle_timestamps(self) -> None:
        """Toggle timestamp display on/off."""
        self._show_timestamps = not self._show_timestamps
        state = "wlaczone" if self._show_timestamps else "wylaczone"
        self._append_styled(f"Znaczniki czasu: {state}", "#a57dcc")

    def on_key(self, event: Key) -> None:
        """Handle arrow up/down for command history, redirect typing to input."""
        try:
            cmd_input = self.query_one("#cmd-input", CmdInput)
        except NoMatches:
            return

        if event.key in ("up", "down"):
            if not self._command_history:
                return
            if not cmd_input.has_focus:
                cmd_input.focus()

            if event.key == "up":
                if self._history_idx == -1:
                    self._history_idx = len(self._command_history) - 1
                elif self._history_idx > 0:
                    self._history_idx -= 1
                cmd_input.value = self._command_history[self._history_idx]
            else:
                if self._history_idx == -1:
                    return
                if self._history_idx < len(self._command_history) - 1:
                    self._history_idx += 1
                    cmd_input.value = self._command_history[self._history_idx]
                else:
                    self._history_idx = -1
                    cmd_input.value = ""

            cmd_input.cursor_position = len(cmd_input.value)
            event.prevent_default()
            event.stop()
            return

        # If user starts typing while log is focused, redirect to input
        if not cmd_input.has_focus:
            if event.character and event.character.isprintable() and len(event.character) == 1:
                cmd_input.focus()
                cmd_input.selection = InputSelection.cursor(len(cmd_input.value))
                cmd_input.cursor_position = len(cmd_input.value)
                cmd_input.value += event.character
                cmd_input.cursor_position = len(cmd_input.value)
                cmd_input.selection = InputSelection.cursor(cmd_input.cursor_position)
                event.prevent_default()
                event.stop()
                return
            # Don't handle history keys when input not focused
            return

    def on_unmount(self) -> None:
        """Clean up serial connection."""
        self._running = False
        if self._serial and self._serial.is_open:
            self._serial.close()


# ── CLI entry point ───────────────────────────────────────────────────
import sys
from rich.console import Console
from rich.panel import Panel

def main() -> None:
    parser = argparse.ArgumentParser(
        description="STM32 Serial Monitor – TUI terminal for ARM ASM Interpreter",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Przykłady:\n"
            "  python stm32serial.py --port COM3\n"
            "  python stm32serial.py --port /dev/ttyACM0 --baud 9600\n"
        ),
    )
    parser.add_argument(
        "--port", "-p",
        required=True,
        help="Port szeregowy (np. COM3, /dev/ttyACM0)",
    )
    parser.add_argument(
        "--baud", "-b",
        type=int,
        default=115200,
        help="Prędkość transmisji (domyślnie: 115200)",
    )
    args = parser.parse_args()

    # Test connection before starting TUI
    try:
        test_serial = serial.Serial(port=args.port, baudrate=args.baud, timeout=0.005)
        test_serial.close()
    except serial.SerialException as exc:
        console = Console()
        console.print(Panel(
            f"[red]Nie można otworzyć portu {args.port}:[/red]\n{exc}\n\n"
            "[dim]Sprawdź czy port jest poprawny, czy urządzenie jest podłączone\n"
            "i czy inny program (np. STM32CubeIDE) nie blokuje dostępu.[/dim]",
            title="[bold red]Błąd połączenia[/bold red]",
            border_style="red",
            expand=False
        ))
        sys.exit(1)

    app = STM32SerialApp(port=args.port, baud=args.baud)
    app.run()


if __name__ == "__main__":
    main()
