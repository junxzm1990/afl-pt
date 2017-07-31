/^# Packages using this file: / {
  s/# Packages using this file://
  ta
  :a
  s/ cpio / cpio /
  tb
  s/ $/ cpio /
  :b
  s/^/# Packages using this file:/
}
