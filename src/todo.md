- Use std::reduce instead of loops if possible with the help of a structure (overflow, data)

- For multiply_large_unsigned_integer_sorted
  - Use Karatsuba algorithm instead of the naive implementation
  - Use Toom-Cook algorithm
  - Use Schönhage–Strassen algorithm
  This is useless as all multiplications are done with one large and one small number

- Use coroutine to generate digits which will allow a separation of the generation and the streaming
- use a multithread queue to allow async generation where the streaming doesn't impact on the generation

- Maybe clean up the whole projet and keep only the infinite stream of digits

- Multi-thread large_integer operations and enable it using CRTP or an executor
  - This could be applied to the digit generator where we look for the highest number that is not larger than the remainder

- Use std::to_chars (#include <charconv>)

- Compute_square_root_digit_by_digit_method to handle both integers and floating point values This new version should return a large_floating_point instead of a string

- Add computation time around each computing method

- Implement different initial estimate methods
    // (https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Approximations_that_depend_on_the_floating_point_representation)

- Using exponential identity {\displaystyle sqrt(S) = e^(0.5*ln(S)
    // (https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Exponential_identity)
- Using two-variable iterative method
    // (https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#A_two-variable_iterative_method)
- Using iterative methods for reciprocal square roots
    // (https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Iterative_methods_for_reciprocal_square_roots)
- Using Goldschmidt’s algorithm
    // (https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Goldschmidt%E2%80%99s_algorithm)
- Using Taylor series
    // (https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Taylor_series)
- Using continued fraction expansion
    // (https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Continued_fraction_expansion)

- Using quake3 approximation method (not fully portable as double representation specific)
    // (https://www.lomont.org/papers/2003/InvSqrt.pdf)
- Using Log base 2 approximation and Newton's method (undefined behavior as union is use to write to one type and read the other)
    // (https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Approximations_that_depend_on_the_floating_point_representation)
- Using Babylonian approximation method (previous + 2 iterations -> 0.25f*u.x + x/u.x;) (undefined behavior as union is use to write to one type and read the other)
- Using Bakhshali approximation (only one iteration)
- Using Newton method approximation (not fully portable as double representation specific)
    // (http://www.azillionmonkeys.com/qed/sqroot.html#calcmeth)
- Using float biased approximation (not fully portable as float representation specific)
    // (http://bits.stephan-brumme.com/squareRoot.html)

- Using recursion instead of loop (possibly using continued fraction expansion implementation)

- Implement a digit by digit version that stream the digits
- Using an async task
- Using a coroutine that return an infinite number of digits
- Using a coroutine that use thread (or an executor)
- Using a math library? -> not possible on most website
- Using a different locale -> maybe not as it's gonna be a ',' instead of a '.'
- Using template parameters, to be computed at compile time without consextpr
- As a reduced expression of prime factor sqrt(2)*sqrt(3)*sqrt(7)