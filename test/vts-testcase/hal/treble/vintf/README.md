# VTS Treble VINTF test

## Targets?

```
branch      | tests
------------+---------------------------------------------
O-MR1, 8.1  | (legacy binary)
------------+---------------------------------------------
P    , 9.0  | vts_treble_vintf_test
            | vts_treble_vintf_vendor_test
            | vts_treble_vintf_framework_test
------------+---------------------------------------------
Q    , 10.0 | vts_treble_vintf_test             (P binary)
            | vts_treble_vintf_vendor_test      (P binary)
            | vts_treble_vintf_vendor_test
            | vts_treble_vintf_framework_test
```

## What to run?

```
Vendor image | System image  |
VTS Tests    | VTS framework | VTS Treble VINTF test
-------------+---------------+---------------------------
O-MR1, 8.1   | O-MR1, 8.1    | (legacy binary)
-------------+---------------+---------------------------
O-MR1, 8.1   | P    , 9.0    | _test             (P)
             |               | _framework_test   (P)
-------------+---------------+---------------------------
P    , 9.0   | P    , 9.0    | _vendor_test      (P)
             |               | _framework_test   (P)
-------------+---------------+---------------------------
O-MR1, 8.1   | Q    , 10.0   | _test (P)
             |               | _framework_test   (Q)
-------------+---------------+---------------------------
P    , 9.0   | Q    , 10.0   | _vendor_test      (P)
             |               | _framework_test   (Q)
-------------+---------------+---------------------------
Q    , 10.0  | Q    , 10.0   | _vendor_test      (Q)
             |               | _framework_test   (Q)
```

## Summary

* O-MR1 is a special case; always run `vts_treble_vintf_test` binary.
* From P onwards, always run `_vendor_test` from VTS tests at VTS tests ${VENDOR}
  snapshot, and latest `_framework_test`.
