# ExecIco
ExecIco is a command-line utility for creating an object file from an icon (.ico) file. These files can then be embedded into a Windows C executable during the compilation process, so that the icon shows as the executables icon.

The tool takes an icon file path as input, processes it to create a resource file (.rc), and then compiles the resource file into an object file (icon.o) using the windres tool, which is part of the MinGW suite.

## Features
- Supports both absolute and relative paths for the input icon file.
- Verifies that the icon file exists before proceeding.
- Automatically generates the resource file (icon.rc) and compiles it into an object file (icon.o).

## Requirements
- MinGW (specifically the windres tool to compile the resource file into an object file).
- Microsoft Windows (for the use of Windows-specific functions).

## Usage
### Command-line Syntax
```bash
./execico <*.ico file path>
```
#### Arguments
<*.ico file path>: The path to the .ico file that you want to convert into an object file.

#### Example - Absolute
```bash
./execico C:\\path\\to\\icon.ico
```

#### Example - Relative
```bash
./execico icon.ico
```