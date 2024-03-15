// Copyright Chad Engler

#include "he/core/macros.h"
#include "he/core/types.h"

namespace he
{
    enum class _AsciiCategory : uint8_t
    {
        None = 0,           ///< Control characters and non-printable characters
        Whitespace = 1,     ///< Space, tab, newline, etc
        UpperAlpha = 2,     ///< Uppercase alphabetic characters
        LowerAlpha = 3,     ///< Lowercase alphabetic characters
        Numeric = 5,        ///< Numeric digits
        Symbol = 6,         ///< Symbols and punctuation
    };

    inline constexpr _AsciiCategory _AsciiCategoryLookup[] =
    {
        // Control
        _AsciiCategory::None,           // 0  NUL (null)
        _AsciiCategory::None,           // 1  SOH (start of heading)
        _AsciiCategory::None,           // 2  STX (start of text)
        _AsciiCategory::None,           // 3  ETX (end of text)
        _AsciiCategory::None,           // 4  EOT (end of transmission)
        _AsciiCategory::None,           // 5  ENQ (enquiry)
        _AsciiCategory::None,           // 6  ACK (acknowledge)
        _AsciiCategory::None,           // 7  BEL (bell)
        _AsciiCategory::None,           // 8  BS  (backspace)
        _AsciiCategory::Whitespace,     // 9  TAB (horizontal tab)
        _AsciiCategory::Whitespace,     // 10  LF (NL line feed, new line)
        _AsciiCategory::Whitespace,     // 11  VT (vertical tab)
        _AsciiCategory::Whitespace,     // 12  FF (NP form feed, new page)
        _AsciiCategory::Whitespace,     // 13  CR (carriage return)
        _AsciiCategory::None,           // 14  SO (shift out)
        _AsciiCategory::None,           // 15  SI (shift in)
        _AsciiCategory::None,           // 16  DLE (data link escape)
        _AsciiCategory::None,           // 17  DC1 (device control 1)
        _AsciiCategory::None,           // 18  DC2 (device control 2)
        _AsciiCategory::None,           // 19  DC3 (device control 3)
        _AsciiCategory::None,           // 20  DC4 (device control 4)
        _AsciiCategory::None,           // 21  NAK (negative acknowledge)
        _AsciiCategory::None,           // 22  SYN (synchronous idle)
        _AsciiCategory::None,           // 23  ETB (end of trans. block)
        _AsciiCategory::None,           // 24  CAN (cancel)
        _AsciiCategory::None,           // 25  EM  (end of medium)
        _AsciiCategory::None,           // 26  SUB (substitute)
        _AsciiCategory::None,           // 27  ESC (escape)
        _AsciiCategory::None,           // 28  FS (file separator)
        _AsciiCategory::None,           // 29  GS (group separator)
        _AsciiCategory::None,           // 30  RS (record separator)
        _AsciiCategory::None,           // 31  US (unit separator)

        // Printable
        _AsciiCategory::Whitespace,     // 32  SP (space)
        _AsciiCategory::Symbol,         // 33  !
        _AsciiCategory::Symbol,         // 34  "
        _AsciiCategory::Symbol,         // 35  #
        _AsciiCategory::Symbol,         // 36  $
        _AsciiCategory::Symbol,         // 37  %
        _AsciiCategory::Symbol,         // 38  &
        _AsciiCategory::Symbol,         // 39  '
        _AsciiCategory::Symbol,         // 40  (
        _AsciiCategory::Symbol,         // 41  )
        _AsciiCategory::Symbol,         // 42  *
        _AsciiCategory::Symbol,         // 43  +
        _AsciiCategory::Symbol,         // 44  ,
        _AsciiCategory::Symbol,         // 45  -
        _AsciiCategory::Symbol,         // 46  .
        _AsciiCategory::Symbol,         // 47  /
        _AsciiCategory::Numeric,        // 48  0
        _AsciiCategory::Numeric,        // 49  1
        _AsciiCategory::Numeric,        // 50  2
        _AsciiCategory::Numeric,        // 51  3
        _AsciiCategory::Numeric,        // 52  4
        _AsciiCategory::Numeric,        // 53  5
        _AsciiCategory::Numeric,        // 54  6
        _AsciiCategory::Numeric,        // 55  7
        _AsciiCategory::Numeric,        // 56  8
        _AsciiCategory::Numeric,        // 57  9
        _AsciiCategory::Symbol,         // 58  :
        _AsciiCategory::Symbol,         // 59  ;
        _AsciiCategory::Symbol,         // 60  <
        _AsciiCategory::Symbol,         // 61  =
        _AsciiCategory::Symbol,         // 62  >
        _AsciiCategory::Symbol,         // 63  ?
        _AsciiCategory::Symbol,         // 64  @
        _AsciiCategory::UpperAlpha,     // 65  A
        _AsciiCategory::UpperAlpha,     // 66  B
        _AsciiCategory::UpperAlpha,     // 67  C
        _AsciiCategory::UpperAlpha,     // 68  D
        _AsciiCategory::UpperAlpha,     // 69  E
        _AsciiCategory::UpperAlpha,     // 70  F
        _AsciiCategory::UpperAlpha,     // 71  G
        _AsciiCategory::UpperAlpha,     // 72  H
        _AsciiCategory::UpperAlpha,     // 73  I
        _AsciiCategory::UpperAlpha,     // 74  J
        _AsciiCategory::UpperAlpha,     // 75  K
        _AsciiCategory::UpperAlpha,     // 76  L
        _AsciiCategory::UpperAlpha,     // 77  M
        _AsciiCategory::UpperAlpha,     // 78  N
        _AsciiCategory::UpperAlpha,     // 79  O
        _AsciiCategory::UpperAlpha,     // 80  P
        _AsciiCategory::UpperAlpha,     // 81  Q
        _AsciiCategory::UpperAlpha,     // 82  R
        _AsciiCategory::UpperAlpha,     // 83  S
        _AsciiCategory::UpperAlpha,     // 84  T
        _AsciiCategory::UpperAlpha,     // 85  U
        _AsciiCategory::UpperAlpha,     // 86  V
        _AsciiCategory::UpperAlpha,     // 87  W
        _AsciiCategory::UpperAlpha,     // 88  X
        _AsciiCategory::UpperAlpha,     // 89  Y
        _AsciiCategory::UpperAlpha,     // 90  Z
        _AsciiCategory::Symbol,         // 91  [
        _AsciiCategory::Symbol,         // 92  \ (backslash)
        _AsciiCategory::Symbol,         // 93  ]
        _AsciiCategory::Symbol,         // 94  ^
        _AsciiCategory::Symbol,         // 95  _
        _AsciiCategory::Symbol,         // 96  `
        _AsciiCategory::LowerAlpha,     // 97  a
        _AsciiCategory::LowerAlpha,     // 98  b
        _AsciiCategory::LowerAlpha,     // 99  c
        _AsciiCategory::LowerAlpha,     // 100  d
        _AsciiCategory::LowerAlpha,     // 101  e
        _AsciiCategory::LowerAlpha,     // 102  f
        _AsciiCategory::LowerAlpha,     // 103  g
        _AsciiCategory::LowerAlpha,     // 104  h
        _AsciiCategory::LowerAlpha,     // 105  i
        _AsciiCategory::LowerAlpha,     // 106  j
        _AsciiCategory::LowerAlpha,     // 107  k
        _AsciiCategory::LowerAlpha,     // 108  l
        _AsciiCategory::LowerAlpha,     // 109  m
        _AsciiCategory::LowerAlpha,     // 110  n
        _AsciiCategory::LowerAlpha,     // 111  o
        _AsciiCategory::LowerAlpha,     // 112  p
        _AsciiCategory::LowerAlpha,     // 113  q
        _AsciiCategory::LowerAlpha,     // 114  r
        _AsciiCategory::LowerAlpha,     // 115  s
        _AsciiCategory::LowerAlpha,     // 116  t
        _AsciiCategory::LowerAlpha,     // 117  u
        _AsciiCategory::LowerAlpha,     // 118  v
        _AsciiCategory::LowerAlpha,     // 119  w
        _AsciiCategory::LowerAlpha,     // 120  x
        _AsciiCategory::LowerAlpha,     // 121  y
        _AsciiCategory::LowerAlpha,     // 122  z
        _AsciiCategory::Symbol,         // 123  {
        _AsciiCategory::Symbol,         // 124  |
        _AsciiCategory::Symbol,         // 125  }
        _AsciiCategory::Symbol,         // 126  ~
        _AsciiCategory::None,           // 127  DEL (delete)

        // Extended
        _AsciiCategory::Symbol,         // 128  €
        _AsciiCategory::None,           // 129  (unused)
        _AsciiCategory::Symbol,         // 130  ‚
        _AsciiCategory::Symbol,         // 131  ƒ
        _AsciiCategory::Symbol,         // 132  „
        _AsciiCategory::Symbol,         // 133  …
        _AsciiCategory::Symbol,         // 134  †
        _AsciiCategory::Symbol,         // 135  ‡
        _AsciiCategory::Symbol,         // 136  ˆ
        _AsciiCategory::Symbol,         // 137  ‰
        _AsciiCategory::Symbol,         // 138  Š
        _AsciiCategory::Symbol,         // 139  ‹
        _AsciiCategory::Symbol,         // 140  Œ
        _AsciiCategory::None,           // 141  (unused)
        _AsciiCategory::Symbol,         // 142  Ž
        _AsciiCategory::None,           // 143  (unused)
        _AsciiCategory::None,           // 144  (unused)
        _AsciiCategory::Symbol,         // 145  ‘
        _AsciiCategory::Symbol,         // 146  ’
        _AsciiCategory::Symbol,         // 147  “
        _AsciiCategory::Symbol,         // 148  ”
        _AsciiCategory::Symbol,         // 149  •
        _AsciiCategory::Symbol,         // 150  –
        _AsciiCategory::Symbol,         // 151  —
        _AsciiCategory::Symbol,         // 152  ˜
        _AsciiCategory::Symbol,         // 153  ™
        _AsciiCategory::Symbol,         // 154  š
        _AsciiCategory::Symbol,         // 155  ›
        _AsciiCategory::Symbol,         // 156  œ
        _AsciiCategory::None,           // 157  (unused)
        _AsciiCategory::Symbol,         // 158  ž
        _AsciiCategory::Symbol,         // 159  Ÿ
        _AsciiCategory::Whitespace,     // 160  NBSP (non breaking space)
        _AsciiCategory::Symbol,         // 161  ¡
        _AsciiCategory::Symbol,         // 162  ¢
        _AsciiCategory::Symbol,         // 163  £
        _AsciiCategory::Symbol,         // 164  ¤
        _AsciiCategory::Symbol,         // 165  ¥
        _AsciiCategory::Symbol,         // 166  ¦
        _AsciiCategory::Symbol,         // 167  §
        _AsciiCategory::Symbol,         // 168  ¨
        _AsciiCategory::Symbol,         // 169  ©
        _AsciiCategory::Symbol,         // 170  ª
        _AsciiCategory::Symbol,         // 171  «
        _AsciiCategory::Symbol,         // 172  ¬
        _AsciiCategory::Symbol,         // 173  SHY (soft hyphen)
        _AsciiCategory::Symbol,         // 174  ®
        _AsciiCategory::Symbol,         // 175  ¯
        _AsciiCategory::Symbol,         // 176  °
        _AsciiCategory::Symbol,         // 177  ±
        _AsciiCategory::Symbol,         // 178  ²
        _AsciiCategory::Symbol,         // 179  ³
        _AsciiCategory::Symbol,         // 180  ´
        _AsciiCategory::Symbol,         // 181  µ
        _AsciiCategory::Symbol,         // 182  ¶
        _AsciiCategory::Symbol,         // 183  ·
        _AsciiCategory::Symbol,         // 184  ¸
        _AsciiCategory::Symbol,         // 185  ¹
        _AsciiCategory::Symbol,         // 186  º
        _AsciiCategory::Symbol,         // 187  »
        _AsciiCategory::Symbol,         // 188  ¼
        _AsciiCategory::Symbol,         // 189  ½
        _AsciiCategory::Symbol,         // 190  ¾
        _AsciiCategory::Symbol,         // 191  ¿
        _AsciiCategory::Symbol,         // 192  À
        _AsciiCategory::Symbol,         // 193  Á
        _AsciiCategory::Symbol,         // 194  Â
        _AsciiCategory::Symbol,         // 195  Ã
        _AsciiCategory::Symbol,         // 196  Ä
        _AsciiCategory::Symbol,         // 197  Å
        _AsciiCategory::Symbol,         // 198  Æ
        _AsciiCategory::Symbol,         // 199  Ç
        _AsciiCategory::Symbol,         // 200  È
        _AsciiCategory::Symbol,         // 201  É
        _AsciiCategory::Symbol,         // 202  Ê
        _AsciiCategory::Symbol,         // 203  Ë
        _AsciiCategory::Symbol,         // 204  Ì
        _AsciiCategory::Symbol,         // 205  Í
        _AsciiCategory::Symbol,         // 206  Î
        _AsciiCategory::Symbol,         // 207  Ï
        _AsciiCategory::Symbol,         // 208  Ð
        _AsciiCategory::Symbol,         // 209  Ñ
        _AsciiCategory::Symbol,         // 210  Ò
        _AsciiCategory::Symbol,         // 211  Ó
        _AsciiCategory::Symbol,         // 212  Ô
        _AsciiCategory::Symbol,         // 213  Õ
        _AsciiCategory::Symbol,         // 214  Ö
        _AsciiCategory::Symbol,         // 215  ×
        _AsciiCategory::Symbol,         // 216  Ø
        _AsciiCategory::Symbol,         // 217  Ù
        _AsciiCategory::Symbol,         // 218  Ú
        _AsciiCategory::Symbol,         // 219  Û
        _AsciiCategory::Symbol,         // 220  Ü
        _AsciiCategory::Symbol,         // 221  Ý
        _AsciiCategory::Symbol,         // 222  Þ
        _AsciiCategory::Symbol,         // 223  ß
        _AsciiCategory::Symbol,         // 224  à
        _AsciiCategory::Symbol,         // 225  á
        _AsciiCategory::Symbol,         // 226  â
        _AsciiCategory::Symbol,         // 227  ã
        _AsciiCategory::Symbol,         // 228  ä
        _AsciiCategory::Symbol,         // 229  å
        _AsciiCategory::Symbol,         // 230  æ
        _AsciiCategory::Symbol,         // 231  ç
        _AsciiCategory::Symbol,         // 232  è
        _AsciiCategory::Symbol,         // 233  é
        _AsciiCategory::Symbol,         // 234  ê
        _AsciiCategory::Symbol,         // 235  ë
        _AsciiCategory::Symbol,         // 236  ì
        _AsciiCategory::Symbol,         // 237  í
        _AsciiCategory::Symbol,         // 238  î
        _AsciiCategory::Symbol,         // 239  ï
        _AsciiCategory::Symbol,         // 240  ð
        _AsciiCategory::Symbol,         // 241  ñ
        _AsciiCategory::Symbol,         // 242  ò
        _AsciiCategory::Symbol,         // 243  ó
        _AsciiCategory::Symbol,         // 244  ô
        _AsciiCategory::Symbol,         // 245  õ
        _AsciiCategory::Symbol,         // 246  ö
        _AsciiCategory::Symbol,         // 247  ÷
        _AsciiCategory::Symbol,         // 248  ø
        _AsciiCategory::Symbol,         // 249  ù
        _AsciiCategory::Symbol,         // 250  ú
        _AsciiCategory::Symbol,         // 251  û
        _AsciiCategory::Symbol,         // 252  ü
        _AsciiCategory::Symbol,         // 253  ý
        _AsciiCategory::Symbol,         // 254  þ
        _AsciiCategory::Symbol,         // 255  ÿ
    };
    static_assert(HE_LENGTH_OF(_AsciiCategoryLookup) == 256);
}
