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

The `-p` flag can be used to display the list of files per page! Press ESC to cancel.

Examples:
* __dir__ : list the files at root.
* __dir z81__ : list files in the z81 directory.
* __dir -p__ : list files at root (par page).
* __dir /__ : force to read the files at the root.

## sd_info

`sd_info`

Try to mount the SDCard and display informations about the identified filesystem.

## send_file

`send_file filename`

Can be used to send a file content (.hex) directly on the UART. UART responses are received in the CLI and executed as command (at least attempt of execution).

It is a great way to send to upload [SCM Apps](https://smallcomputercentral.com/scm-apps/) and run it on your SCM (Small Computer Monitor) prompt.

It is exactly what shown the example here below.

```
---- Picoterm Command interpreter ----
Use: exit, list, <command> ?

$ dir
----    5435 config.txt
D---       0 cf_info/
D---       0 memtest/

$ dir memtest/
----     823 memtest/SCM_MemTest_v2.0_Z80.hex
----    6609 memtest/SCM_MemTest_v2.0_Z80.asm
----    1224 memtest/ReadMe.txt.hex

$ send_file memtest/scm_memtest_v2.0_z80.hex

823 / 823 sent!
$ Ready
Ready ???
$

$ *
* ???
$ exit
```

Remarks: _The "Ready" and "*" command seen after the end of file transfer are the responses sent by the SCM as response to the HEX upload. They are coming on the serial line (and interpreted as CLI command, so producing some error messages).

With the __exit__ command, the CLI exits and returns to the terminal mode (showing SCM running on the RC2014).

Now, we are in SCM and we do need to execute the uploaded HEX file with __G8000__ .

```
*
* G8000
Z80 64k memory test v2.0 by Stephen C Cousins
Upper 32k RAM: Pass
Lower 32k RAM: Pass
ROM page out test: Pass
*
```

The send_file command can also be used with some [basic program ](https://github.com/RC2014Z80/RC2014-BASIC-Programs) when the basic is already started.

## type

`type [filename] [-p]`

Display the content of a file on the screen. The `-p` flag can be used to display the list of files per page! Press ESC to cancel.
