# ctikz-compiler
A higher-level language for describing geometric diagrams, that compiles to tikz. This project currently supports a very small subset of TikZ (just points and lines), more features will be implemented soon...

## How To Use
This project builds using [nobpp](https://github.com/arlotaylor/nobpp), my single-header C++17 build system. If that doesn't work for you, just compile and link all of the .cpp files in the src directory with at least C++17. This project uses only standard libraries, so this shouldn't be very difficult.

Once you have built the project, simply run it and enter the path of a text file you want to compile. Currently TikZ is the only supported backend, so the program automatically outputs TikZ.

## The Language Spec
### Drawing Points
To tell the compiler to draw a point, just write that point's coordinates, in the form `(x, y)`. Spaces are ignored (except the leading space in each line, more on that later). The following example displays four points.
```
(0,0)
(1,0)
(0,1)
(1,1)
```
You can also write these on the same line, like so:
```
(0,0); (1,0); (0,1); (1,1)
```
Where you write these points in your source code does not matter; if the compiler sees a point, it draws it. To prevent a point from being drawn, simply prepend it with an underscore. The following example only draws three points:
```
(0,0); _(1,0); (0,1); (1,1)
```

### Drawing Lines
The simplest (and currently only) way to specify a line is with two points it passes through. We do this with the `ltp` function (as in 'line through points'). The following example draws a line from (0,0) to (1,1):
```
ltp((0,0), (1,1))
```
Note that this also draws the two points (0,0) and (1,1). To draw only the line, we can write:
```
ltp(_(0,0), _(1,1))
```
Now that we have lines, we can also specify a point as the intersection of two lines, with the `pil` function (think 'point intersection of lines'). The following example draws only the point (0.5, 0.5).
```
pil(_ltp(_(0,0), _(1,1)), _ltp(_(1,0), _(0,1)))
```
Note that by placing an underscore before our lines, we avoid drawing them.

### Variables
As you create more complex diagrams, you may want to save drawable objects for later use. We can store any drawable object to a variable, in this case `cool_line` with the following syntax:
```
$cool_line = _ltp(_(0,3), _(4,0))
```
This stores the line without drawing it. To use it later, we simply write `$cool_line`:
```
pil($cool_line, $boring_line)
```
For implementation reasons, the `$` in a variable definition must be the first character of its line, and the `$` in a variable reference cannot be the first character of its line. If you want to start a line with a variable reference, you must add whitespace beforehand:
```
  $cool_line
```
Note however that this line would not do anything, as the object `$cool_line` contains is drawn (or not drawn) when `$cool_line` is defined.

### Math
This language uses Polish notation for its mathematical expressions. That means that the expression `(3 + 5) * 2` would be expressed as `* + 3, 5, 2`. The commas in such an expression are optional, whitespace is sufficient to separate numbers. Parentheses are not supported, as they have no meaning in Polish notation.
The following operators are supported:
* `+`, `*`, `/`, `%` and `^` represent addition, multiplication, division, modulo and exponentiation respectively.
* `-` represents negation, so a subtraction such as `a - b` is represented by `+ a, -b` rather than `- a, b`.
* `s`, `c`, `t`, `as`, `ac` and `at` represent sine, cosine, tangent and their inverses respectively.
* `p` represents pi and `e` represents Euler's number e.

All mathematical calculations are done with floating point numbers. A floating point number may also be stored as a variable, with the following syntax:
```
$my_favourite_number =: / ^p2 6
```
This is differentiated from the typical variable assignment by the `:`.
Such numerical variables may also be used mathematical expressions in the same way that drawable variables are referenced.

### Function Definition
Functions are essentially treated as a third kind of variable, with the following syntax:
```
$drawTriangle(3) = ltp($0,$1); ltp($0,$2); ltp($1,$2)
```
The above code defines a function that takes three arguments (which are assumed to be points), and draws a triangle with them. The syntax for calling such a function is the following:
```
  $drawTriangle((0,0), (1,0), (0,1))
```
Notice the whitespace beforehand. This is required for the same reasons mentioned in the variables section.
For now, functions can only take drawable objects as arguments, and return the final statement they contain. Thus the following code would draw a triangle as well as the point (0.5, 0.5):
```
pil($drawTriangle(_(0,0),_(1,0),(0,1)),_ltp(_(0,0),_(1,1)))
```

### That's All For Now
I plan to add more features in the future, including new drawable objects, styled objects, and programmatic constructs such as conditionals and loops.
