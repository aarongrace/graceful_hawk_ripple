# Graceful Hawk

Graceful Hawk extends the base Hawk emulator by Dr. Douglas Jones into a fully animated, color-themed, and dynamically reactive terminal environment. It replaces the static monochrome interface with a modular theming engine, animated banners, and real-time color modulation synchronized with the emulator's internal state.

Most of the functionalities are implemented in C (`graceful_hawk.c` / `graceful_hawk.h`) but I also modified the display loop in `console.c`.

---

## Core Features

### 1. Runtime Theme Management

Maintains a global theme struct that defines RGB color sets for interface components (title, registers, memory, menu, CPU line, etc.).

**Supports multiple themes:**

- **t_default** – balanced blue/purple base
- **t_desert** – golden, warm palette
- **t_ocean** – cool blue-green palette
- **t_meadows** – fresh green-turquoise palette
- **t_crimson** – deep red-violet palette

**Each theme defines:**

- Fixed colors for interface elements
- Gradients for "colorful number" visualization (`n_mod0` … `n_mod3`)

**Major functions:**

- `init_themes_and_color_pairs()` – initializes all color pairs and sets the default theme
- `change_theme(int theme)` – switches between themes (cyclic or explicit)
- `start_theme(int t)` – applies the RGB definitions to curses color pairs

### 2. Animated Banners

Graceful Hawk includes an animated banner system rendered across the terminal header. Each banner can cycle colors and characters to simulate flowing gradients or alternating symbols.

**Banner modes:**

- **bs_disco** – shifting, saturated colors with flashing `$` / `\` characters
- **bs_gradient** – smooth color blending across tints and hues (`~` / `\`)
- **bs_alternating** – structured two-tone pulses using vertical bars (`|` `-` `-`)

**Major functions:**

- `set_banner_style(int incre)` – changes the current banner mode and re-initializes its colors
- `set_banner_colors()` – animates the banner by rotating its color indices each frame

### 3. Colorful Number Rendering

Registers, memory cells, and monitor digits can be displayed as color-coded hexadecimal values using gradient palettes that emphasize visual contrast.

Each 4-bit nibble (0–F) is mapped to a dynamic color pair, interpolated through the current theme's modulation vectors.

### 4. Dynamic Modulation

Each theme includes RGB modulation factors (`mr`, `mg`, `mb`) that control how other visual systems (e.g., ripple rendering) inherit the active color profile. The color modulation affects banner gradients, numeric visualization, and cross-fade animations.


------

Please see the original README of the Hawk emulator by Dr. Douglas Jones below. 

# Hawk

The Hawk emulator is an emulator for a 32-bit RISC machine written in C
designed to run under Linux.  It has been in use since 1996 as a tool
for teaching introductory computer organization and assembly language
programming.

## Installation

Use `make compiler=cc` from the command line.  This will read `Makefile`
and produce an executable named `hawk`.  You can set compler options here,
for example, use `make 'compiler=cc -O'` to make the compiler generate
more optimal code.

Variables set in the makefile itself allow you to set the amount of RAM
and ROM on your emulated machine, and they allow you to select between
a full configuration of the Hawk emulator and the Sparrowhawk subset.

For more detail, see `Makefile`.  Note:  The Hawk emulator uses the
`curses` library.  If this is not installed on your system, you will need
to get it before using Make.

## Use

From the command line, type `hawk testfile` to launch the Hawk emulator
with `testfile` loaded in memory (RAM, ROM or both).  The emulator always
starts in halt state with the program counter set to zero.

The test file supplied with this distribution contains the scrambled fragments
of a limerick.  When run, the program sorts the fragments into order and
outputs the resulting limerick.  To produce additional object files, you
will need to use the SMAL assembler.

Multiple object files may be specified on the command line; all of them
will be loaded in the order specified.  If any of the files load the same
memory location, they will overlay each other, leftmost file first.

The object code format expected is that produced by the SMAL assembler
or linker.

Once the loading is complete, the emulator will display the CPU state in the
top of the terminal window, leaving the bottom mapped to the emulator's
video RAM.  The terminal window may not be resized after the emulator is
launched.

When halted, the emulator responds to single letter commands.  These are
described in a brief one-line help message.  The commands are:

* **`r`** -- run until _pc_ = _0_ or until _pc_ = _break_
* **`s`** -- single step (follows branches and calls)
* **`n`** -- set _break_ to next instruction, run (e.g. until subroutine return)
* **`q`** -- quit
* **hexadecimal digits** -- shifted into _n_, a command parameter
* **`m`** -- show memory around address _n_
* **`+`** -- scroll the memory display to increasing addresses
* **`-`** -- scroll the memory display to decreasing addresses
* **`t`** -- toggle the memory display between hexadecimal and disassembly
* **`p`** -- set _break_ = _n_ and run
* **`>`** -- increment _break_
* **`<`** -- decrement _break_
* **`i`** -- set _break_ = _pc_ and run (typically one loop iteration)
* **`z`** -- set _refresh_ = _n_ (memory cycles between display updates)

## Files in this distribution

* `README`     -- this file
* `Makefile`   -- used to make the emulator (includes brief instructions)
* `bus.h`      -- the "communication bus" between emulator components
* `irfields.h` -- instruction register field definitions
* `cpu.c`      -- the emulator CPU -- the main program
* `console.h`
* `console.c`  -- the console interface for the emulator
* `float.h`
* `float.c`    -- the floating point coprocessor
* `powerup.h`
* `powerup.c`  -- code to "power up" the emulator (includes the loader)
* `showop.h`
* `showop.c`   -- support for symbolic dumps of Hawk object code
* `testfile`   -- a loadable object file to demonstrate the emulator

## Related Material

* [http://www.cs.uiowa.edu/~dwjones/arch/hawk] -- the Hawk CPU Manual
* [http://www.cs.uiowa.edu/~dwjones/assem/notes] -- course notes
* [http://www.cs.uiowa.edu/~dwjones/cross/SMAL32] -- the SMAL assembler manual
* [http://www.cs.uiowa.edu/~dwjones/arch/hawk/headers] -- SMAL Hawk header files
* [http://www.cs.uiowa.edu/~dwjones/arch/hawk/monitor.txt] -- the SMAL Hawk
  monitor and subroutine library

Note that the Hawk emulator's loadable object file format is fully documented
in Section 9.1 of the Hawk CPU manual.

## Author

Douglas W. Jones

[http://www.cs.uiowa.edu/~dwjones/]
```
 Department of Computer Science
 University of Iowa
 Iowa City, IA  52242
 USA
```

## Copyright

All of the files included here may be redistributed freely, and there
are no restrictions on alteration and additional redistribution so long as:

* the original author or authors are given credit for their contributions.
* any altered files contain brief revision notes explaining the alteration.
* any altered files identify the author of the revision.
* this file is included in the redistribution, with all authors identified.
* these terms are preserved in the redistribution.

## Revision history

* 9 Nov. 2023  -- DWJ created new `README` file.
* 11 Dec. 2023 -- DWJ added keyboard interrupt and CPU-polite KBDSTAT polling.
