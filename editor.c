/*
 * editor.c — A simple command-line line editor with TUI-style screen refresh.
 *
 * Layout on each iteration:
 *   [menu box]
 *   [blank line]
 *   Status: <last action result>
 *   [blank line]
 *   [blank line]
 *   > [input prompt]
 *
 * Commands:
 *   i <line_num> <text>        Insert <text> at line <line_num>
 *   d <line_num>               Delete the line at <line_num>
 *   p                          Print the entire document
 *   s <query>                  Search for a word or phrase
 *   r <line_num|*> <old>/<new> Replace text on one line or all lines
 *   q                          Quit and free all memory
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define MAX_LINE_LEN 256
#define STATUS_LEN   4096

typedef struct {
    char **lines;
    int count;
    int capacity;
} Document;

/* Global status message displayed below the menu after every command. */
static char g_status[STATUS_LEN];

/* Write a formatted string into the global status buffer. */
static void set_status(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g_status, STATUS_LEN, fmt, ap);
    va_end(ap);
}

/* Initialize the document with an initial capacity of 4 and an empty line array. */
void doc_init(Document *doc)
{
    doc->capacity = 4;
    doc->count    = 0;
    doc->lines    = malloc((size_t)doc->capacity * sizeof(char *));
    if (doc->lines == NULL) {
        fprintf(stderr, "Fatal: out of memory during initialization\n");
        exit(EXIT_FAILURE);
    }
}

