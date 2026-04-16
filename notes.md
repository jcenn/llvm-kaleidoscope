## Current objective - variable / function parameter type hints
Goal:

Syntax like
`let a:i32 = 1;`


- Modify prototype parsing to treat parameters the same way as normal variables
- Make expressions bubble their types up to allow for type inference
- Check if expression type matches specified type for let statements and assignments
- Make sure variables aren't assigned with a void type

- arithmetic operations on doubles

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

## Other ideas
- functions from stdlib are automatically imported but have to be accessed with the std:: prefix if threre's a local function that shadows them 
ex. std::readline is available as `readline` unless user defines their own `readline` then the original has to be accessed via `std::readline`
- variable argument count printf(pattern, ...)