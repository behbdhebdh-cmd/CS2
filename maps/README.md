# Map meshes

`.tri` files are generated locally and are not committed.

```bat
cd D:\CS2\cphys-extractor
dotnet run -c Release -- --official --tri --out D:\CS2\maps --nopause
```

Output: `D:\CS2\maps\tri\de_mirage.tri` (and the other official maps).
The overlay loads the file that matches the current map name.
