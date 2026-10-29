#pragma once

namespace bb::game {

// Fixed-point trigonometry of the 2002 code (bSin, bCos in ben_maths): the
// sine and cosine of a whole number of degrees (0..359, not checked),
// times 1024. Shift a product right by kTrigShift to scale it back.
constexpr int kTrigShift = 10;  // COSINUS

int sine1024(int degrees) noexcept;
int cosine1024(int degrees) noexcept;

// The whole part of the square root of n, the int(sqrt(n)) of the 2002
// code without floating point (D-56); 0 for a negative n.
int squareRoot(int n) noexcept;

}  // namespace bb::game
