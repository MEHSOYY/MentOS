/// @file video.c
/// @brief Video functions and costants.
/// @copyright (c) 2014-2024 This file is distributed under the MIT License.
/// See LICENSE.md for details.

#include "io/port_io.h"
#include "ctype.h"
#include "io/vga/vga.h"
#include "io/video.h"
#include "io/debug.h"
#include "stdbool.h"
#include "stdio.h"
#include "string.h"

#define HEIGHT       25                   ///< The height of the
#define WIDTH        80                   ///< The width of the
#define W2           (WIDTH * 2)          ///< The width of the
#define TOTAL_SIZE   (HEIGHT * WIDTH * 2) ///< The total size of the screen.
#define ADDR         (char *)0xB8000U     ///< The address of the
#define STORED_PAGES 3                    ///< The number of stored pages.

/// @brief Stores the association between ANSI colors and pure VIDEO colors.
struct ansi_color_map_t {
    /// The ANSI color number.
    uint8_t ansi_color;
    /// The VIDEO color number.
    uint8_t video_color;
}
/// @brief The mapping.
ansi_color_map[] = {
    { 0, 7 },

    { 30, 0 },
    { 31, 4 },
    { 32, 2 },
    { 33, 6 },
    { 34, 1 },
    { 35, 5 },
    { 36, 3 },
    { 37, 7 },

    { 90, 8 },
    { 91, 12 },
    { 92, 10 },
    { 93, 14 },
    { 94, 9 },
    { 95, 13 },
    { 96, 11 },
    { 97, 15 },

    { 40, 0 },
    { 41, 4 },
    { 42, 2 },
    { 43, 6 },
    { 44, 1 },
    { 45, 5 },
    { 46, 3 },
    { 47, 7 },

    { 100, 8 },
    { 101, 12 },
    { 102, 10 },
    { 103, 14 },
    { 104, 9 },
    { 105, 13 },
    { 106, 11 },
    { 107, 15 }
};

/// Pointer to a position of the screen writer.
char *pointer = ADDR;
/// The current color.
unsigned char color = 7;
/// Used to write on the escape_buffer. If -1, we are not parsing an escape sequence.
int escape_index = -1;
/// Used to store an escape sequence.
char escape_buffer[256];
/// Buffer where we store the upper scroll history.
char upper_buffer[STORED_PAGES * TOTAL_SIZE] = { 0 };
/// Buffer where we store the lower scroll history.
char original_page[TOTAL_SIZE] = { 0 };
/// Determines if the screen is currently scrolled.
int scrolled_page = 0;

/// @brief Get the current column number.
/// @return The column number.
static inline unsigned __get_x(void)
{
    return ((pointer - ADDR) % (WIDTH * 2)) / 2;
}

/// @brief Get the current row number.
/// @return The row number.
static inline unsigned __get_y(void)
{
    return (pointer - ADDR) / (WIDTH * 2);
}

/// @brief Draws the given character.
/// @param c The character to draw.
static inline void __draw_char(char c)
{
    for (char *ptr = (ADDR + TOTAL_SIZE + (WIDTH * 2)); ptr > pointer; ptr -= 2) {
        *(ptr)     = *(ptr - 2);
        *(ptr + 1) = *(ptr - 1);
    }
    *(pointer++) = c;
    *(pointer++) = color;
}

/// @brief Hides the VGA cursor.
void __video_hide_cursor(void)
{
    outportb(0x3D4, 0x0A);
    unsigned char cursor_start = inportb(0x3D5);
    outportb(0x3D5, cursor_start | 0x20); // Set the most significant bit to disable the cursor.
}

/// @brief Shows the VGA cursor.
void __video_show_cursor(void)
{
    outportb(0x3D4, 0x0A);
    unsigned char cursor_start = inportb(0x3D5);
    outportb(0x3D5, cursor_start & 0xDF); // Clear the most significant bit to enable the cursor.
}

