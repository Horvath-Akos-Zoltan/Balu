# [WIP] Important: This language does not yet compile, it only has a lexer and a parser
#
# The Balu Programming Language
### Tired of constantly getting compile-time errors?
### Try out the Balu programming language! It fixes itself, so your HTML brain can rest easy.
### You don't want to learn a new syntax? We support multiple kinds of syntaxes! (e.g. [Variable declaration](#variable-declaration))

## How to install?
### It's a singular file, so just download it and put it in any folder you'd like

## How to use?
### Windows: `.\main.exe filename.balu`
### Currently only Windows is supported
### To ignore the errors, just add the `-np`, or `--no-panic` flag in the command line
### For help, add the `-h`, `--help`, `-?`, `/help`, `/h` or `/help` flag in the command line

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