/* Double the document's capacity by reallocating the lines array. */
void doc_grow(Document *doc)
{
    doc->capacity *= 2;
    doc->lines = realloc(doc->lines, (size_t)doc->capacity * sizeof(char *));
    if (doc->lines == NULL) {
        fprintf(stderr, "Fatal: out of memory during grow\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Insert a new line containing <text> at 1-indexed position <line_num>.
 * Valid positions are 1 through doc->count + 1 inclusive.
 * Lines at and after <line_num> are shifted right to make room.
 * Text longer than MAX_LINE_LEN - 1 characters is silently truncated.
 * Sets g_status to a success or error message.
 */
void doc_insert(Document *doc, int line_num, const char *text)
{
    if (line_num < 1 || line_num > doc->count + 1) {
        set_status("Error: line number %d out of range (valid: 1-%d)",
                   line_num, doc->count + 1);
        return;
    }

    if (doc->count == doc->capacity) {
        doc_grow(doc);
    }

    /* Shift existing lines right to open a slot at index line_num - 1. */
    for (int i = doc->count; i > line_num - 1; i--) {
        doc->lines[i] = doc->lines[i - 1];
    }

    /*
     * Always allocate MAX_LINE_LEN bytes so strncpy + null-terminator
     * never write past the end of the buffer.
     */
    char *new_line = malloc(MAX_LINE_LEN);
    if (new_line == NULL) {
        fprintf(stderr, "Fatal: out of memory during insert\n");
        exit(EXIT_FAILURE);
    }
    strncpy(new_line, text, MAX_LINE_LEN - 1);
    new_line[MAX_LINE_LEN - 1] = '\0';

    doc->lines[line_num - 1] = new_line;
    doc->count++;

    set_status("[+] Line %d inserted: \"%s\"", line_num, new_line);
}

/*
 * Delete the line at 1-indexed position <line_num>.
 * Sets g_status to a success or error message.
 * The freed slot is closed by shifting subsequent lines left.
 */
void doc_delete(Document *doc, int line_num)
{
    if (doc->count == 0) {
        set_status("Error: document is empty — nothing to delete");
        return;
    }

    if (line_num < 1 || line_num > doc->count) {
        set_status("Error: line number %d out of range (valid: 1-%d)",
                   line_num, doc->count);
        return;
    }

    /* Snapshot a preview of the deleted line for the status message. */
    char preview[48];
    strncpy(preview, doc->lines[line_num - 1], sizeof(preview) - 1);
    preview[sizeof(preview) - 1] = '\0';

    free(doc->lines[line_num - 1]);

    /* Shift remaining lines left to close the gap. */
    for (int i = line_num - 1; i < doc->count - 1; i++) {
        doc->lines[i] = doc->lines[i + 1];
    }

    doc->count--;

    set_status("[-] Line %d deleted: \"%s\"", line_num, preview);
}

/*
 * Build the document's numbered contents into the global status buffer.
 * Shows a placeholder if the document is empty.
 */
void doc_display(const Document *doc)
{
    if (doc->count == 0) {
        set_status("(document is empty)");
        return;
    }

    int pos = 0;
    for (int i = 0; i < doc->count; i++) {
        int n = snprintf(g_status + pos, (size_t)(STATUS_LEN - pos),
                         "  %d: %s\n", i + 1, doc->lines[i]);
        if (n < 0 || pos + n >= STATUS_LEN - 1) break;
        pos += n;
    }
    /* Trim trailing newline so the layout stays consistent. */
    if (pos > 0 && g_status[pos - 1] == '\n') {
        g_status[pos - 1] = '\0';
    }
}

/*
 * Search every line for the substring <query> using strstr.
 * Builds a list of matching line numbers and content previews into g_status.
 * The search is case-sensitive. Sets g_status to report all matches or
 * a "not found" message when there are none.
 */
void doc_search(const Document *doc, const char *query)
{
    if (doc->count == 0) {
        set_status("Search: document is empty");
        return;
    }

    if (query == NULL || query[0] == '\0') {
        set_status("Error: search query is empty");
        return;
    }

    int pos     = 0;
    int matches = 0;

    /* Write the header into g_status, then append each matching line. */
    int n = snprintf(g_status, STATUS_LEN,
                     "Search results for \"%s\":\n", query);
    if (n > 0 && n < STATUS_LEN) pos = n;

    for (int i = 0; i < doc->count; i++) {
        if (strstr(doc->lines[i], query) != NULL) {
            matches++;
            n = snprintf(g_status + pos, (size_t)(STATUS_LEN - pos),
                         "  Line %d: %s\n", i + 1, doc->lines[i]);
            if (n < 0 || pos + n >= STATUS_LEN - 1) break;
            pos += n;
        }
    }

    if (matches == 0) {
        set_status("Search: \"%s\" not found in any line", query);
    } else {
        /* Append the match count summary, trim trailing newline. */
        n = snprintf(g_status + pos, (size_t)(STATUS_LEN - pos),
                     "  (%d line%s matched)", matches, matches == 1 ? "" : "s");
        if (n > 0) pos += n;
        if (pos > 0 && g_status[pos - 1] == '\n') g_status[pos - 1] = '\0';
    }
}

/*
 * Replace all occurrences of <old> with <new_str> within <src>, writing
 * the result into <dst> (at most dst_size - 1 characters).
 * Returns the number of substitutions made.
 */
static int str_replace_all(const char *src, const char *old,
                            const char *new_str, char *dst, size_t dst_size)
{
    size_t old_len = strlen(old);
    size_t new_len = strlen(new_str);
    size_t pos     = 0;
    int    count   = 0;
    const char *cursor = src;
    const char *match;

    if (old_len == 0 || dst_size == 0) return 0;

    while ((match = strstr(cursor, old)) != NULL) {
        /* Copy the text before the match. */
        size_t prefix = (size_t)(match - cursor);
        if (pos + prefix >= dst_size - 1) {
            prefix = dst_size - 1 - pos;
        }
        memcpy(dst + pos, cursor, prefix);
        pos += prefix;
        if (pos >= dst_size - 1) break;

        /* Copy the replacement text. */
        size_t copy = (new_len < dst_size - 1 - pos) ? new_len : dst_size - 1 - pos;
        memcpy(dst + pos, new_str, copy);
        pos += copy;

        cursor = match + old_len;
        count++;
        if (pos >= dst_size - 1) break;
    }

    /* Copy whatever remains after the last match. */
    size_t tail = strlen(cursor);
    if (pos + tail >= dst_size) tail = dst_size - 1 - pos;
    memcpy(dst + pos, cursor, tail);
    dst[pos + tail] = '\0';

    return count;
}

/*
 * Replace all occurrences of <old_text> with <new_text>.
 * If line_num > 0, operates only on that 1-indexed line.
 * If line_num == 0 (the user typed '*'), operates on every line.
 * Reports total replacements and affected lines via g_status.
 */
void doc_replace(Document *doc, int line_num,
                 const char *old_text, const char *new_text)
{
    if (doc->count == 0) {
        set_status("Error: document is empty");
        return;
    }

    if (old_text[0] == '\0') {
        set_status("Error: find string cannot be empty");
        return;
    }

    char buf[MAX_LINE_LEN];
    int total = 0;  /* total substitutions */
    int changed = 0; /* lines that had at least one substitution */

    if (line_num == 0) {
        /* Global replace — scan every line. */
        for (int i = 0; i < doc->count; i++) {
            int n = str_replace_all(doc->lines[i], old_text, new_text,
                                    buf, MAX_LINE_LEN);
            if (n > 0) {
                strncpy(doc->lines[i], buf, MAX_LINE_LEN - 1);
                doc->lines[i][MAX_LINE_LEN - 1] = '\0';
                total += n;
                changed++;
            }
        }
        if (total == 0) {
            set_status("Replace: \"%s\" not found in any line", old_text);
        } else {
            set_status("[~] Replaced %d occurrence%s across %d line%s",
                       total,   total   == 1 ? "" : "s",
                       changed, changed == 1 ? "" : "s");
        }
    } else {
        /* Single-line replace. */
        if (line_num < 1 || line_num > doc->count) {
            set_status("Error: line number %d out of range (valid: 1-%d)",
                       line_num, doc->count);
            return;
        }
        int n = str_replace_all(doc->lines[line_num - 1], old_text, new_text,
                                buf, MAX_LINE_LEN);
        if (n == 0) {
            set_status("Replace: \"%s\" not found on line %d", old_text, line_num);
        } else {
            strncpy(doc->lines[line_num - 1], buf, MAX_LINE_LEN - 1);
            doc->lines[line_num - 1][MAX_LINE_LEN - 1] = '\0';
            set_status("[~] Replaced %d occurrence%s on line %d",
                       n, n == 1 ? "" : "s", line_num);
        }
    }
}

/* Free every line string and the lines array itself. */
void doc_free(Document *doc)
{
    for (int i = 0; i < doc->count; i++) {
        free(doc->lines[i]);
    }
    free(doc->lines);
    doc->lines    = NULL;
    doc->count    = 0;
    doc->capacity = 0;
}

/* Print the command reference menu as a fixed-width box. */
void print_menu(void)
{
    printf("+------------------------------------------+\n");
    printf("|          SIMPLE LINE EDITOR              |\n");
    printf("+------------------------------------------+\n");
    printf("| i <num> <text>  Insert text at line <num>|\n");
    printf("|   e.g.  i 1 Hello, world!                |\n");
    printf("|                                          |\n");
    printf("| d <num>         Delete line at <num>     |\n");
    printf("|   e.g.  d 2                              |\n");
    printf("|                                          |\n");
    printf("| p               Print the document       |\n");
    printf("|                                          |\n");
    printf("| s <query>       Search for word/phrase   |\n");
    printf("|   e.g.  s hello                          |\n");
    printf("|                                          |\n");
    printf("| r <n|*> <old>/<new>  Find & replace      |\n");
    printf("|   e.g.  r 2 old/new  or  r * old/new    |\n");
    printf("|                                          |\n");
    printf("| q               Quit the editor          |\n");
    printf("+------------------------------------------+\n");
}

/*
 * Clear the terminal, redraw the menu, and show the current status message.
 * The input prompt is printed separately so the cursor lands right after it.
 */
static void redraw(void)
{
    system("cls");
    print_menu();
    printf("\n");
    printf("  Status: %s\n", g_status);
    printf("\n\n");
}

int main(void)
{
    Document doc;
    doc_init(&doc);
    set_status("Ready -- enter a command below.");

    char input[MAX_LINE_LEN + 32]; /* extra room for the command prefix */

    while (1) {
        redraw();
        printf("> ");
        fflush(stdout);

        if (fgets(input, (int)sizeof(input), stdin) == NULL) {
            /* EOF (Ctrl-Z on Windows, Ctrl-D on Unix) — treat as quit. */
            break;
        }

        /* Strip the trailing newline, if present. */
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
            len--;
        }

        /* Skip blank input — just redraw without changing the status. */
        if (len == 0) {
            continue;
        }

        char cmd = input[0];

        if (cmd == 'p') {
            doc_display(&doc);

        } else if (cmd == 'q') {
            doc_free(&doc);
            break;

        } else if (cmd == 'd') {
            /* Expected format: "d <line_num>" */
            int line_num;
            char extra;
            int parsed = sscanf(input + 1, " %d %c", &line_num, &extra);
            if (parsed < 1) {
                set_status("Error: missing argument -- usage: d <num>");
            } else {
                doc_delete(&doc, line_num);
            }

        } else if (cmd == 'i') {
            /*
             * Expected format: "i <line_num> <text>"
             * Use strtol to parse the number, then treat the rest as text.
             */
            char *rest = input + 1;

            /* Skip whitespace before line number. */
            while (*rest == ' ' || *rest == '\t') {
                rest++;
            }

            if (*rest == '\0') {
                set_status("Error: missing arguments -- usage: i <num> <text>");
                continue;
            }

            /* Parse the line number. */
            char *endptr;
            long line_num_l = strtol(rest, &endptr, 10);
            if (endptr == rest) {
                set_status("Error: invalid line number -- usage: i <num> <text>");
                continue;
            }

            /* Skip whitespace between line number and text. */
            while (*endptr == ' ' || *endptr == '\t') {
                endptr++;
            }

            if (*endptr == '\0') {
                set_status("Error: missing text -- usage: i <num> <text>");
                continue;
            }

            doc_insert(&doc, (int)line_num_l, endptr);

        } else if (cmd == 's') {
            /* Expected format: "s <query>" — everything after 's ' is the search term. */
            char *query = input + 1;

            /* Skip leading whitespace. */
            while (*query == ' ' || *query == '\t') {
                query++;
            }

            if (*query == '\0') {
                set_status("Error: missing query -- usage: s <word or phrase>");
            } else {
                doc_search(&doc, query);
            }

        } else if (cmd == 'r') {
            /*
             * Expected format: "r <line_num|*> <old>/<new>"
             * line_num is a 1-based integer, or '*' meaning all lines.
             * The first '/' in the text portion separates old from new.
             */
            char *rest = input + 1;
            while (*rest == ' ' || *rest == '\t') rest++;

            if (*rest == '\0') {
                set_status("Error: usage: r <num|*> <old>/<new>");
                continue;
            }

            /* Parse line number or '*'. */
            int target_line;
            char *after_num;
            if (*rest == '*') {
                target_line = 0; /* 0 means all lines */
                after_num = rest + 1;
            } else {
                char *endptr;
                long ln = strtol(rest, &endptr, 10);
                if (endptr == rest) {
                    set_status("Error: expected line number or '*' -- usage: r <num|*> <old>/<new>");
                    continue;
                }
                target_line = (int)ln;
                after_num = endptr;
            }

            /* Skip whitespace between number and text pair. */
            while (*after_num == ' ' || *after_num == '\t') after_num++;

            if (*after_num == '\0') {
                set_status("Error: missing <old>/<new> -- usage: r <num|*> <old>/<new>");
                continue;
            }

            /* Split on the first '/' to get old_text and new_text. */
            char *slash = strchr(after_num, '/');
            if (slash == NULL) {
                set_status("Error: missing '/' separator -- usage: r <num|*> <old>/<new>");
                continue;
            }

            *slash = '\0';              /* terminate old_text in-place */
            const char *old_text = after_num;
            const char *new_text = slash + 1;

            if (old_text[0] == '\0') {
                set_status("Error: find string cannot be empty");
                continue;
            }

            doc_replace(&doc, target_line, old_text, new_text);

        } else {
            set_status("Error: unknown command '%c' -- use i, d, p, s, r, or q", cmd);
        }
    }

    return 0;
}
