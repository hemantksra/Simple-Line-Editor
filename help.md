# Simple Line Editor — Command Reference

A minimal command-line text editor that operates on numbered lines.
The menu is redrawn on every iteration; the last action result is
always shown in the **Status** bar just above the prompt.

---

## How to Build & Run

```bash
gcc -Wall -Wextra -o editor editor.c
./editor          # Linux / macOS
.\editor.exe      # Windows PowerShell
```

---

## Commands at a Glance

| Command | Syntax              | Description                                       |
|---------|---------------------|---------------------------------------------------|
| Insert  | `i <num> <text>`    | Insert a line of text at position `<num>`         |
| Delete  | `d <num>`           | Delete the line at position `<num>`               |
| Print   | `p`                 | Display all lines with their line numbers         |
| Search  | `s <query>`         | Find every line containing `<query>`              |
| Replace | `r <num|*> <o>/<n>` | Replace text on one line or the whole document    |
| Undo    | `u`                 | Reverse the last insert, delete, or replace       |
| Stats   | `w`                 | Show line count, word count, and character count  |
| Quit    | `q`                 | Free all memory and exit                          |

---

## Command Details

### `i` — Insert

```
i <line_num> <text>
```

Inserts `<text>` as a new line at the **1-indexed** position `<line_num>`.
All existing lines from that position onward are shifted down by one.

- Valid range: `1` to `current line count + 1` (appending after the last line).
- Text longer than **255 characters** is silently truncated.

**Examples**

```
i 1 Hello, world!          -> inserts as the very first line
i 3 A new third line       -> inserts before current line 3
i 5 Appended at the end    -> appends when the doc has 4 lines
```

**Error cases**

```
i 0 text    -> Error: line number 0 out of range
i 99 text   -> Error: line number 99 out of range (if doc has fewer lines)
i           -> Error: missing arguments
```

---

### `d` — Delete

```
d <line_num>
```

Permanently removes the line at `<line_num>`. All subsequent lines
shift up by one to close the gap. Cannot delete from an empty document.

- Valid range: `1` to `current line count`.

**Examples**

```
d 1    -> removes the first line
d 3    -> removes the third line
```

**Error cases**

```
d       -> Error: missing argument
d 0     -> Error: line number out of range
d 99    -> Error: line number out of range (if fewer lines exist)
```

---

### `p` — Print

```
p
```

Displays the full document in the **Status** area, with each line
prefixed by its 1-indexed line number.

**Example output**

```
  1: Hello, world!
  2: The quick brown fox
  3: Jumped over the lazy dog
```

If the document is empty, shows `(document is empty)`.

---

### `s` — Search

```
s <query>
```

Searches every line for the substring `<query>` and reports all
matching line numbers along with their content.

- **Case-sensitive** — `Hello` and `hello` are distinct.
- Multi-word phrases are supported: `s quick brown` matches any line
  containing that exact phrase.

**Examples**

```
s hello          -> finds all lines containing "hello"
s quick brown    -> finds all lines containing the phrase "quick brown"
```

**Example output**

```
Search results for "hello":
  Line 1: hello world
  Line 3: say hello again
  (2 lines matched)
```

**When nothing matches**

```
Search: "xyz" not found in any line
```

---

### `r` — Find & Replace

```
r <line_num|*> <old>/<new>
```

Replaces **all occurrences** of `<old>` with `<new>` on a specific
line, or across the entire document when `*` is used.

- The **first `/`** separates `<old>` from `<new>`.
- `<new>` may be empty to **delete** all occurrences of `<old>`.
- Case-sensitive.

**Examples**

```
r 1 cat/dog          -> replace "cat" with "dog" on line 1 only
r * the/a            -> replace every "the" with "a" across all lines
r 2 hello/           -> delete every "hello" on line 2
r * colour/color     -> global spelling fix across the whole document
```

**Example output**

```
[~] Replaced 2 occurrences on line 1
[~] Replaced 5 occurrences across 3 lines
Replace: "cat" not found on line 2
```

---

### `u` — Undo

```
u
```

Reverses the **most recent** insert, delete, or replace operation.
The undo history holds up to **32 actions**; once full, the oldest
entry is discarded to make room.

| Previous action    | What undo does                                        |
|--------------------|-------------------------------------------------------|
| `i N text`         | Deletes the line that was just inserted at position N |
| `d N`              | Re-inserts the deleted text at position N             |
| `r N old/new`      | Restores the original text on line N                  |
| `r * old/new`      | Restores original text on every line that changed     |

**Example**

```
> i 1 Hello
  Status: [+] Line 1 inserted: "Hello"

> u
  Status: [U] Undo: removed inserted line 1
```

**When nothing can be undone**

```
Undo: nothing left to undo
```

NOTE: Undo itself is not recorded — you cannot "redo" an undone action.

---

### `w` — Statistics

```
w
```

Counts and displays document statistics in the **Status** area.

| Statistic    | Description                                                    |
|--------------|----------------------------------------------------------------|
| Lines        | Total number of lines                                          |
| Words        | Whitespace-delimited token count across all lines              |
| Characters   | Sum of all line lengths (newline characters not counted)       |
| Longest line | Character length of the widest single line                     |

**Example output**

```
Stats: 3 lines | 7 words | 32 chars | longest line: 19 chars
```

**Empty document**

```
Stats: 0 lines | 0 words | 0 chars
```

---

### `q` — Quit

```
q
```

Frees all allocated memory (document lines and undo stack) and exits
the editor cleanly. You can also press **Ctrl-Z** (Windows) or
**Ctrl-D** (Unix) to trigger an EOF quit.

---

## Constraints & Limits

| Item                  | Limit                                              |
|-----------------------|----------------------------------------------------|
| Maximum line length   | 255 characters (longer input is silently truncated)|
| Undo history depth    | 32 actions (oldest is evicted when full)           |
| Find & replace        | Case-sensitive substring matching only             |
| File I/O              | Not supported — document lives in memory only      |

---

## Quick Workflow Example

```
> i 1 The cat sat on the mat
  [+] Line 1 inserted: "The cat sat on the mat"

> i 2 The cat ate a rat
  [+] Line 2 inserted: "The cat ate a rat"

> i 3 The end
  [+] Line 3 inserted: "The end"

> p
  1: The cat sat on the mat
  2: The cat ate a rat
  3: The end

> s cat
  Search results for "cat":
    Line 1: The cat sat on the mat
    Line 2: The cat ate a rat
  (2 lines matched)

> r * cat/dog
  [~] Replaced 2 occurrences across 2 lines

> w
  Stats: 3 lines | 12 words | 43 chars | longest line: 22 chars

> u
  [U] Undo: reverted replacements across 2 lines

> d 3
  [-] Line 3 deleted: "The end"

> q
```
