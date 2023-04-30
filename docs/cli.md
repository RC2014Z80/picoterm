# Picoterm CLI (_Command Line Interpreter_)

Picoterm do includes a command line interpreter used for provides extra services/features to the picoterm firmware.

# Commands

This section contains the list of commands availables in the CLI.

Commands syntax are the following

`command required_param1 [optional_param2] [-flag]`

* Optional parameter are showed between brackets.
* Required parameter parameter are listed without bracket.
* Flags are:
	* always optional
	* marked with dash "-"
	* always added at the end of the command.

## dir

`dir [path] [-p]`

List the files availables on the SDCard (either the root, either a sub-path).

The `-p` flag can be used to display the list of files per page!

Examples:
* __dir__ : list the files at root.
* __dir z81__ : list files in the z81 directory.
* __dir -p__ : list files at root (par page).

## sd_info

`sd_info`

Try to mount the SDCard and display informations about the identified filesystem.

## type

`type [filename] [-p]`

Display the content of a file on the screen. The `-p` flag can be used to display the list of files per page!
