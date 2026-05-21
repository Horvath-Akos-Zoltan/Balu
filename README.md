# [WIP] Important: This language does not yet compile, it only has a lexer and a parser
#
# The Balu Programming Language
### Tired of constantly getting compile-time errors?
### Try out the Balu programming language! It fixes itself, so your HTML brain can rest easy.
### You don't want to learn a new syntax? We support multiple kinds of syntaxes! (e.g. [Variable declaration](#variable-declaration))

## What is this?
### The Balu programming language is a gradually typed, garbage collected language

## How to install?
### It's a singular file, so just download it and put it in any folder you'd like

## How to use?
### Windows: `.\main.exe filename.balu`
### Currently only Windows is supported, Win10 is used for testing
### To ignore the errors, just add the `-np`, or `--no-panic` flag in the command line
### For help, add the `-h`, `--help`, `-?`, `/help`, `/h` or `/help` flag in the command line

# Types
### The Balu language is gradually typed, meaning it CAN type statically (give strict types), but you can also create dynamic variables (like python, or javascript)
## Numbers:
### byte:
- 0-255
- uses 8 bits / 1 byte of memory
### bool:
- 0-1 / false-true
- uses 8 bits / 1 byte of memory
### number:
- dynamically allocates memory until system-maximum is reached
- can be in *precise mode*, which makes it allocate as many bytes as needed to store **any** number ; indiated with the '@' character after the number literal
  - Why would I use this? Well, `10e999 + 1 == 10e999` is usually true, because `10e999` is a stupidly big number, BUT `10e999@ + 1 == 10e999@` returns false (x + 1 > x), because this language likes precision
  - This may be a lot slower than the system-maximum
## Characters
### char:
- Stores a singular character, inicated by surrounding with 's, e.g. 'a'
### string:
- Stores a sequence of characters, inicated by surrounding with "s, e.g. "abc"
- Cannot be indexed **yet**
## Variable declaration
### See [examples/declarations.lang](examples/declarations.lang) for more examples
#### Javascript
```ts
let javascriptMutuable = "Hello World";
const javascriptImmutable = "Hello World";
```
#### Typescript
```ts
let typescriptMutuable: any = "Hello World";
const typescriptImmutable: string = "Hello World";
```
#### Rust-like
```rust
mut rustLikeMutuable = "Hello World";
mut rustLikeMutuable2: string = "Hello World";
mut string notRustLikeMutuable = "Hello World";

string rustLikeImmutable = "Hello World";
```