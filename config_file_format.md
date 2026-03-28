### Format:

#### Basic Key-Value Pairs

Each setting is defined as a **key-value pair**.  
The assignment character `=` associates a key with a value. Whitespace is optional.

```ini
  key1=value
  key2 = value
  key3 = 'value'
  key4 = "value"
  ...
```

Multiple pairs can also appear on the same line, separated by commas:

```ini
  key1=value, key2 = value, key3 = 'value', key4 = "value", ...
```

#### Structs

A struct is a collection of **key-value pairs** grouped together using square brackets `[` `]`.
This allows related settings to be logically grouped.

```ini
  struct = [
    key1=value,
    key2 = value,
    key3 = 'value',
    key4 = "value",
    ...
  ]
```

Structs can also be written in a single line:

```ini
  struct = [ key1=value, key2 = value, key3 = 'value', key4 = "value", ... ]
```