/// @brief Sets the VGA cursor shape by specifying the start and end scan lines.
///
/// @param start The starting scan line of the cursor (0-15).
/// @param end The ending scan line of the cursor (0-15).
void __video_set_cursor_shape(unsigned char start, unsigned char end)
{
    // Set the cursor's start scan line
    outportb(0x3D4, 0x0A);
    outportb(0x3D5, start);

    // Set the cursor's end scan line
    outportb(0x3D4, 0x0B);
    outportb(0x3D5, end);
}

/// @brief Issue the vide to move the cursor to the given position.
/// @param x The x coordinate.
/// @param y The y coordinate.
static inline void __video_set_cursor_position(unsigned int x, unsigned int y)
{
    uint32_t position = y * WIDTH + x;
    // Cursor LOW port to VGA index register.
    outportb(0x3D4, 0x0F);
    outportb(0x3D5, (uint8_t)(position & 0xFFU));
    // Cursor HIGH port to VGA index register.
    outportb(0x3D4, 0x0E);
    outportb(0x3D5, (uint8_t)((position >> 8U) & 0xFFU));
}

/// @brief Retrieves the current VGA cursor position in terms of x and y coordinates.
///
/// @param x Pointer to store the x-coordinate (column).
/// @param y Pointer to store the y-coordinate (row).
static inline void __video_get_cursor_position(unsigned int *x, unsigned int *y)
{
    uint16_t position;

    // Get the low byte of the cursor position.
    outportb(0x3D4, 0x0F);
    position = inportb(0x3D5);
    // Get the high byte of the cursor position.
    outportb(0x3D4, 0x0E);
    position |= ((uint16_t)inportb(0x3D5)) << 8;
    // Calculate x and y.
    if (x) *x = position % WIDTH;
    if (y) *y = position / WIDTH;
}

/// @brief Sets the provided ansi code.
/// @param ansi_code The ansi code describing background and foreground color.
static inline void __set_color(uint8_t ansi_code)
{
    for (size_t i = 0; i < count_of(ansi_color_map); ++i) {
        if (ansi_code == ansi_color_map[i].ansi_color) {
            if (
                (ansi_code == 0) ||
                ((ansi_code >= 30) && (ansi_code <= 37)) ||
                ((ansi_code >= 90) && (ansi_code <= 97))) {
                color = (color & 0xF0U) | ansi_color_map[i].video_color;
            } else {
                color = (color & 0x0FU) | (ansi_color_map[i].video_color << 4U);
            }
            break;
        }
    }
}

/// @brief Moves the cursor backward.
/// @param erase  If 1 also erase the character.
/// @param amount How many times we move backward.
static inline void __move_cursor_backward(int erase, int amount)
{
    for (int i = 0; i < amount; ++i) {
        // Bring back the pointer.
        pointer -= 2;
        if (erase) {
            strcpy(pointer, pointer + 2);
        }
    }
    video_update_cursor_position();
}

/// @brief Moves the cursor forward.
/// @param erase  If 1 also erase the character.
/// @param amount How many times we move forward.
static inline void __move_cursor_forward(int erase, int amount)
{
    for (int i = 0; i < amount; ++i) {
        // Bring forward the pointer.
        if (erase) {
            __draw_char(' ');
        } else {
            pointer += 2;
        }
    }
    video_update_cursor_position();
}

/// @brief Parses the cursor shape escape code and sets the cursor shape accordingly.
/// @param shape The integer representing the cursor shape code.
static inline void __parse_cursor_escape_code(int shape)
{
    switch (shape) {
    case 0: // Default blinking block cursor
    case 2: // Blinking block cursor
        __video_set_cursor_shape(0, 15);
        break;
    case 1: // Steady block cursor
        __video_set_cursor_shape(0, 15);
        break;
    case 3: // Blinking underline cursor
        __video_set_cursor_shape(13, 15);
        break;
    case 4: // Steady underline cursor
        __video_set_cursor_shape(13, 15);
        break;
    case 5: // Blinking vertical bar cursor
        __video_set_cursor_shape(0, 1);
        break;
    case 6: // Steady vertical bar cursor
        __video_set_cursor_shape(0, 1);
        break;
    default:
        // Handle any other cases if needed
        break;
    }
}

