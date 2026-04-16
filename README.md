## Kaleidoscope
Compiled toy programming language based on the [Kaleidoscope](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html) guide series from LLVM.

### Features:
- function declarations
- `extern` statements for importing C functions
- simple mathematical expressions (ex. 5 + 2 - 3)
- function calls
- if statements
- for/while loops


### Limitations:
- available types: i32, double, string
- does not support array types, including dynamically created strings

### Example program
available at `programs/example.kld`
Iterates over a set amount of natural numbers and prints if the number is even or odd
```
extern printf(pattern: string, val: i32);

fn main(){
    let i: i32 = 0;
    for i < 5 {
        if i % 2 == 0 {
            printf("number %d is even", i);
        }else {
            printf("number %d is odd", i);
        }
        i = i + 1;
    }
    return 0;
}

```
