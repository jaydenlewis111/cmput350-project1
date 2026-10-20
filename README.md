# CMPUT 350 Project 1b

A C++20/SFML 3 Galaga game and reusable object-oriented engine. See
[project.txt](project.txt) for the state machine, design decisions,
 and the implemented Vector2D extra-credit item.

## Build and play

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/Project1
```

Press **5** to insert a credit, then **1** to start. Hold **A/D** or the arrow keys
to move; hold **Space** to fire. Three lives, five waves of eight enemies, and
three enemy types are included. Defeat the formation to advance to the next
level. Close the window to quit. The Part 1a ball demo remains available with
`./build/Project1 --balls`.


## Tests

Headless math, shape collision, notification, Bezier, and bullet tests:

```sh
ctest --test-dir build --output-on-failure
```

Optional Linux window/input and deterministic gameplay tests require an X display
and X11 development headers:

```sh
cmake -S . -B build -DENABLE_WINDOW_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Strict compiler checks can be enabled with
`-DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic"`.



