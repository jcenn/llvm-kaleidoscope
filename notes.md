## Language features
Similar to official kaleidoscope specs
- accepts only integers ( no type declarations )
- if / else statements
- allows for function declarations
- extern keyword for std::lib functions

### Example ource program
```
    fn main(){
        let x = 5 + 2;
        return x;
    }
```

## TODO
- [ ] Fully functional arithmetic expression parsing (order of precedence)
- [x] Function calls
- [x] Extern keyword
- [ ] void functions and call statements
- [ ] void returns (`return;`)