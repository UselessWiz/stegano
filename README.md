# Stegano
A simple image staganography program for encoding short messages into BMP files, written in C.

# Features
- Encodes and decodes messages into bitmap (.bmp) images.
- Message compression before encoding and decompression after decoding.
- Input a message from a text file, or output a decoded message to a text file.
- Uses a library, which can work as a standalone tool (stegano.h).
- Recalls recently accessed files.
- Warns users if specified files aren’t .bmp images in the correct format.

# Usage
```
-e: Encodes a message
-d: Decodes a message
-i [file]: takes the given file as input. If -e is passed, encodes using the text in this file.
If -d is passed, decodes from this image file.
-o [file]: takes the given file as output. If -e is passed, encodes text into this image
file. If -d is passed, places the message into this text file.
-m [message]: encodes ‘message’ into an image.
If no flags are passed, the program should enter an interactive mode where all operations
can be conducted within a user interface.
```
