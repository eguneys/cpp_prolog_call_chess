`swipl-ld -shared -o test_cpp test_cpp.cpp`


```bash
swipl test.pl
?- use_foreign_library(test_cpp).
?- hello(world).
Hello world
true.
```


[https://github.com/SWI-Prolog/packages-cpp/blob/master/test_cpp.cpp](https://github.com/SWI-Prolog/packages-cpp/blob/master/test_cpp.cpp).


[https://github.com/SWI-Prolog/packages-cpp/blob/master/test_cpp.pl](https://github.com/SWI-Prolog/packages-cpp/blob/master/test_cpp.pl).