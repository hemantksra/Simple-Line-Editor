# Simple Line Editor

A line editor is a simple text editor that works one line at a time — no cursor
moving freely around a 2-D screen, just commands that operate on line numbers.
Think of it as the ancestor of modern editors: the user types a command, the
editor acts on a specific line (or range of lines), and the result is shown back.

---

## Team Members

| Name | Roll Number |
|------|-------------|
| Hemant Saxena | R25EF100 |
| D Keertana | R25EF068 |
| Dhanush AK | R25EF073 |

---

## How to Compile & Run

### Prerequisites
- GCC (any version supporting C99 or later)
- A terminal / command prompt

### Compile

```bash
gcc -Wall -Wextra -o editor editor.c
```

### Run

```bash
# Windows (PowerShell / CMD)
.\editor.exe

# Linux / macOS
./editor
```

### Exit
Type `q` at the prompt, or press **Ctrl-Z** (Windows) / **Ctrl-D** (Unix).

---

## Features (Chronological Order)

### 1. Core Document Structure
The foundational `Document` data structure — a dynamically-resizable array of
string pointers — along with three supporting functions:

- **`doc_init`** — initialises the document with a starting capacity of 4 lines.
- **`doc_grow`** — doubles the internal capacity via `realloc` whenever the
  document is full.
- **`doc_free`** — frees every line string and the array itself.

---

### 2. Insert (`i <num> <text>`)
Inserts a new line of text at any valid 1-indexed position. Lines below the
insertion point are shifted down automatically. Input longer than 255 characters
is silently truncated.

```
> i 1 Hello, world!
  Status: [+] Line 1 inserted: "Hello, world!"
```

---

### 3. Delete (`d <num>`)
Removes the line at the given position. Subsequent lines shift up to close the
gap. Validates the range and reports a clear error if the line does not exist.

```
> d 2
  Status: [-] Line 2 deleted: "Hello, world!"
```

---

### 4. Print / Display (`p`)
Prints the entire document to the status area with 1-indexed line numbers.
Shows `(document is empty)` when there are no lines.

```
> p
  1: The quick brown fox
  2: Jumped over the lazy dog
```

---

### 5. TUI Screen Refresh
The terminal is cleared and the command menu is redrawn before every prompt so
the interface never scrolls. The result of the last command always appears in a
fixed **Status** line between the menu and the prompt.

```
+------------------------------------------+
|          SIMPLE LINE EDITOR              |
+------------------------------------------+
| i <num> <text>  Insert text at line <num>|
| ...                                      |
+------------------------------------------+

  Status: [+] Line 1 inserted: "Hello"


> _
```

---

### 6. Search (`s <query>`)
Scans every line for a substring and reports all matching line numbers with a
match count. Case-sensitive. Supports multi-word phrase queries.

```
> s fox
  Search results for "fox":
    Line 1: The quick brown fox
  (1 line matched)
```

---

### 7. Find & Replace (`r <num|*> <old>/<new>`)
Replaces all occurrences of a word or phrase on a specific line (`r 2 cat/dog`)
or across the entire document at once (`r * cat/dog`). Setting `<new>` to empty
deletes all occurrences. Reports the exact count of substitutions made.

```
> r * cat/dog
  Status: [~] Replaced 3 occurrences across 2 lines
```

---

### 8. Undo (`u`)
Reverses the most recent insert, delete, or replace operation. For global
replaces (`r *`) the entire set of changed lines is restored in one step.
The undo history holds up to 32 actions; the oldest is discarded when full.

```
> u
  Status: [U] Undo: reverted replacements across 2 lines
```

---

### 9. Document Statistics (`w`)
Reports a word count, line count, total character count, and the length of the
longest line — all computed on demand without modifying the document.

```
> w
  Status: Stats: 3 lines | 12 words | 54 chars | longest line: 22 chars
```

---

## File Overview

| File | Purpose |
|------|---------|
| `editor.c` | Full source — all data structures, functions, and the command loop |
| `help.md` | Detailed per-command reference with examples and error messages |
| `README.md` | This file — team info, build steps, and feature history |

---

## Quick Command Reference

| Command | Syntax | Action |
|---------|--------|--------|
| Insert | `i <num> <text>` | Add a line at position `<num>` |
| Delete | `d <num>` | Remove the line at position `<num>` |
| Print | `p` | Show all lines |
| Search | `s <query>` | Find lines containing `<query>` |
| Replace | `r <num|*> <old>/<new>` | Find & replace on one line or all |
| Undo | `u` | Reverse the last action |
| Stats | `w` | Line / word / character counts |
| Quit | `q` | Exit and free memory |

> See [`help.md`](help.md) for full documentation with examples and error messages.
