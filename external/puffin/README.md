# Puffin

Source code for Puffin: A utility for deterministic DEFLATE recompression.

TODO(ahassani): Describe the directory structure and how-tos.

## Glossary

* __Alphabet__ A value that occurs in the input stream. It can be either a
    literal:[0..255], and end of block sign [256], a length[257..285], or a
    distance [0..29].

* __Huffman code__ A variable length code representing the Huffman encoded of an
    alphabet. Huffman codes can be created uniquely using Huffman code length
    array.
* __Huffman code array__ An array which an array index identifies a Huffman code
    and the array element in that index represents the corresponding
    alphabet. Throughout the code, Huffman code arrays are identified by
    vectors with postfix `hcodes_`.
* __Huffman reverse code array__ An array which an array index identifies an
    alphabet and the array element in that index contains the Huffman code of
    the alphabet. Throughout the code, The Huffman reverse code arrays are
    identified by vectors with postfix `rcodes_`.
* __Huffman code length__ The number of bits in a Huffman code.
* __Huffman code length array__ An array of Huffman code lengths with the array
    index as the alphabet. Throughout the code, Huffman code length arrays
    are identified by vectors with postfix `lens_`.
