3 :quadratic-roots defun
  :c var :b var :a var

  $b 2 ^
  4 $a * $c *
  -
  0.5 ^
  :det var

  $b neg $det +
  2 $a *
  /

  $b neg $det -
  2 $a *
  /
end
