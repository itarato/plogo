# P-Logo

## Use

- compile: `make`
- run: `./main` or `./main <SOURCE>`

## Commands

```
clear()

pos(<val>, <val>)

forward(<length>)
backward(<length>)

left(<angle>)
right(<angle>)

loop (<number>) {
}

if (<cond>) {
}

if (<cond>) {
} else {
}

fn <name>(<arg1>, <arg2>, ..) {
}

rand(<min>, <max>)

intvar(<varname>, <min>, <max>, <default>)
```

## Example

```
fn leaf(size, angle, iter, dec, limit) {
  x = getx()
  y = gety()
  oldangle = getangle()

  loop (iter) {
    f(size)
    r(angle)
    size = size * dec

    if (size > limit) {
      l(90)
      leaf(size * 0.5, angle, iter * 0.7, dec, limit)
      l(180)
      leaf(size * 0.5, angle, iter * 0.7, dec, limit)
      l(90)
    }
  }

  pos(x, y)
  angle(oldangle)
}

intvar("size", 10, 130, 120)
intvar("angle", 0, 60, 10)
intvar("iter", 2, 26, 18)
intvar("dec", 0.5, 1, 0.85)
intvar("limit", 0.5, 5, 1)

leaf(size, angle, iter, dec, limit)
```

![Leaf fractal](./misc/frac_leaf.png)

## Variables

Variable control UI can be obtained with `intvar`, eg: `intvar("size", 0, 200, 50)`.

![Tree example](./misc/frac_tree_vars.png)