void video_init(void)
{
    video_clear();
    __parse_cursor_escape_code(0);
}

void video_update(void)
{
#ifndef VGA_TEXT_MODE
    if (vga_is_enabled())
        vga_update();
#endif
}

void video_putc(int c)
{
    // ESCAPE SEQUENCES
    if (c == '\033') {
        escape_index = 0;
        return;
    }
    if (escape_index >= 0) {
        if ((escape_index == 0) && (c == '[')) {
            return;
        }
        escape_buffer[escape_index++] = c;
        escape_buffer[escape_index]   = 0;
        if (isalpha(c)) {
            escape_buffer[--escape_index] = 0;

            // Move cursor forward (e.g., ESC [ <num> C)
            if (c == 'C') {
                __move_cursor_forward(false, atoi(escape_buffer));
            }
            // Move cursor backward (e.g., ESC [ <num> D)
            else if (c == 'D') {
                __move_cursor_backward(false, atoi(escape_buffer));
            }
            // Set color (e.g., ESC [ <num> m)
            else if (c == 'm') {
                __set_color(atoi(escape_buffer));
            }
            // Clear screen (e.g., ESC [ <num> J)
            else if (c == 'J') {
                video_clear();
            }
            // Clear screen (e.g., ESC [ <num> J)
            else if (c == 'H') {
                char *semicolon = strchr(escape_buffer, ';');
                if (semicolon != NULL) {
                    *semicolon     = '\0';
                    unsigned int y = atoi(escape_buffer);
                    unsigned int x = atoi(semicolon + 1);
                    pointer        = ADDR + ((y - 1) * WIDTH * 2 + (x - 1) * 2);
                } else {
                    pointer = ADDR;
                }
                video_update_cursor_position();
            }
            // Handle cursor shape (e.g., ESC [ <num> q)
            else if (c == 'q') {
                __parse_cursor_escape_code(atoi(escape_buffer));
            }
            escape_index = -1;
        }
        return;
    }

#ifndef VGA_TEXT_MODE
    if (vga_is_enabled()) {
        vga_putc(c);
        return;
    }
#endif

    // == NORMAL CHARACTERS =======================================================================
    // If the character is '\n' go the new line.
    if (c == '\n') {
        video_new_line();
        //video_shift_one_line_down();
    } else if (c == '\b') {
        __move_cursor_backward(true, 1);
    } else if (c == '\r') {
        video_cartridge_return();
    } else if (c == 127) {
        strcpy(pointer, pointer + 2);
    } else if ((c >= 0x20) && (c <= 0x7E)) {
        __draw_char(c);
    } else {
        return;
    }

    video_shift_one_line_up();
    video_update_cursor_position();
}

void video_puts(const char *str)
{
#ifndef VGA_TEXT_MODE
    if (vga_is_enabled()) {
        vga_puts(str);
        return;
    }
#endif
    while ((*str) != 0) {
        video_putc((*str++));
    }
}

void video_update_cursor_position(void)
{
#ifndef VGA_TEXT_MODE
    if (vga_is_enabled())
        return;
#endif
    __video_set_cursor_position(
        ((pointer - ADDR) / 2U) % WIDTH,
        ((pointer - ADDR) / 2U) / WIDTH);
}

void video_move_cursor(unsigned int x, unsigned int y)
{
#ifndef VGA_TEXT_MODE
    if (vga_is_enabled()) {
        vga_move_cursor(x, y);
        return;
    }
#endif
    pointer = ADDR + ((y * WIDTH * 2) + (x * 2));
    video_update_cursor_position();
}

void video_get_cursor_position(unsigned int *x, unsigned int *y)
{
#ifndef VGA_TEXT_MODE
    if (vga_is_enabled()) {
        vga_get_cursor_position(x, y);
        return;
    }
#endif
    if (x) {
        *x = __get_x();
    }
    if (y) {
        *y = __get_y();
    }
}

