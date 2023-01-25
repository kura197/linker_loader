# linker & loader
Toy programs to learn how linkers and loaders work.  

## linker
Read object files and load them. Then, dynamically apply relocations and finally call a specified function.   
example:
```
cd linker
./dynlink sample_main test_main.o sample.o sample2.o
```

## loader
Read a executable file and load its data segments, and then invoke its entry point or dynamic linker's entry point. 

example:  
```
./loader/loader ./others/elfdump ./loader/loader
```
