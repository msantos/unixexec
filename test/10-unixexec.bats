#!/usr/bin/env bats

@test "accept: client connection" {
  unixexec test.s echo "test" &
  run nc -U test.s
  expect="test"
  cat <<EOF
|$output|
---
|$expect|
EOF

  [ "$status" -eq 0 ]
  [ "$output" = "$expect" ]
}

@test "accept: unlink and bind socket" {
  unixexec test.s echo "test" &
  run nc -U test.s
  expect="test"
  cat <<EOF
|$output|
---
|$expect|
EOF

  [ "$status" -eq 0 ]
  [ "$output" = "$expect" ]
}

@test "accept: do not unlink existing socket and attempt to bind" {
  run unixexec -U test.s echo "test"
  [ "$status" -eq 111 ]
}

@test "accept: not a socket" {
  run unixexec -U /etc/passwd echo "test"
  [ "$status" -eq 111 ]
}
