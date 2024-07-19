# Inkognito

## Description
Inkognito takes in a message (or a text file) and encrypts the message and stores the data in a image file.  

> [!WARNING]
> The project is still in development and some encryption methods are not yet implemented.

## Usage

```sh
gcc -O3 ./main.c -o ./inkognito.exe
./inkognito.exe [options]
```

<details>

<summary>Command Line Arguments</summary>

--version:		get version
-e:			encode [default]
-d:			decode
-i <message|filename>:	input string or a file
-o <filename>:		output image file [default: ./output.bmp]
-r <int>:		rotate ascii in input text [default: random]
-m <seed|filename>:	mix with specific base64 seed or image [optional]
-p <int>:		pixel minimum value to be true [default: 128]

|	Argument	|	Description				|	Notes			|
|	:---:		|	:---:					|	:---:			|
|	-h|--help	|	Display help message			|				|
|	--version	|	Display version				|				|
|	-e		|	Encode					|	Default			|
|	-d		|	Decode					|				|
|	-i		|	Input string or a file			|				|
|	-o		|	Output image file			|	Default: ./output.bmp	|
|	-r		|	Rotate ascii in input text		|	Default: random		|
|	-m		|	Mix with specific base64 seed or image	|	Optional		|
|	-p		|	Pixel minimum value to be true		|	Default: 128		|

</details>