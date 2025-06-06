# ve

visual editor

## build

To build the project and use the editor, run the following commands

```sh
make
./bin/ve
```

To cleanup run the following command

```sh
make clean
```

## Support features

- Prompt command support
	- `:hello`: print hello world to the status bar
	- `:discard`: discard the file changes
	- `:quit`: quit editor but makes sure that content is saved
	- `:saveas`: change the name of the file
	- `:read`: read content of a file to the editing file
	- `:write`: save the content to a file
- Basic vim motions
	- `i`: insert mode
	- `h`: move cursor left
	- `j`: move cursor down
	- `k`: move cursor up
	- `l`: move cursor right
	- `w`: move cursor by word
	- `W`: move cursor by WORD
	- `$`: move cursor end of file
	- `0`: move cursor start of file
