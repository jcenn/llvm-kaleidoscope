## Language features
Similar to official kaleidoscope specs
- accepts only integers ( no type declarations )
- if / else statements
- allows for function declarations
- extern keyword for std::lib functions
- color parameter for strings / printf (ANSI)
### Example program
```
    fn add(x, y) -> i32 {
        return x + y;
    }
    
    // should output 3 to the standard output
    fn main(){
        return add(1, 2);
    }
```

## TODO
- [x] Fully functional arithmetic expression parsing (order of precedence)
- [x] Function calls
- [x] Extern statements
- [x] void functions and call statements
- [x] void returns (`return;`)
- [x] code comments (`// this is a comment`)
- [ ] visitor pattern codegen
- [ ] boolean expressions (==, <, >, !=, etc.)
- [ ] more arithmetic operations 
- [ ] handle negative values (and unary operators in general) 
- [ ] variable reassignment after let statements 
