# TensorFlow Lite Optimizing Converter command-line reference

This page is complete reference of command-line flags. It is complemented by the
following other documents:

*   [README](../README.md)
*   [Command-line examples](cmdline_examples.md)

Table of contents:

[TOC]

## High-level overview

A full list and detailed specification of all flags is given in the next
section. For now we focus on a higher-level description of command lines:

```
toco \
  --input_format=... \
  --output_format=... \
  --input_file=... \
  --output_file=... \
  [model flags...] \
  [transformation flags...] \
  [logging flags...]
```

In other words, the converter requires at least the following mandatory flags:
`--input_format`, `--output_format`, `--input_file`, `--output_file`. Depending
on the input and output formats, additional flags may be allowed or mandatory:

*   *Model flags* provide additional information about the model stored in the
    input file.
    *   `--output_array` or `--output_arrays` specify which arrays in the input
        file are to be considered the output activations.
    *   `--input_array` or `--input_arrays` specify which arrays in the input
        file are to be considered the input activations.
    *   `--input_shape` or `--input_shapes` specify the shapes of the input
        arrays.
    *   `--input_data_type` or `--input_data_types` specify the data types of
        input arrays, which can be used if the input file does not already
        specify them.
    *   `--mean_value` or `--mean_values`, and `--std_value` or `--std_values`,
        give the dequantization parameters of the input arrays, for the case
        when the output file will accept quantized input arrays.
*   *Transformation flags* specify options of the transformations to be applied
    to the graph, i.e. they specify requested properties that the output file
    should have.
    *   `--inference_type` specifies the type of real-numbers arrays in the
        output file. This only affects arrays of real numbers and allows to
        control their quantization or dequantization, effectively switching
        between floating-point and quantized arithmetic for the inference
        workload, as far as real numbers are concerned. Other data types are
        unaffected (e.g. plain integers, and strings).
    *   `--inference_input_type` is like `--inference_type` but specifically
        controlling input arrays, separately from other arrays. If not
        specified, then `--inference_type` is used. The use case for specifying
        `--inference_input_type` is when one wants to perform floating-point
        inference on a quantized input, as is common in image models operating
        on bitmap image inputs.
    *   Some transformation flags allow to carry on with quantization when the
        input graph is not properly quantized: `--default_ranges_min`,
        `--default_ranges_max`, `--drop_fake_quant`,
        `--reorder_across_fake_quant`.
*   *Logging flags* described below.

## Command-line flags complete reference

### Mandatory flags

*   `--input_format`. Type: string. Specifies the format of the input file.
    Allowed values:
    *   `TENSORFLOW_GRAPHDEF` &mdash; The TensorFlow GraphDef format. Both
        binary and text proto formats are allowed.
    *   `TFLITE` &mdash; The TensorFlow Lite flatbuffers format.
*   `--output_format`. Type: string. Specifies the format of the output file.
    Allowed values:
    *   `TENSORFLOW_GRAPHDEF` &mdash; The TensorFlow GraphDef format. Always
        produces a file in binary (not text) proto format.
    *   `TFLITE` &mdash; The TensorFlow Lite flatbuffers format.
        *   Whether a float or quantized TensorFlow Lite file will be produced
            depends on the `--inference_type` flag.
    *   `GRAPHVIZ_DOT` &mdash; The GraphViz `.dot` format. This asks the
        converter to generate a reasonable graphical representation of the graph
        after simplification by a generic set of transformation.
        *   A typical `dot` command line to view the resulting graph might look
            like: `dot -Tpdf -O file.dot`.
        *   Note that since passing this `--output_format` means losing the
            information of which output format you actually care about, and
            since the converter's transformations depend on the specific output
            format, the resulting visualization may not fully reflect what you
            would get on the actual output format that you are using. To avoid
            that concern, and generally to get a visualization of exactly what
            you get in your actual output format as opposed to just a merely
            plausible visualization of a model, consider using `--dump_graphviz`
            instead and keeping your true `--output_format`.
*   `--input_file`. Type: string. Specifies the path of the input file. This may
    be either an absolute or a relative path.
*   `--output_file`. Type: string. Specifies the path of the output file.

### Model flags

*   `--output_array`. Type: string. Specifies a single array as the output
    activations. Incompatible with `--output_arrays`.
*   `--output_arrays`. Type: comma-separated list of strings. Specifies a list
    of arrays as the output activations, for models with multiple outputs.
    Incompatible with `--output_array`.
*   `--input_array`. Type: string. Specifies a single array as the input
    activations. Incompatible with `--input_arrays`.
*   `--input_arrays`. Type: comma-separated list of strings. Specifies a list of
    arrays as the input activations, for models with multiple inputs.
    Incompatible with `--input_array`.

When `--input_array` is used, the following flags are available to provide
additional information about the single input array:

*   `--input_shape`. Type: comma-separated list of integers. Specifies the shape
    of the input array, in TensorFlow convention: starting with the outer-most
    dimension (the dimension corresponding to the largest offset stride in the
    array layout), ending with the inner-most dimension (the dimension along
    which array entries are typically laid out contiguously in memory).
    *   For example, a typical vision model might pass
        `--input_shape=1,60,80,3`, meaning a batch size of 1 (no batching), an
        input image height of 60, an input image width of 80, and an input image
        depth of 3, for the typical case where the input image is a RGB bitmap
        (3 channels, depth=3) stored by horizontal scanlines (so 'width' is the
        next innermost dimension after 'depth').
