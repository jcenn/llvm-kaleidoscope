## Language features
Similar to official kaleidoscope specs
- accepts only integers ( no type declarations )
- if / else statements
- allows for function declarations
- extern keyword for std::lib functions
## Example compilation steps
### Source program
```
    fn main(){
        let x = 5 + 2;
        return x;
    }
```
### AST
Function(main){
    
}