void video_get_screen_size(unsigned int *width, unsigned int *height)
{
#ifndef VGA_TEXT_MODE
    if (vga_is_enabled()) {
        vga_get_screen_size(width, height);
        return;
    }
#endif
    if (width) {
        *width = WIDTH;
    }
    if (height) {
        *height = HEIGHT;
    }
}

void video_clear(void)
{
#ifndef VGA_TEXT_MODE
    if (vga_is_enabled()) {
        vga_clear_screen();
        return;
    }
#endif
    memset(upper_buffer, 0, STORED_PAGES * TOTAL_SIZE);
    memset(ADDR, 0, TOTAL_SIZE);
}

void video_new_line(void)
{
#ifndef VGA_TEXT_MODE
    if (vga_is_enabled()) {
        vga_new_line();
        return;
    }
#endif
    pointer = ADDR + ((pointer - ADDR) / W2 + 1) * W2;
    video_shift_one_line_up();
    video_update_cursor_position();
}

void video_cartridge_return(void)
{
#ifndef VGA_TEXT_MODE
    if (vga_is_enabled()) {
        vga_new_line();
        return;
    }
#endif
    pointer = ADDR + ((pointer - ADDR) / W2 - 1) * W2;
    video_new_line();
    video_shift_one_line_up();
    video_update_cursor_position();
}

void video_shift_one_line_up(void)
{
    if (pointer >= ADDR + TOTAL_SIZE) {
        // Move the upper buffer up by one line.
        for (int row = 0; row < (STORED_PAGES * HEIGHT); ++row) {
            memcpy(upper_buffer + W2 * row, upper_buffer + W2 * (row + 1), 2 * WIDTH);
        }
        // Copy the first line on the screen inside the last line of the upper buffer.
        memcpy(upper_buffer + (TOTAL_SIZE * STORED_PAGES - W2), ADDR, 2 * WIDTH);
        // Move the screen up by one line.
        for (int row = 0; row < HEIGHT; ++row) {
            memcpy(ADDR + (W2 * row), ADDR + (W2 * (row + 1)), 2 * WIDTH);
        }
        // Update the pointer.
        pointer = ADDR + ((pointer - ADDR) / W2 - 1) * W2;
    }
}

void video_shift_one_page_up(void)
{
    if (scrolled_page > 0) {
        // Decrese the number of scrolled pages, and compute which page must be loaded.
        int page_to_load = (STORED_PAGES - (--scrolled_page));
        // If we have reached 0, restore the original page.
        if (scrolled_page == 0) {
            memcpy(ADDR, original_page, TOTAL_SIZE);
        } else {
            memcpy(ADDR, upper_buffer + (page_to_load * TOTAL_SIZE), TOTAL_SIZE);
        }
    }
}

void video_shift_one_page_down(void)
{
    if (scrolled_page < STORED_PAGES) {
        // Increase the number of scrolled pages, and compute which page must be loaded.
        int page_to_load = (STORED_PAGES - (++scrolled_page));
        // If we are loading the first history page, save the original.
        if (scrolled_page == 1) {
            memcpy(original_page, ADDR, TOTAL_SIZE);
        }
        // Load the specific page.
        memcpy(ADDR, upper_buffer + (page_to_load * TOTAL_SIZE), TOTAL_SIZE);
    }
}

#if 0
void video_scroll_up(void)
{
    if (is_scrolled)
        return;
    char *ptr = memory = pointer;
    for (unsigned int it = 0; it < TOTAL_SIZE; it++) {
        downbuffer[y][x] = *ptr;
        *ptr++           = upbuffer[y][x];
    }

    is_scrolled = true;

    stored_x = video_get_column();

    stored_y = video_get_line();

    video_move_cursor(width, height);
}

void video_scroll_down(void)
{
    char *ptr = memory;

    // If PAGEUP hasn't been pressed, it's useless to go down, there is nothing.
    if (!is_scrolled) {
        return;
    }
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width * 2; x++) {
            *ptr++ = downbuffer[y][x];
        }
    }

    is_scrolled = false;

    video_move_cursor(stored_x, stored_y);

    stored_x = 0;

    stored_y = 0;
}

#endif