*   `--mean_value` and `--std_value`. Type: floating-point. The decimal point
    character is always the dot (`.`) regardless of the locale. These specify
    the (de-)quantization parameters of the input array, when it is quantized.
    *   The meaning of mean_value and std_value is as follows: each quantized
        value in the quantized input array will be interpreted as a mathematical
        real number (i.e. as an input activation value) according to the
        following formula:
        *   `real_value = (quantized_input_value - mean_value) / std_value`.
    *   When performing float inference (`--inference_type=FLOAT`) on a
        quantized input, the quantized input would be immediately dequantized by
        the inference code according to the above formula, before proceeding
        with float inference.
    *   When performing quantized inference
        (`--inference_type=QUANTIZED_UINT8`), no dequantization is ever to be
        performed by the inference code; however, the quantization parameters of
        all arrays, including those of the input arrays as specified by
        mean_value and std_value, all participate in the determination of the
        fixed-point multipliers used in the quantized inference code.

When `--input_arrays` is used, the following flags are available to provide
additional information about the multiple input arrays:

*   `--input_shapes`. Type: colon-separated list of comma-separated lists of
    integers. Each comma-separated list of integer gives the shape of one of the
    input arrays specified in `--input_arrays`, in the same order. See
    `--input_shape` for details.
    *   Example: `--input_arrays=foo,bar --input_shapes=2,3:4,5,6` means that
        there are two input arrays. The first one, "foo", has shape [2,3]. The
        second one, "bar", has shape [4,5,6].
*   `--mean_values`, `--std_values`. Type: comma-separated lists of
    floating-point numbers. Each number gives the corresponding value for one of
    the input arrays specified in `--input_arrays`, in the same order. See
    `--mean_value`, `--std_value` for details.

### Transformation flags

*   `--inference_type`. Type: string. Sets the type of real-number arrays in the
    output file, that is, controls the representation (quantization) of real
    numbers in the output file, except for input arrays, which are controlled by
    `--inference_input_type`.

    This flag only impacts real-number arrays. By "real-number" we mean float
    arrays, and quantized arrays. This excludes plain integer arrays, strings
    arrays, and every other data type.

    For real-number arrays, the impact of this flag is to allow the output file
    to choose a different real-numbers representation (quantization) from what
    the input file used. For any other types of arrays, changing the data type
    would not make sense.

    Specifically:

    *   If `FLOAT`, then real-numbers arrays will be of type float in the output
        file. If they were quantized in the input file, then they get
        dequantized.
    *   If `QUANTIZED_UINT8`, then real-numbers arrays will be quantized as
        uint8 in the output file. If they were float in the input file, then
        they get quantized.
    *   If not set, then all real-numbers arrays retain the same type in the
        output file as they have in the input file.

*   `--inference_input_type`. Type: string. Similar to inference_type, but
    allows to control specifically the quantization of input arrays, separately
    from other arrays.

    If not set, then the value of `--inference_type` is implicitly used, i.e. by
    default input arrays are quantized like other arrays.

    Like `--inference_type`, this only affects real-number arrays. By
    "real-number" we mean float arrays, and quantized arrays. This excludes
    plain integer arrays, strings arrays, and every other data type.

    The typical use for this flag is for vision models taking a bitmap as input,
    typically with uint8 channels, yet still requiring floating-point inference.
    For such image models, the uint8 input is quantized, i.e. the uint8 values
    are interpreted as real numbers, and the quantization parameters used for
    such input arrays are their `mean_value`, `std_value` parameters.

*   `--default_ranges_min`, `--default_ranges_max`. Type: floating-point. The
    decimal point character is always the dot (`.`) regardless of the locale.
    These flags enable what is called "dummy quantization". If defined, their
    effect is to define fallback (min, max) range values for all arrays that do
    not have a properly specified (min, max) range in the input file, thus
    allowing to proceed with quantization of non-quantized or
    incorrectly-quantized input files. This enables easy performance prototyping
    ("how fast would my model run if I quantized it?") but should never be used
    in production as the resulting quantized arithmetic is inaccurate.

*   `--drop_fake_quant`. Type: boolean. Default: false. Causes fake-quantization
    nodes to be dropped from the graph. This may be used to recover a plain
    float graph from a fake-quantized graph.

*   `--reorder_across_fake_quant`. Type: boolean. Default: false. Normally,
    fake-quantization nodes must be strict boundaries for graph transformations,
    in order to ensure that quantized inference has the exact same arithmetic
    behavior as quantized training --- which is the whole point of quantized
    training and of FakeQuant nodes in the first place. However, that entails
    subtle requirements on where exactly FakeQuant nodes must be placed in the
    graph. Some quantized graphs have FakeQuant nodes at unexpected locations,
    that prevent graph transformations that are necessary in order to generate a
    well-formed quantized representation of these graphs. Such graphs should be
    fixed, but as a temporary work-around, setting this
    reorder_across_fake_quant flag allows the converter to perform necessary
    graph transformations on them, at the cost of no longer faithfully matching
    inference and training arithmetic.

### Logging flags

The following are standard Google logging flags:

*   `--logtostderr` redirects Google logging to standard error, typically making
    it visible in a terminal.
*   `--v` sets verbose logging levels (for debugging purposes). Defined levels:
    *   `--v=1`: log all graph transformations that did make a change on the
        graph.
    *   `--v=2`: log all graph transformations that did *not* make a change on
        the graph.

The following flags allow to generate graph visualizations of the actual graph
at various points during transformations:

*   `--dump_graphviz=/path` enables dumping of the graphs at various stages of
    processing as GraphViz `.dot` files. Generally preferred over
    `--output_format=GRAPHVIZ_DOT` as this allows you to keep your actually
    relevant `--output_format`.
*   `--dump_graphviz_video` enables dumping of the graph after every single
    graph transformation (for debugging purposes).
