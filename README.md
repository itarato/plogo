# P-Logo

## Use

- compile: `make`
- run: `./main` or `./main <SOURCE>`

## Commands

```
pos(<X>, <Y>)

forward(<V>)
backward(<V>)

left(<V>)
right(<V>)

loop (<I>) {
}

if (<C>) {
}

if (<C>) {
} else {
}

fn <N>(<A1>, <A2>, ..) {
}
```

## Example

```
fn triangle(size, limit) {
    loop (3) {
        if (size > limit) {
            triangle(size / 2, limit)
        }

        f(size)
        r(120)
    }
}

pos(200, 100)
r(90)
triangle(600, 10)
```

![Triangle fractal](./misc/frac_triangle.png)

## Variables

Available variables to toggle (through the debug panel):

```
// Int values (0-500)
va
vb
vc
vd

// Start position
vstartx
vstarty
vstartangle
```

![Tree example](./misc/frac_tree.png)
